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

        let ltype = self.verify_type(left)?;
        let rtype = self.verify_type(right)?;

        match op {
            Addition | Multiplication | Subtraction | Division => {
                let true = ltype.is_numeric() else {
                    return Err(TypeCheckError::ExpectedNumericType(ltype).spanned(left.span.clone()));
                };

                let true = rtype.is_numeric() else {
                    return Err(TypeCheckError::ExpectedNumericType(rtype).spanned(right.span.clone()));
                };

                // Subtyping relation between numeric types is total.
                let super_type = if ltype.is_subtype_of(&rtype) {
                    rtype
                } else {
                    ltype
                };

                Ok(if let Subtraction = op {
                    super_type.minimum_type(Type::Integer)
                } else if let Division = op {
                    super_type.minimum_type(Type::Real)
                } else {
                    super_type
                })
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
                #[allow(unreachable_patterns)]
                _ => {
                    return Err(
                        TypeCheckError::ExpectedNumericType(otype).spanned(operand.span.clone())
                    )
                }
            }),
        }
    }
}

type TypeCheckResult = Result<Type, Spanned<TypeCheckError>>;

#[derive(Debug)]
pub enum TypeCheckError {
    ExpectedNumericType(Type),
}

impl HasSpan for TypeCheckError {}

impl Error for TypeCheckError {
    fn message(&self) -> String {
        use TypeCheckError::*;

        match self {
            ExpectedNumericType(t) => format!("Expected an expression type one of `Natural`, `Integer`, or `Real` instead found `{t:?}`"),
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
