use crate::{
    common::*,
    parser::{BinaryOp, Expression, TypeExpr, UnaryOp},
};

pub struct TypeCheker {
    env: Environment<Type>,
}

impl TypeCheker {
    pub fn new() -> Self {
        Self {
            env: Environment::new(),
        }
    }

    pub fn verify_type(&mut self, e: &Spanned<Expression>) -> TypeCheckResult {
        use Expression::*;

        Ok(match &e.data {
            Identifier(symbol) => match self.env.resolve(symbol) {
                Some(ty) => ty.clone(),
                None => {
                    return Err(
                        TypeCheckError::UndefinedIdentifier(symbol.clone()).spanned(e.span.clone())
                    )
                }
            },
            NaturalNumber(_) => Type::Natural,
            RealNumber(_) => Type::Real,
            BoolValue(_) => Type::Bool,
            Binary { op, left, right } => self.verify_binary_op(op, left, right)?,
            Unary { op, operand } => self.verify_unary_op(op, operand)?,
            Let {
                name,
                type_annot,
                value,
                expr,
            } => self.verify_let_expr(name, type_annot, value, expr)?,
        })
    }

    fn verify_binary_op(
        &mut self,
        op: &BinaryOp,
        left: &Spanned<Expression>,
        right: &Spanned<Expression>,
    ) -> TypeCheckResult {
        use BinaryOp::*;
        use Type::*;

        let ltype = self.verify_type(left)?;
        let rtype = self.verify_type(right)?;

        Ok(match op {
            Addition | Multiplication | Subtraction | Division => {
                Self::expect_numeric(&ltype).map_err(|err| err.spanned(left.span.clone()))?;
                Self::expect_numeric(&rtype).map_err(|err| err.spanned(right.span.clone()))?;

                // Subtyping relation between numeric types is total.
                let super_type = if ltype.is_subtype_of(&rtype) {
                    rtype
                } else {
                    ltype
                };

                match op {
                    Subtraction => super_type.minimum_type(Integer),
                    Division => super_type.minimum_type(Real),
                    _ => super_type,
                }
            }
            And | Or => {
                Self::expect_type(&ltype, &Bool).map_err(|err| err.spanned(left.span.clone()))?;
                Self::expect_type(&rtype, &Bool).map_err(|err| err.spanned(right.span.clone()))?;
                Bool
            }
            Equal | NotEqual => {
                Self::expect_type(&ltype, &rtype).map_err(|_| {
                    TypeCheckError::EqualityCheckOfDifferentTypes {
                        left: ltype,
                        right: rtype,
                    }
                    .start_end(left.span.clone(), right.span.clone())
                })?;
                Bool
            }
            Less | LessEqual | Greater | GreaterEqual => {
                Self::expect_numeric(&ltype).map_err(|err| err.spanned(left.span.clone()))?;
                Self::expect_numeric(&rtype).map_err(|err| err.spanned(right.span.clone()))?;
                Bool
            }
        })
    }

    fn verify_unary_op(&mut self, op: &UnaryOp, operand: &Spanned<Expression>) -> TypeCheckResult {
        use Type::*;
        use UnaryOp::*;

        let otype = self.verify_type(operand)?;

        Ok(match op {
            Negation => match otype {
                Natural | Integer => Integer,
                Real => Real,
                _ => {
                    return Err(
                        TypeCheckError::ExpectedNumericType(otype).spanned(operand.span.clone())
                    )
                }
            },
            Not => {
                Self::expect_type(&otype, &Bool)
                    .map_err(|err| err.spanned(operand.span.clone()))?;
                Bool
            }
        })
    }

    fn verify_let_expr(
        &mut self,
        name: &Spanned<Symbol>,
        type_annot: &Option<Spanned<TypeExpr>>,
        value: &Spanned<Expression>,
        expr: &Spanned<Expression>,
    ) -> TypeCheckResult {
        let vtype = self.verify_type(value)?;

        let vtype = if let Some(type_annot) = type_annot {
            let annotated_type = Self::eval_type_e(type_annot)?;
            Self::expect_type(&vtype, &annotated_type)
                .map_err(|err| err.spanned(value.span.clone()))?;
            annotated_type
        } else {
            vtype
        };

        self.env.define(name.data.clone(), vtype);
        let texpr = self.verify_type(expr)?;
        self.env.shallow();

        Ok(texpr)
    }

    fn expect_type(t: &Type, expected: &Type) -> Result<(), TypeCheckError> {
        if expected.is_numeric() {
            if t.is_subtype_of(expected) {
                return Ok(());
            } else {
                return Err(TypeCheckError::MismatchedType {
                    found: t.clone(),
                    expected: expected.clone(),
                });
            }
        }

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

    fn eval_type_e(e: &Spanned<TypeExpr>) -> TypeCheckResult {
        use TypeExpr::*;

        Ok(match &e.data {
            Identifier(symbol) => match &**symbol {
                "Nat" => Type::Natural,
                "Int" => Type::Integer,
                "Bool" => Type::Bool,
                "Real" => Type::Real,
                _ => {
                    return Err(
                        TypeCheckError::UndefinedTypeName(symbol.clone()).spanned(e.span.clone())
                    )
                }
            },
        })
    }
}

type TypeCheckResult = Result<Type, Spanned<TypeCheckError>>;

#[derive(Debug)]
pub enum TypeCheckError {
    ExpectedNumericType(Type),
    MismatchedType { found: Type, expected: Type },
    EqualityCheckOfDifferentTypes { left: Type, right: Type },
    UndefinedTypeName(Symbol),
    UndefinedIdentifier(Symbol),
}

impl HasSpan for TypeCheckError {}

impl Error for TypeCheckError {
    fn message(&self) -> String {
        use TypeCheckError::*;

        match self {
            ExpectedNumericType(t) => format!("Expected an expression type one of `Natural`, `Integer`, or `Real` instead found `{t:?}`"),
            MismatchedType { found, expected } => format!("Expected an expression type of `{expected:?}` instead found `{found:?}`"),
            EqualityCheckOfDifferentTypes { left, right } => format!("Cannot determine the equality of different types: `{left:?}` == `{right:?}` "),
            UndefinedTypeName(name) => format!("A type named `{name}` does not exist."),
            UndefinedIdentifier(name) => format!("Identifier `{name}` was never defined."),
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
