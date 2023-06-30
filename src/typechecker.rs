use crate::{
    common::*,
    parser::{BinaryOp, Expression, TopLevel, TypeExpr, UnaryOp},
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
            UnitValue => Type::Unit,
            Binary { op, left, right } => self.verify_binary_op(op, left, right)?,
            Unary { op, operand } => self.verify_unary_op(op, operand)?,
            Let {
                name,
                type_annot,
                value,
                expr,
            } => self.verify_let_expr(name, type_annot, value, expr)?,
            Fun {
                name,
                args,
                return_type,
                expr,
                in_expr,
            } => self.verify_fun_expr(name, args, return_type, expr, in_expr)?,
            FunctionCall { f, args } => self.verify_function_call(f, args)?,
            If {
                condition,
                true_expr,
                false_expr,
            } => self.verify_if_expr(condition, true_expr, false_expr)?,
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
                ltype.is_compatable_with(&rtype).ok_or_else(|| {
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
            Sequence => rtype,
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
        let vtype = if let Some(type_annot) = type_annot {
            let annotated_type = Self::eval_type_e(type_annot)?;
            Self::expect_type(&self.verify_type(value)?, &annotated_type)
                .map_err(|err| err.spanned(value.span.clone()))?;
            annotated_type
        } else {
            self.verify_type(value)?
        };

        self.env.define_local(name.data.clone(), vtype);
        let texpr = self.verify_type(expr)?;
        self.env.shallow(1);

        Ok(texpr)
    }

    fn verify_fun_expr(
        &mut self,
        name: &Spanned<Symbol>,
        args: &Vec<(Spanned<Symbol>, Spanned<TypeExpr>)>,
        return_type: &Option<Spanned<TypeExpr>>,
        expr: &Spanned<Expression>,
        in_expr: &Spanned<Expression>,
    ) -> TypeCheckResult {
        let arg_types = args
            .iter()
            .map(|(_, type_annot)| Self::eval_type_e(type_annot))
            .collect::<Result<Vec<Type>, _>>()?;

        let return_type = Box::new(match return_type {
            Some(return_type) => Self::eval_type_e(return_type)?,
            _ => Type::Unit,
        });

        let t = Type::Function {
            arg_types: arg_types.clone(),
            return_type: return_type.clone(),
        };

        self.env.define_local(name.data.clone(), t);
        for (index, arg_type) in arg_types.into_iter().enumerate() {
            self.env.define_local(args[index].0.data.clone(), arg_type);
        }

        let ftype = self.verify_type(expr)?;
        Self::expect_type(&ftype, &return_type).map_err(|_| {
            TypeCheckError::UnexpectedReturnType {
                function_name: name.data.clone(),
                found: ftype,
                expected: *return_type,
            }
            .spanned(name.span.clone())
        })?;

        self.env.shallow(args.len());
        let result = self.verify_type(in_expr)?;
        self.env.shallow(1);
        Ok(result)
    }

    fn verify_if_expr(
        &mut self,
        condition: &Spanned<Expression>,
        true_expr: &Spanned<Expression>,
        false_expr: &Option<Box<Spanned<Expression>>>,
    ) -> TypeCheckResult {
        Self::expect_type(&self.verify_type(condition)?, &Type::Bool)
            .map_err(|err| err.spanned(condition.span.clone()))?;

        let ttrue = self.verify_type(true_expr)?;
        let tfalse = match false_expr {
            Some(false_expr) => self.verify_type(false_expr)?,
            None => Type::Unit,
        };

        ttrue.is_compatable_with(&tfalse).ok_or_else(|| {
            TypeCheckError::DifferentTypedBranches {
                true_branch: ttrue.clone(),
                false_branch: tfalse.clone(),
            }
            // TODO: fix span
            .spanned(condition.span.clone())
        })
    }

    fn verify_function_call(
        &mut self,
        f: &Spanned<Expression>,
        args: &[Spanned<Expression>],
    ) -> TypeCheckResult {
        let t = self.verify_type(f)?;
        let Type::Function { arg_types, return_type } = t else {
            return Err(TypeCheckError::UncallableType(t).spanned(f.span.clone()))
        };

        let true = args.len() == arg_types.len() else {
            return Err(TypeCheckError::ArgumentNumberMismatch { found: args.len(), expected: arg_types.len() }.spanned(f.span.clone()))
        };

        for (arg, expected) in std::iter::zip(args, arg_types) {
            let arg_type = self.verify_type(arg)?;
            Self::expect_type(&arg_type, &expected).map_err(|err| err.spanned(arg.span.clone()))?;
        }

        Ok(*return_type)
    }

    pub fn verify_top_level(&mut self, definitions: &Vec<Spanned<TopLevel>>) -> TypeCheckResult {
        use TopLevel::*;

        // First pass - Collect definitions
        for definition in definitions {
            #[allow(clippy::single_match)]
            match &definition.data {
                Fun {
                    name,
                    args,
                    return_type,
                    expr: _,
                } => {
                    let arg_types = args
                        .iter()
                        .map(|(_, type_annot)| Self::eval_type_e(type_annot))
                        .collect::<Result<Vec<Type>, _>>()?;

                    let return_type = Box::new(match return_type {
                        Some(return_type) => Self::eval_type_e(return_type)?,
                        _ => Type::Unit,
                    });

                    self.env.define_global(
                        name.data.clone(),
                        Type::Function {
                            arg_types,
                            return_type,
                        },
                    )
                }
                _ => (),
            }
        }

        // Second pass - Verify
        for definition in definitions {
            match &definition.data {
                Let {
                    name,
                    type_annot,
                    value,
                } => {
                    let vtype = if let Some(type_annot) = type_annot {
                        let annotated_type = Self::eval_type_e(type_annot)?;
                        Self::expect_type(&self.verify_type(value)?, &annotated_type)
                            .map_err(|err| err.spanned(value.span.clone()))?;
                        annotated_type
                    } else {
                        self.verify_type(value)?
                    };
                    self.env.define_global(name.data.clone(), vtype);
                }
                Fun {
                    name, args, expr, ..
                } => {
                    let t = self.env.resolve_global(&name.data).unwrap().clone();
                    let Type::Function { arg_types, return_type } = t.clone() else {
                        unreachable!()
                    };

                    self.env.define_local(name.data.clone(), t);
                    for (index, arg_type) in arg_types.into_iter().enumerate() {
                        self.env.define_local(args[index].0.data.clone(), arg_type);
                    }

                    let ftype = self.verify_type(expr)?;
                    Self::expect_type(&ftype, &return_type).map_err(|_| {
                        TypeCheckError::UnexpectedReturnType {
                            function_name: name.data.clone(),
                            found: ftype,
                            expected: *return_type,
                        }
                        .spanned(name.span.clone())
                    })?;
                    self.env.shallow(args.len() + 1);
                }
            }
        }

        let entry_point = self
            .env
            .resolve_global("main")
            .ok_or_else(|| TypeCheckError::EntryPointNotProvided.spanned(0..0))?;

        let Type::Function { arg_types: _, return_type } = entry_point else {
            return Err(TypeCheckError::EntryPointNotProvided.spanned(0..0));
        };

        Ok(*return_type.clone())
    }

    fn expect_type(t: &Type, expected: &Type) -> Result<(), TypeCheckError> {
        if !t.is_subtype_of(expected) {
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
            UnitValue => Type::Unit,
            Function {
                arg_types,
                return_type,
            } => {
                let arg_types = arg_types
                    .iter()
                    .map(Self::eval_type_e)
                    .collect::<Result<Vec<_>, _>>()?;
                let return_type = Box::new(match return_type {
                    Some(return_type) => Self::eval_type_e(return_type)?,
                    None => Type::Unit,
                });

                Type::Function {
                    arg_types,
                    return_type,
                }
            }
        })
    }
}

type TypeCheckResult = Result<Type, Spanned<TypeCheckError>>;

#[derive(Debug)]
pub enum TypeCheckError {
    ExpectedNumericType(Type),
    MismatchedType {
        found: Type,
        expected: Type,
    },
    EqualityCheckOfDifferentTypes {
        left: Type,
        right: Type,
    },
    UndefinedTypeName(Symbol),
    UndefinedIdentifier(Symbol),
    UnexpectedReturnType {
        function_name: Symbol,
        found: Type,
        expected: Type,
    },
    EntryPointNotProvided,
    UncallableType(Type),
    ArgumentNumberMismatch {
        found: usize,
        expected: usize,
    },
    DifferentTypedBranches {
        true_branch: Type,
        false_branch: Type,
    },
}

impl HasSpan for TypeCheckError {}

impl Error for TypeCheckError {
    fn message(&self) -> String {
        use TypeCheckError::*;

        match self {
            ExpectedNumericType(t) => format!("Expected an expression type one of `Natural`, `Integer`, or `Real` instead found `{t}`"),
            MismatchedType { found, expected } => format!("Expected an expression type of `{expected}` instead found `{found}`"),
            EqualityCheckOfDifferentTypes { left, right } => format!("Cannot determine the equality of different types: `{left}` == `{right}` "),
            UndefinedTypeName(name) => format!("A type named `{name}` does not exist."),
            UndefinedIdentifier(name) => format!("Identifier `{name}` was never defined."),
            UnexpectedReturnType { function_name, found, expected } => format!("Function {function_name} expected to return `{expected}` instead found `{found}`"),
            EntryPointNotProvided => "Entry point (main) for the program is not provided".to_string(),
            UncallableType(t) => format!("Cannot call a `{t}`, only functions are callable"),
            ArgumentNumberMismatch { found, expected } => format!("Expected `{expected}` number of arguments instead found `{found}` number of arguments"),
            DifferentTypedBranches { true_branch, false_branch } => format!("Branches of if expression have uncompatable types: `{true_branch}` and `{false_branch}`"),
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
pub enum Type {
    Natural,
    Integer,
    Real,
    Bool,
    Function {
        arg_types: Vec<Type>,
        return_type: Box<Type>,
    },
    Unit,
}

impl Type {
    fn is_subtype_of(&self, super_type: &Type) -> bool {
        use Type::*;

        match self {
            Natural => matches!(super_type, Natural | Integer | Real),
            Integer => matches!(super_type, Integer | Real),
            Function {
                arg_types,
                return_type,
            } => {
                let Function { arg_types: sargs_types, return_type: sreturn_type } = super_type else {
                    return false;
                };

                arg_types.len() == sargs_types.len()
                    && std::iter::zip(arg_types, sargs_types)
                        .all(|(arg_type, sarg_type)| arg_type.is_subtype_of(sarg_type))
                    && return_type.is_subtype_of(sreturn_type)
            }
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

    fn is_compatable_with(&self, other: &Type) -> Option<Type> {
        if self.is_subtype_of(other) {
            Some(other.clone())
        } else if other.is_subtype_of(self) {
            Some(self.clone())
        } else {
            None
        }
    }
}

impl std::fmt::Display for Type {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use Type::*;

        match self {
            Natural => write!(f, "Natural"),
            Integer => write!(f, "Integer"),
            Real => write!(f, "Real"),
            Bool => write!(f, "Bool"),
            Function {
                arg_types,
                return_type,
            } => {
                write!(f, "fun(")?;
                for arg_type in arg_types {
                    write!(f, "{arg_type},")?;
                }
                write!(f, ")")?;
                write!(f, " -> ")?;
                write!(f, "{return_type}")
            }
            Unit => write!(f, "()"),
        }
    }
}
