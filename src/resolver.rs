use std::{collections::HashSet, vec};

use crate::{
    bound::{Bound, Path},
    declaration::{Declaration, Method, Module, TypedIdentifier, VariantCase},
    expression::{Expression, TypeExpression},
    interner::{InternIdx, Interner},
    location::{Located, SourceLocation},
    reportable::{Reportable, ReportableResult},
    statement::{MatchBranch, Pattern, Statement},
};

pub struct Resolver {
    modules: HashSet<InternIdx>,

    variants: HashSet<Path>,
    procedures: HashSet<Path>,

    locals: Vec<InternIdx>,

    current_imports: HashSet<InternIdx>,
    current_path: Path,
    current_source: String,
}

impl Resolver {
    pub fn new() -> Self {
        Self {
            variants: HashSet::new(),
            procedures: HashSet::new(),
            locals: vec![],

            modules: HashSet::new(),

            current_imports: HashSet::new(),
            current_path: Path::empty(),
            current_source: String::new(),
        }
    }

    pub fn resolve(&mut self, mut modules: Vec<Module>) -> ReportableResult<Vec<Module>> {
        for module in &mut modules {
            self.collect_names(module)?;
        }

        for module in &mut modules {
            // TODO: this is a bit unnecessary
            self.which_module(module)?;
            self.module(module)?;
        }

        Ok(modules)
    }

    fn module(&mut self, module: &mut Module) -> ReportableResult<()> {
        for declaration in module.declarations_mut() {
            self.declaration(declaration)?;
        }

        Ok(())
    }

    fn which_module(&mut self, module: &Module) -> ReportableResult<()> {
        self.current_imports.clear();
        self.current_path = Path::empty();
        self.current_source = module.source().to_string();

        let mut declared = false;
        for declaration in module.declarations() {
            match declaration {
                Declaration::Module { name } => {
                    if !declared {
                        declared = true;
                        self.current_path.push(*name.data());
                        self.modules.insert(*name.data());
                    } else {
                        return self
                            .error(ResolveError::DuplicateModuleDeclaration, name.location());
                    }
                }
                Declaration::Import { name } => {
                    self.current_imports.insert(*name.data());
                }
                _ => (),
            }
        }

        if self.current_path.is_empty() {
            return self.error(ResolveError::ModuleIsNotDeclared, SourceLocation::dummy());
        }

        Ok(())
    }

    fn collect_names(&mut self, module: &mut Module) -> ReportableResult<()> {
        self.which_module(module)?;
        for declaration in module.declarations_mut() {
            match declaration {
                Declaration::Module { .. } => {}
                Declaration::Import { .. } => {}
                Declaration::Procedure { name, path, .. } => {
                    self.collect_procedure_name(name, path)?
                }
                Declaration::Variant {
                    name, cases, path, ..
                } => self.collect_variant_name(name, cases, path)?,
            }
        }

        Ok(())
    }

    fn collect_procedure_name(
        &mut self,
        name: &Located<InternIdx>,
        path: &mut Path,
    ) -> ReportableResult<()> {
        let procedure_path = self.current_path.append(*name.data());
        if !self.procedures.contains(&procedure_path) {
            self.procedures.insert(procedure_path.clone());
            *path = procedure_path;
        } else {
            return self.error(
                ResolveError::DuplicateProcedureDeclaration(procedure_path),
                name.location(),
            );
        }

        Ok(())
    }

    fn collect_variant_name(
        &mut self,
        name: &Located<InternIdx>,
        cases: &mut [Located<VariantCase>],
        path: &mut Path,
    ) -> ReportableResult<()> {
        let variant_path = self.current_path.append(*name.data());
        if self.variants.contains(&variant_path) {
            return self.error(
                ResolveError::DuplicateTypeDeclaration(variant_path),
                name.location(),
            );
        }

        self.current_path.push(*name.data());
        for case in cases.iter_mut() {
            let constructor = *case.data().identifier().data();
            let constructor_path = self.current_path.append(constructor);
            if !self.procedures.contains(&constructor_path) {
                self.procedures.insert(constructor_path.clone());
                *case.data_mut().path_mut() = constructor_path;
            } else {
                return self.error(
                    ResolveError::DuplicateConstructorDeclaration {
                        constructor,
                        variant_path,
                    },
                    case.data().identifier().location(),
                );
            }
        }
        self.current_path.pop();
        self.variants.insert(variant_path.clone());
        *path = variant_path;

        Ok(())
    }

    fn find_name(&self, intern_idx: &InternIdx) -> Option<Bound> {
        // Local Scope
        for (index, name_idx) in self.locals.iter().rev().enumerate() {
            if name_idx == intern_idx {
                return Some(Bound::local(index));
            }
        }

        if self.current_imports.contains(intern_idx) {
            Some(Bound::absolute(vec![*intern_idx]))
        } else {
            None
        }
    }

    fn find_type_name(&self, intern_idx: &InternIdx) -> Option<Bound> {
        // TODO: Local Scope (type variables)

        if self.current_imports.contains(intern_idx) {
            Some(Bound::absolute(vec![*intern_idx]))
        } else {
            None
        }
    }

    fn expression(&mut self, expression: &mut Located<Expression>) -> ReportableResult<()> {
        let location = expression.location();
        match expression.data_mut() {
            Expression::Path(parts, bound) => self.path(parts, bound, location)?,
        }

        Ok(())
    }

    fn path(
        &mut self,
        parts: &[InternIdx],
        bound: &mut Bound,
        location: SourceLocation,
    ) -> ReportableResult<()> {
        let base = self
            .find_name(&parts[0])
            .unwrap_or(Bound::Absolute(self.current_path.append(parts[0])));

        match base {
            Bound::Local(_) => {
                assert!(parts.len() == 1);
                *bound = base
            }
            Bound::Absolute(base_path) => {
                let path = base_path.append_parts(&parts[1..]);
                let Some(path) = self.procedures.get(&path) else {
                    return self.error(ResolveError::UnboundValuePath(path), location);
                };
                *bound = Bound::Absolute(path.clone());
            }
            Bound::Undetermined => unreachable!(),
        };

        Ok(())
    }

    fn type_expression(
        &mut self,
        type_expression: &mut Located<TypeExpression>,
    ) -> ReportableResult<()> {
        let location = type_expression.location();
        match type_expression.data_mut() {
            TypeExpression::Path(parts, bound) => self.type_path(parts, bound, location)?,
        }

        Ok(())
    }

    fn type_path(
        &mut self,
        parts: &[InternIdx],
        bound: &mut Bound,
        location: SourceLocation,
    ) -> ReportableResult<()> {
        let base = self
            .find_type_name(&parts[0])
            .unwrap_or(Bound::Absolute(self.current_path.append(parts[0])));

        match base {
            Bound::Local(_) => {
                assert!(parts.len() == 1);
                *bound = base
            }
            Bound::Absolute(base_path) => {
                let path = base_path.append_parts(&parts[1..]);
                let Some(path) = self.variants.get(&path) else {
                    return self.error(ResolveError::UnboundTypePath(path), location);
                };
                *bound = Bound::Absolute(path.clone());
            }
            Bound::Undetermined => unreachable!(),
        };

        Ok(())
    }

    fn statement(&mut self, statement: &mut Located<Statement>) -> ReportableResult<()> {
        match statement.data_mut() {
            Statement::Expression(expression) => self.expression(expression),
            Statement::Return(expression) => self.expression(expression),
            Statement::Match { expression, branches } => self.matc(expression, branches),
        }
    }


    fn matc(&mut self, expression: &mut Located<Expression>, branches: &mut [Located<MatchBranch>]) -> ReportableResult<()> {
        self.expression(expression)?;
        let locals_len = self.locals.len();

        for branch in branches {
            match branch.data().pattern.data() {
                Pattern::VariantCase { name: _, fields } => {
                    let fields = if let Some(fields) = fields {
                        fields
                    } else {
                        &vec![]
                    };

                    let fields = fields.iter().map(|field| *field.data());
                    self.locals.extend(fields);
                },
            }

            self.statement(&mut branch.data_mut().statement)?;
            self.locals.truncate(locals_len);
        }

        Ok(())
    }


    fn declaration(&mut self, declaration: &mut Declaration) -> ReportableResult<()> {
        // TODO: maybe take Located<Declaration> for better error reporting
        match declaration {
            Declaration::Module { .. } => {}
            Declaration::Import { name } => self.import(name)?,
            Declaration::Procedure {
                arguments,
                return_type,
                body,
                ..
            } => self.procedure(arguments, return_type, body)?,
            Declaration::Variant { cases, methods, .. } => self.variant(cases, methods)?,
        };

        Ok(())
    }

    fn import(&self, name: &Located<InternIdx>) -> ReportableResult<()> {
        if !self.modules.contains(name.data()) {
            return self.error(
                ResolveError::ModuleDoesNotExist(*name.data()),
                name.location(),
            );
        }

        Ok(())
    }

    fn procedure(
        &mut self,
        arguments: &mut [Located<TypedIdentifier>],
        return_type: &mut Located<TypeExpression>,
        body: &mut [Located<Statement>],
    ) -> ReportableResult<()> {
        for argument in arguments.iter_mut() {
            self.type_expression(argument.data_mut().type_expression_mut())?;
        }

        self.type_expression(return_type)?;

        let argument_names = arguments.iter().map(|idx| *idx.data().indentifier().data());
        self.locals.extend(argument_names);
        for statement in body {
            self.statement(statement)?
        }
        self.locals.truncate(self.locals.len() - arguments.len());

        Ok(())
    }

    fn variant(
        &mut self,
        cases: &mut [Located<VariantCase>],
        methods: &mut [Method],
    ) -> ReportableResult<()> {
        for case in cases {
            if let Some(arguments) = case.data_mut().arguments_mut() {
                for argument in arguments {
                    self.type_expression(argument.data_mut().type_expression_mut())?;
                }
            }
        }

        for method in methods {
            self.locals.push(*method.this.data());
            self.procedure(
                &mut method.arguments,
                &mut method.return_type,
                &mut method.body,
            )?;
            self.locals.pop();
        }

        Ok(())
    }

    fn error<T>(&self, error: ResolveError, location: SourceLocation) -> ReportableResult<T> {
        let reportable = (Located::new(error, location), self.current_source.clone());
        Err(Box::new(reportable))
    }
}

pub enum ResolveError {
    ModuleIsNotDeclared,
    ModuleDoesNotExist(InternIdx),
    DuplicateModuleDeclaration,
    DuplicateProcedureDeclaration(Path),
    DuplicateTypeDeclaration(Path),
    DuplicateConstructorDeclaration {
        constructor: InternIdx,
        variant_path: Path,
    },
    UnboundValuePath(Path),
    UnboundTypePath(Path),
}

impl Reportable for (Located<ResolveError>, String) {
    fn location(&self) -> SourceLocation {
        self.0.location()
    }

    fn source(&self) -> &str {
        &self.1
    }

    fn description(&self, interner: &Interner) -> String {
        match self.0.data() {
            ResolveError::ModuleIsNotDeclared => "No module declarations found.".into(),
            ResolveError::ModuleDoesNotExist(name) => {
                format!("Imported module `{}` does not exist.", interner.get(name))
            }
            ResolveError::DuplicateModuleDeclaration => "Duplicate declaration of module.".into(),
            ResolveError::DuplicateProcedureDeclaration(path) => {
                format!(
                    "Duplicate declaration of procedure `{}`.",
                    path.as_string(interner)
                )
            }
            ResolveError::DuplicateTypeDeclaration(path) => {
                format!(
                    "Duplicate declaration of type `{}`.",
                    path.as_string(interner)
                )
            }
            ResolveError::DuplicateConstructorDeclaration {
                constructor,
                variant_path: variant,
            } => {
                format!(
                    "Duplicate declaration of constructor `{}` in variant type `{}`.",
                    interner.get(constructor),
                    variant.as_string(interner)
                )
            }
            ResolveError::UnboundValuePath(path) => {
                format!("`{}` is not bound to a value.", path.as_string(interner))
            }
            ResolveError::UnboundTypePath(path) => {
                format!("`{}` is not bound to a type.", path.as_string(interner))
            }
        }
    }
}
