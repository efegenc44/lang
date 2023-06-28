use std::num::IntErrorKind;

use apnum::{self, BigInt, BigNat};

use crate::{
    common::*,
    parser::{BinaryOp, Expression, UnaryOp},
};

pub struct Engine {}

impl Engine {
    pub fn new() -> Self {
        Self {}
    }

    pub fn evaluate(&self, e: &Spanned<Expression>) -> EvaluationResult {
        use Expression::*;

        Ok(match &e.data {
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
            Binary { op, left, right } => self.evaluate_binary_op(op, left, right)?,
            Unary { op, operand } => self.evaluate_unary_op(op, operand)?,
        })
    }

    fn evaluate_binary_op(
        &self,
        op: &BinaryOp,
        left: &Spanned<Expression>,
        right: &Spanned<Expression>,
    ) -> EvaluationResult {
        use BinaryOp::*;

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
        })
    }

    fn evaluate_unary_op(&self, op: &UnaryOp, operand: &Spanned<Expression>) -> EvaluationResult {
        use UnaryOp::*;

        let ovalue = self.evaluate(operand)?;

        Ok(match op {
            Negation => -ovalue,
            Not => !ovalue,
        })
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
}

impl std::fmt::Display for Value {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use Value::*;

        match self {
            Natural(nat) => write!(f, "{nat}"),
            Integer(int) => write!(f, "{int}"),
            Real(real) => write!(f, "{real}"),
            Bool(value) => write!(f, "{value}"),
        }
    }
}

impl Value {
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
        use IntegerValue as iv;
        use NaturalValue as nv;
        use Value::*;

        match (self, other) {
            (Natural(lnat), Natural(rnat)) => match (lnat, rnat) {
                (nv::Small(lsmall), nv::Small(rsmall)) => lsmall == rsmall,
                // TODO: Consider doing comparison directly, without creating a new big num from small num
                (nv::Small(small), nv::Big(big)) | (nv::Big(big), nv::Small(small)) => {
                    &BigNat::from(*small) == big
                }
                (nv::Big(lbig), nv::Big(rbig)) => lbig == rbig,
            },
            (Integer(lint), Integer(rint)) => match (lint, rint) {
                (iv::Small(lsmall), iv::Small(rsmall)) => lsmall == rsmall,
                // TODO: Consider doing comparison directly, without creating a new big num from small num
                (iv::Small(small), iv::Big(big)) | (iv::Big(big), iv::Small(small)) => {
                    &BigInt::from(*small) == big
                }
                (iv::Big(lbig), iv::Big(rbig)) => lbig == rbig,
            },
            (Real(lreal), Real(rreal)) => lreal == rreal,
            (Bool(lbool), Bool(rbool)) => lbool == rbool,
            _ => unreachable!(),
        }
    }
}

impl std::cmp::Eq for Value {}

impl std::cmp::Ord for Value {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        use IntegerValue as iv;
        use NaturalValue as nv;
        use Value::*;

        match (self, other) {
            // TODO: Ideally get rid of clones
            any!(Real, lvalue, rvalue) => lvalue.clone().real().total_cmp(&rvalue.clone().real()),
            // TODO: Ideally get rid of clones
            any!(Integer, lvalue, rvalue) => match (lvalue.clone().int(), rvalue.clone().int()) {
                (iv::Small(lsmall), iv::Small(rsmall)) => lsmall.cmp(&rsmall),
                (iv::Small(small), iv::Big(big)) => BigInt::from(small).cmp(&big),
                (iv::Big(big), iv::Small(small)) => big.cmp(&BigInt::from(small)),
                (iv::Big(lbig), iv::Big(rbig)) => lbig.cmp(&rbig),
            },
            (Natural(lnat), Natural(rnat)) => match (lnat, rnat) {
                (nv::Small(lsmall), nv::Small(rsmall)) => lsmall.cmp(rsmall),
                (nv::Small(small), nv::Big(big)) => BigNat::from(*small).cmp(big),
                (nv::Big(big), nv::Small(small)) => big.cmp(&BigNat::from(*small)),
                (nv::Big(lbig), nv::Big(rbig)) => lbig.cmp(rbig),
            },
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
