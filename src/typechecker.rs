use crate::{
    common::*,
    parser::{BinaryOp, Expression},
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
            Binary { op, left, right } => self.verify_binary_op(op, left, right)?,
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
            Addition | Multiplication => {
                let true = ltype.is_numeric() else {
                    return Err(TypeCheckError::ExpectedNumericType(ltype).spanned(left.span.clone()));
                };

                let true = rtype.is_numeric() else {
                    return Err(TypeCheckError::ExpectedNumericType(rtype).spanned(right.span.clone()));
                };

                let super_type = if ltype.is_subtype_of(&rtype) {
                    rtype
                } else if rtype.is_subtype_of(&ltype) {
                    ltype
                } else {
                    // Subtyping relation between numeric types is total.
                    unreachable!();
                };

                Ok(super_type)
            }
        }
    }
}

type TypeCheckResult = Result<Type, Spanned<TypeCheckError>>;

#[derive(Debug)]
pub enum TypeCheckError {
    ExpectedNumericType(Type),
}

impl HasSpan for TypeCheckError {}

#[derive(Debug, PartialEq)]
pub enum Type {
    Natural,
    Integer,
    Real,
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

        match self {
            Natural | Integer | Real => true,
        }
    }
}
