use crate::{
    common::*,
    parser::{BinaryOp, Expression, UnaryOp},
};

pub struct TypeCheker {}

impl TypeCheker {
    pub fn new() -> Self {
        Self {}
    }

    pub fn verify_type(&self, e: &Spanned<Expression>) -> TypeCheckResult {
        use Expression::*;

        Ok(match &e.data {
            NaturalNumber(_) => Type::Natural,
            RealNumber(_) => Type::Real,
            BoolValue(_) => Type::Bool,
            Binary { op, left, right } => self.verify_binary_op(op, left, right)?,
            Unary { op, operand } => self.verify_unary_op(op, operand)?,
        })
    }

    fn verify_binary_op(
        &self,
        op: &BinaryOp,
        left: &Spanned<Expression>,
        right: &Spanned<Expression>,
    ) -> TypeCheckResult {
        use BinaryOp::*;
        use Type::*;

        let ltype = self.verify_type(left)?;
        let rtype = self.verify_type(right)?;

        match op {
            Addition | Multiplication | Subtraction | Division => {
                Self::expect_numeric(&ltype).map_err(|err| err.spanned(left.span.clone()))?;
                Self::expect_numeric(&rtype).map_err(|err| err.spanned(right.span.clone()))?;

                // Subtyping relation between numeric types is total.
                let super_type = if ltype.is_subtype_of(&rtype) {
                    rtype
                } else {
                    ltype
                };

                Ok(match op {
                    Subtraction => super_type.minimum_type(Integer),
                    Division => super_type.minimum_type(Real),
                    _ => super_type,
                })
            }
            And | Or => {
                Self::expect_type(&ltype, &Bool).map_err(|err| err.spanned(left.span.clone()))?;
                Self::expect_type(&rtype, &Bool).map_err(|err| err.spanned(right.span.clone()))?;
                Ok(Bool)
            }
        }
    }

    fn verify_unary_op(&self, op: &UnaryOp, operand: &Spanned<Expression>) -> TypeCheckResult {
        use UnaryOp::*;

        let otype = self.verify_type(operand)?;

        match op {
            Negation => Ok(match otype {
                Type::Natural | Type::Integer => Type::Integer,
                Type::Real => Type::Real,
                _ => {
                    return Err(
                        TypeCheckError::ExpectedNumericType(otype).spanned(operand.span.clone())
                    )
                }
            }),
        }
    }

    fn expect_type(t: &Type, expected: &Type) -> Result<(), TypeCheckError> {
        if t != expected {
            return Err(TypeCheckError::MismatchedType {
                found: t.clone(),
                expected: expected.clone(),
            });
        };
        Ok(())
    }

    fn expect_numeric(t: &Type) -> Result<(), TypeCheckError> {
        if !t.is_numeric() {
            return Err(TypeCheckError::ExpectedNumericType(t.clone()));
        };
        Ok(())
    }
}

type TypeCheckResult = Result<Type, Spanned<TypeCheckError>>;

#[derive(Debug)]
pub enum TypeCheckError {
    ExpectedNumericType(Type),
    MismatchedType { found: Type, expected: Type },
}

impl HasSpan for TypeCheckError {}

impl Error for TypeCheckError {
    fn message(&self) -> String {
        use TypeCheckError::*;

        match self {
            ExpectedNumericType(t) => format!("Expected an expression type one of `Natural`, `Integer`, or `Real` instead found `{t:?}`"),
            MismatchedType { found, expected } => format!("Expected an expression type of `{expected:?}` instead found `{found:?}`"),
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
pub enum Type {
    Natural,
    Integer,
    Real,
    Bool,
}

impl Type {
    fn is_subtype_of(&self, super_type: &Type) -> bool {
        use Type::*;

        match self {
            Natural => matches!(super_type, Natural | Integer | Real),
            Integer => matches!(super_type, Integer | Real),
            candidate => candidate == super_type,
        }
    }

    fn is_numeric(&self) -> bool {
        use Type::*;

        matches!(self, Natural | Integer | Real)
    }

    fn minimum_type(&self, ty: Type) -> Type {
        if ty.is_subtype_of(self) {
            self.clone()
        } else {
            ty
        }
    }
}
