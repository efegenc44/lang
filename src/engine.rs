use std::num::IntErrorKind;

use apnum::{self, BigInt, BigNat};

use crate::{
    common::*,
    parser::{BinaryOp, Expression, TopLevel, TypeExpr, UnaryOp},
};

pub struct Engine {
    env: Environment<Value>,
}

impl Engine {
    pub fn new() -> Self {
        Self {
            env: Environment::new(),
        }
    }

    pub fn evaluate(&mut self, e: &Spanned<Expression>) -> EvaluationResult {
        use Expression::*;

        Ok(match &e.data {
            Identifier(symbol) => self.env.resolve(symbol).unwrap().clone(),
            NaturalNumber(nat) => {
                Value::Natural(match nat.parse::<Nat>() {
                    Ok(nat) => NaturalValue::Small(nat),
                    Err(err) => {
                        // Other errors shouldn't happen.
                        debug_assert_eq!(err.kind(), &IntErrorKind::PosOverflow);
                        NaturalValue::Big(BigNat::try_from(&**nat).unwrap())
                    }
                })
            }
            RealNumber(real) => Value::Real(real.parse::<Real>().unwrap()),
            BoolValue(value) => Value::Bool(*value),
            UnitValue => Value::Unit,
            Binary { op, left, right } => self.evaluate_binary_op(op, left, right)?,
            Unary { op, operand } => self.evaluate_unary_op(op, operand)?,
            Let {
                name,
                type_annot: _,
                value,
                expr,
            } => self.evaluate_let_expr(name, value, expr)?,
            Fun {
                name,
                args,
                return_type: _,
                expr,
                in_expr,
            } => self.evaluate_fun_expr(name, args, expr, in_expr)?,
            FunctionCall { f, args } => self.evaluate_function_call(f, args)?,
            If {
                condition,
                true_expr,
                false_expr,
            } => self.evaluate_if_expr(condition, true_expr, false_expr)?,
        })
    }

    fn evaluate_binary_op(
        &mut self,
        op: &BinaryOp,
        left: &Spanned<Expression>,
        right: &Spanned<Expression>,
    ) -> EvaluationResult {
        use BinaryOp::*;
        use Expression::*;

        let lvalue = self.evaluate(left)?;
        let rvalue = self.evaluate(right)?;

        Ok(match op {
            Addition => lvalue + rvalue,
            Multiplication => lvalue * rvalue,
            Subtraction => lvalue - rvalue,
            Division => (lvalue / rvalue).ok_or_else(|| {
                EvaluationError::AttemptToDivideByZero.spanned(right.span.clone())
            })?,
            And => lvalue & rvalue,
            Or => lvalue | rvalue,
            Equal => Value::Bool(lvalue == rvalue),
            NotEqual => Value::Bool(lvalue != rvalue),
            Less => Value::Bool(lvalue < rvalue),
            LessEqual => Value::Bool(lvalue <= rvalue),
            Greater => Value::Bool(lvalue > rvalue),
            GreaterEqual => Value::Bool(lvalue >= rvalue),
            Sequence => rvalue,
            Assignment => {
                match &left.data {
                    Identifier(symbol) => self.env.assign(symbol, rvalue),
                    _ => unreachable!(),
                }
                Value::Unit
            }
        })
    }

    fn evaluate_unary_op(
        &mut self,
        op: &UnaryOp,
        operand: &Spanned<Expression>,
    ) -> EvaluationResult {
        use UnaryOp::*;

        let ovalue = self.evaluate(operand)?;

        Ok(match op {
            Negation => -ovalue,
            Not => !ovalue,
        })
    }

    fn evaluate_let_expr(
        &mut self,
        name: &Spanned<Symbol>,
        value: &Spanned<Expression>,
        expr: &Spanned<Expression>,
    ) -> EvaluationResult {
        let value = self.evaluate(value)?;
        self.env.define_local(name.data.clone(), value);
        let result = self.evaluate(expr)?;
        self.env.shallow(1);
        Ok(result)
    }

    fn evaluate_fun_expr(
        &mut self,
        name: &Spanned<Symbol>,
        args: &[(Spanned<Symbol>, Spanned<TypeExpr>)],
        expr: &Spanned<Expression>,
        in_expr: &Spanned<Expression>,
    ) -> EvaluationResult {
        let args = args
            .iter()
            .map(|(name, _)| name.data.clone())
            .collect::<Vec<_>>();

        self.env.define_local(
            name.data.clone(),
            Value::Function {
                name: name.data.clone(),
                args,
                expr: expr.clone(),
            },
        );

        let result = self.evaluate(in_expr)?;
        self.env.shallow(1);
        Ok(result)
    }

    fn evaluate_if_expr(
        &mut self,
        condition: &Spanned<Expression>,
        true_expr: &Spanned<Expression>,
        false_expr: &Option<Box<Spanned<Expression>>>,
    ) -> EvaluationResult {
        Ok(if self.evaluate(condition)?.bool() {
            self.evaluate(true_expr)?
        } else if let Some(false_expr) = false_expr {
            self.evaluate(false_expr)?
        } else {
            Value::Unit
        })
    }

    fn evaluate_function_call(
        &mut self,
        f: &Spanned<Expression>,
        args: &[Spanned<Expression>],
    ) -> EvaluationResult {
        let f = self.evaluate(f)?;
        let arg_values = args
            .iter()
            .map(|arg| self.evaluate(arg))
            .collect::<Result<Vec<_>, _>>()?;

        self.call_function(f, &arg_values)
    }

    pub fn evaluate_top_level(&mut self, definitions: &Vec<Spanned<TopLevel>>) -> EvaluationResult {
        use TopLevel::*;

        // First pass - Collect definitions
        for definition in definitions {
            #[allow(clippy::single_match)]
            match &definition.data {
                Fun {
                    name,
                    args,
                    return_type: _,
                    expr,
                } => {
                    let args = args
                        .iter()
                        .map(|(name, _)| name.data.clone())
                        .collect::<Vec<_>>();

                    self.env.define_global(
                        name.data.clone(),
                        Value::Function {
                            name: name.data.clone(),
                            args,
                            expr: expr.clone(),
                        },
                    )
                }
                _ => (),
            }
        }

        for definition in definitions {
            match &definition.data {
                Let {
                    name,
                    type_annot: _,
                    value,
                } => {
                    let value = self.evaluate(value)?;
                    self.env.define_global(name.data.clone(), value);
                }
                Fun { .. } => (),
            }
        }

        let entry_point = self.env.resolve_global("main").unwrap().clone();
        let result = self.call_function(entry_point, &[])?;
        Ok(result)
    }

    fn call_function(&mut self, f: Value, arg_values: &[Value]) -> EvaluationResult {
        let Value::Function { name, args, expr } = f.clone() else {
            unreachable!()
        };
        let args_len = args.len();
        for (arg, value) in std::iter::zip(args, arg_values) {
            self.env.define_local(arg, value.clone());
        }
        self.env.define_local(name, f);
        let ftype = self.evaluate(&expr)?;
        self.env.shallow(args_len + 1);
        Ok(ftype)
    }
}

type EvaluationResult = Result<Value, Spanned<EvaluationError>>;

#[derive(Debug)]
pub enum EvaluationError {
    AttemptToDivideByZero,
}

impl HasSpan for EvaluationError {}

impl Error for EvaluationError {
    fn message(&self) -> String {
        use EvaluationError::*;

        match self {
            AttemptToDivideByZero => "Attempted to divide by zero".to_string(),
        }
    }
}

#[derive(Clone)]
pub enum Value {
    Natural(NaturalValue),
    Integer(IntegerValue),
    Real(Real),
    Bool(bool),
    Function {
        name: Symbol,
        args: Vec<Symbol>,
        expr: Spanned<Expression>,
    },
    Unit,
}

impl std::fmt::Display for Value {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use Value::*;

        match self {
            Natural(nat) => write!(f, "{nat}"),
            Integer(int) => write!(f, "{int}"),
            Real(real) => write!(f, "{real}"),
            Bool(value) => write!(f, "{value}"),
            Function {
                name,
                args: _,
                expr: _,
            } => write!(f, "<function: {name}>"),
            Unit => write!(f, "()"),
        }
    }
}

impl Value {
    fn bool(self) -> bool {
        use Value::*;

        match self {
            Bool(bool) => bool,
            _ => unreachable!(),
        }
    }

    fn real(self) -> Real {
        use IntegerValue as iv;
        use NaturalValue as nv;
        use Value::*;

        match self {
            Natural(nat) => match nat {
                nv::Small(nat) => nat as crate::common::Real,
                // TODO: Proper BigNum to float conversion.
                nv::Big(nat) => nat.to_string().parse().unwrap(),
            },
            Integer(int) => match int {
                iv::Small(int) => int as crate::common::Real,
                // TODO: Proper BigNum to float conversion.
                iv::Big(int) => int.to_string().parse().unwrap(),
            },
            Real(real) => real,
            _ => unreachable!(),
        }
    }

    fn int(self) -> IntegerValue {
        use IntegerValue as iv;
        use NaturalValue as nv;
        use Value::*;

        match self {
            Natural(nat) => match nat {
                nv::Small(nat) => match Int::try_from(nat) {
                    Ok(int) => iv::Small(int),
                    Err(_) => iv::Big(BigInt::from(nat)),
                },
                nv::Big(nat) => iv::Big(nat.into()),
            },
            Integer(int) => int,
            _ => unreachable!(),
        }
    }
}

// Match if a binary tuple contains specified value in either of its containers
macro_rules! any {
    ($v:ident, $l:ident, $r:ident) => (($l @ $v(_), $r) | ($l, $r @ $v(_)))
}

impl std::ops::Add for Value {
    type Output = Value;

    fn add(self, rhs: Self) -> Self::Output {
        use Value::*;

        match (self, rhs) {
            any!(Real, lvalue, rvalue) => Real(lvalue.real() + rvalue.real()),
            any!(Integer, lvalue, rvalue) => Integer(lvalue.int() + rvalue.int()),
            (Natural(lnat), Natural(rnat)) => Natural(lnat + rnat),
            _ => unreachable!(),
        }
    }
}

impl std::ops::Mul for Value {
    type Output = Value;

    fn mul(self, rhs: Self) -> Self::Output {
        use Value::*;

        match (self, rhs) {
            any!(Real, lvalue, rvalue) => Real(lvalue.real() * rvalue.real()),
            any!(Integer, lvalue, rvalue) => Integer(lvalue.int() * rvalue.int()),
            (Natural(lnat), Natural(rnat)) => Natural(lnat * rnat),
            _ => unreachable!(),
        }
    }
}

impl std::ops::Sub for Value {
    type Output = Value;

    fn sub(self, rhs: Self) -> Self::Output {
        use Value::*;

        match (self, rhs) {
            any!(Real, lvalue, rvalue) => Real(lvalue.real() - rvalue.real()),
            (lvalue, rvalue) => Integer(lvalue.int() - rvalue.int()),
        }
    }
}

impl std::ops::Div for Value {
    type Output = Option<Value>;

    fn div(self, rhs: Self) -> Self::Output {
        match rhs.real() {
            rvalue if rvalue == 0. => None,
            rvalue => Some(Value::Real(self.real() / rvalue)),
        }
    }
}

impl std::ops::Neg for Value {
    type Output = Value;

    fn neg(self) -> Self::Output {
        use Value::*;

        match self {
            Natural(nat) => Integer(-nat),
            Integer(int) => Integer(-int),
            Real(real) => Real(-real),
            _ => unreachable!(),
        }
    }
}

impl std::ops::Not for Value {
    type Output = Value;

    fn not(self) -> Self::Output {
        use Value::*;

        match self {
            Bool(value) => Bool(!value),
            _ => unreachable!(),
        }
    }
}

impl std::ops::BitAnd for Value {
    type Output = Value;

    fn bitand(self, rhs: Self) -> Self::Output {
        use Value::*;

        match (self, rhs) {
            (Bool(lbool), Bool(rbool)) => Bool(lbool && rbool),
            _ => unreachable!(),
        }
    }
}

impl std::ops::BitOr for Value {
    type Output = Value;

    fn bitor(self, rhs: Self) -> Self::Output {
        use Value::*;

        match (self, rhs) {
            (Bool(lbool), Bool(rbool)) => Bool(lbool || rbool),
            _ => unreachable!(),
        }
    }
}

impl std::cmp::PartialEq for Value {
    fn eq(&self, other: &Self) -> bool {
        use Value::*;

        match (self, other) {
            (Unit, Unit) => true,
            (Function { .. }, Function { .. }) => false,
            (Bool(lbool), Bool(rbool)) => lbool == rbool,
            // TODO: Ideally get rid of clones
            any!(Real, lvalue, rvalue) => lvalue.clone().real() == rvalue.clone().real(),
            // TODO: Ideally get rid of clones
            any!(Integer, lvalue, rvalue) => lvalue.clone().int() == rvalue.clone().int(),
            (Natural(lnat), Natural(rnat)) => lnat == rnat,
            _ => unreachable!(),
        }
    }
}

impl std::cmp::Eq for Value {}

impl std::cmp::Ord for Value {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        use Value::*;

        match (self, other) {
            // TODO: Ideally get rid of clones
            any!(Real, lvalue, rvalue) => lvalue.clone().real().total_cmp(&rvalue.clone().real()),
            // TODO: Ideally get rid of clones
            any!(Integer, lvalue, rvalue) => lvalue.clone().int().cmp(&rvalue.clone().int()),
            (Natural(lnat), Natural(rnat)) => lnat.cmp(rnat),
            _ => unreachable!(),
        }
    }
}

impl std::cmp::PartialOrd for Value {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

#[derive(Clone)]
pub enum NaturalValue {
    Small(Nat),
    Big(apnum::BigNat),
}

impl std::fmt::Display for NaturalValue {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use NaturalValue::*;

        match self {
            Small(nat) => write!(f, "{nat}"),
            Big(nat) => write!(f, "{nat}"),
        }
    }
}

impl std::ops::Add for NaturalValue {
    type Output = NaturalValue;

    fn add(self, rhs: Self) -> Self::Output {
        use NaturalValue::*;

        match (self, rhs) {
            (Small(lint), Small(rint)) => lint
                .checked_add(rint)
                .map(Small)
                .unwrap_or_else(|| Big(BigNat::from(lint) + BigNat::from(rint))),
            (Small(small), Big(big)) | (Big(big), Small(small)) => Big(BigNat::from(small) + big),
            (Big(lbig), Big(rbig)) => Big(lbig + rbig),
        }
    }
}

impl std::ops::Mul for NaturalValue {
    type Output = NaturalValue;

    fn mul(self, rhs: Self) -> Self::Output {
        use NaturalValue::*;

        match (self, rhs) {
            (Small(lint), Small(rint)) => lint
                .checked_mul(rint)
                .map(Small)
                .unwrap_or_else(|| Big(BigNat::from(lint) * BigNat::from(rint))),
            (Small(small), Big(big)) | (Big(big), Small(small)) => Big(BigNat::from(small) * big),
            (Big(lbig), Big(rbig)) => Big(lbig * rbig),
        }
    }
}

impl std::ops::Neg for NaturalValue {
    type Output = IntegerValue;

    fn neg(self) -> Self::Output {
        use IntegerValue as iv;
        use NaturalValue::*;

        match self {
            Small(nat) => match Int::try_from(nat) {
                Ok(int) => -iv::Small(int),
                Err(_) => iv::Big(-BigInt::from(nat)),
            },
            Big(int) => iv::Big(-BigInt::from(int)),
        }
    }
}

impl std::cmp::PartialEq for NaturalValue {
    fn eq(&self, other: &Self) -> bool {
        use NaturalValue::*;

        match (self, other) {
            (Small(lsmall), Small(rsmall)) => lsmall == rsmall,
            // TODO: Consider doing comparison directly, without creating a new big num from small num
            (Small(small), Big(big)) | (Big(big), Small(small)) => &BigNat::from(*small) == big,
            (Big(lbig), Big(rbig)) => lbig == rbig,
        }
    }
}

impl std::cmp::Eq for NaturalValue {}

impl std::cmp::Ord for NaturalValue {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        use NaturalValue::*;

        match (self, other) {
            (Small(lsmall), Small(rsmall)) => lsmall.cmp(rsmall),
            (Small(small), Big(big)) => BigNat::from(*small).cmp(big),
            (Big(big), Small(small)) => big.cmp(&BigNat::from(*small)),
            (Big(lbig), Big(rbig)) => lbig.cmp(rbig),
        }
    }
}

impl std::cmp::PartialOrd for NaturalValue {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

#[derive(Clone)]
pub enum IntegerValue {
    Small(Int),
    Big(apnum::BigInt),
}

impl std::fmt::Display for IntegerValue {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use IntegerValue::*;

        match self {
            Small(nat) => write!(f, "{nat}"),
            Big(nat) => write!(f, "{nat}"),
        }
    }
}

impl std::ops::Add for IntegerValue {
    type Output = IntegerValue;

    fn add(self, rhs: Self) -> Self::Output {
        use IntegerValue::*;

        match (self, rhs) {
            (Small(lint), Small(rint)) => lint
                .checked_add(rint)
                .map(Small)
                .unwrap_or_else(|| Big(BigInt::from(lint) + BigInt::from(rint))),
            (Small(small), Big(big)) | (Big(big), Small(small)) => Big(BigInt::from(small) + big),
            (Big(lbig), Big(rbig)) => Big(lbig + rbig),
        }
    }
}

impl std::ops::Mul for IntegerValue {
    type Output = IntegerValue;

    fn mul(self, rhs: Self) -> Self::Output {
        use IntegerValue::*;

        match (self, rhs) {
            (Small(lint), Small(rint)) => lint
                .checked_mul(rint)
                .map(Small)
                .unwrap_or_else(|| Big(BigInt::from(lint) * BigInt::from(rint))),
            (Small(small), Big(big)) | (Big(big), Small(small)) => Big(BigInt::from(small) * big),
            (Big(lbig), Big(rbig)) => Big(lbig * rbig),
        }
    }
}

impl std::ops::Sub for IntegerValue {
    type Output = IntegerValue;

    fn sub(self, rhs: Self) -> Self::Output {
        use IntegerValue::*;

        match (self, rhs) {
            (Small(lint), Small(rint)) => lint
                .checked_sub(rint)
                .map(Small)
                .unwrap_or_else(|| Big(BigInt::from(lint) - BigInt::from(rint))),
            (Small(small), Big(big)) => Big(BigInt::from(small) - big),
            (Big(big), Small(small)) => Big(big - BigInt::from(small)),
            (Big(lbig), Big(rbig)) => Big(lbig - rbig),
        }
    }
}

impl std::ops::Neg for IntegerValue {
    type Output = IntegerValue;

    fn neg(self) -> Self::Output {
        use IntegerValue::*;

        match self {
            Small(int) => match int.checked_neg() {
                Some(int) => Small(int),
                None => Big(-BigInt::from(int)),
            },
            Big(int) => Big(-int),
        }
    }
}

impl std::cmp::PartialEq for IntegerValue {
    fn eq(&self, other: &Self) -> bool {
        use IntegerValue::*;

        match (self, other) {
            (Small(lsmall), Small(rsmall)) => lsmall == rsmall,
            // TODO: Consider doing comparison directly, without creating a new big num from small num
            (Small(small), Big(big)) | (Big(big), Small(small)) => &BigInt::from(*small) == big,
            (Big(lbig), Big(rbig)) => lbig == rbig,
        }
    }
}

impl std::cmp::Eq for IntegerValue {}

impl std::cmp::Ord for IntegerValue {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        use IntegerValue::*;

        match (self, other) {
            (Small(lsmall), Small(rsmall)) => lsmall.cmp(rsmall),
            (Small(small), Big(big)) => BigInt::from(*small).cmp(big),
            (Big(big), Small(small)) => big.cmp(&BigInt::from(*small)),
            (Big(lbig), Big(rbig)) => lbig.cmp(rbig),
        }
    }
}

impl std::cmp::PartialOrd for IntegerValue {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}
