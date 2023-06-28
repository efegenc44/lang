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
        })
    }

    fn evaluate_unary_op(&self, op: &UnaryOp, operand: &Spanned<Expression>) -> EvaluationResult {
        use UnaryOp::*;

        let ovalue = self.evaluate(operand)?;

        Ok(match op {
            Negation => -ovalue,
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

pub enum Value {
    Natural(NaturalValue),
    Integer(IntegerValue),
    Real(Real),
}

impl std::fmt::Display for Value {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use Value::*;

        match self {
            Natural(nat) => write!(f, "{nat}"),
            Integer(int) => write!(f, "{int}"),
            Real(real) => write!(f, "{real}"),
        }
    }
}

impl Value {
    fn real(self) -> Real {
        use Value::*;
        use NaturalValue as nv;
        use IntegerValue as iv;

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
        }
    }

    fn int(self) -> IntegerValue {
        use Value::*;
        use NaturalValue as nv;
        use IntegerValue as iv;

        match self {
            Natural(nat) => match nat {
                nv::Small(nat) => match Int::try_from(nat) {
                    Ok(int) => iv::Small(int),
                    Err(_) => iv::Big(BigInt::from(nat)),
                },
                nv::Big(nat) => iv::Big(nat.into()),
            },
            Integer(int) => int,
            Real(_) => unreachable!(),
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
        }
    }
}

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
