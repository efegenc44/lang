use crate::{common::*, lexer::Token};

pub struct Parser {
    tokens: Vec<Spanned<Token>>,
    index: Nat,
}

macro_rules! binary_expr_precedence_level {
    ( $name:ident, $inferior:ident, $operators:pat, LEFT_ASSOC ) => {
        fn $name(&mut self) -> ParseResult {
            let mut left = self.$inferior()?;
            while let current_token @ $operators = self.current_token() {
                let op = current_token.into();
                self.advance();
                let right = self.$inferior()?;
                let (left_span, right_span) = (left.span.clone(), right.span.clone());
                left = Expression::Binary {
                    op,
                    left: Box::new(left),
                    right: Box::new(right),
                }
                .start_end(left_span, right_span)
            }
            Ok(left)
        }
    };

    ( $name:ident, $inferior:ident, $operators:pat, NO_ASSOC ) => {
        fn $name(&mut self) -> ParseResult {
            let mut left = self.$inferior()?;
            if let current_token @ $operators = self.current_token() {
                let op = current_token.into();
                self.advance();
                let right = self.$inferior()?;
                let (left_span, right_span) = (left.span.clone(), right.span.clone());
                left = Expression::Binary {
                    op,
                    left: Box::new(left),
                    right: Box::new(right),
                }
                .start_end(left_span, right_span)
            }
            Ok(left)
        }
    };
}

impl Parser {
    pub fn new(tokens: Vec<Spanned<Token>>) -> Self {
        Self { tokens, index: 0 }
    }

    fn current_token(&self) -> &Token {
        &self.tokens.get(self.index).unwrap().data
    }

    fn get_span(&self) -> Span {
        self.tokens.get(self.index).unwrap().span.clone()
    }

    fn advance(&mut self) {
        self.index += 1;
    }

    fn expect(&mut self, expected: Token) -> Result<Span, Spanned<ParseError>> {
        if self.current_token() != &expected {
            return Err(ParseError::ExpectedADifferentToken {
                found: self.current_token().clone(),
                expected,
            }
            .spanned(self.get_span()));
        }
        let span = self.get_span();
        self.advance();
        Ok(span)
    }

    fn expect_identifier(&mut self) -> Result<Spanned<Symbol>, Spanned<ParseError>> {
        let Token::Identifier(symbol) = self.current_token() else {
            return Err(ParseError::ExpectedAnIdentifier(self.current_token().clone())
                .spanned(self.get_span()));
        };
        let symbol = symbol.clone();
        let span = self.get_span();
        self.advance();
        Ok(Spanned { data: symbol, span })
    }

    fn optional(&mut self, expected: Token) -> bool {
        if self.current_token() == &expected {
            self.advance();
            true
        } else {
            false
        }
    }

    fn type_expr(&mut self) -> Result<Spanned<TypeExpr>, Spanned<ParseError>> {
        use Token::*;

        let current_token_span = self.get_span();
        let expr = match self.current_token() {
            Identifier(symbol) => TypeExpr::Identifier(symbol.clone()).spanned(current_token_span),
            LParen => {
                self.advance();
                if self.optional(RParen) {
                    return Ok(TypeExpr::UnitValue.start_end(current_token_span, self.get_span()));
                }
                let expr = self.type_expr()?.data;
                let end_span = self.expect(RParen)?;
                return Ok(expr.start_end(current_token_span, end_span));
            }
            Kfun => {
                self.advance();
                let arg_types = self.parse_comma_seperated(Self::type_expr)?;
                let return_type = match self.optional(Arrow) {
                    true => Some(Box::new(self.type_expr()?)),
                    false => None,
                };
                return Ok(TypeExpr::Function {
                    arg_types,
                    return_type,
                }
                .start_end(current_token_span, self.get_span()));
            }
            unexpected_token => {
                return Err(
                    ParseError::UnkownStartOfATypeExpression(unexpected_token.clone())
                        .spanned(current_token_span),
                )
            }
        };
        self.advance();

        Ok(expr)
    }

    fn product(&mut self) -> ParseResult {
        use Token::*;

        let current_token_span = self.get_span();
        let expr = match self.current_token() {
            Identifier(symbol) => {
                Expression::Identifier(symbol.clone()).spanned(current_token_span)
            }
            NaturalNumber(nat) => {
                Expression::NaturalNumber(nat.clone()).spanned(current_token_span)
            }
            RealNumber(real) => Expression::RealNumber(real.clone()).spanned(current_token_span),
            Ktrue => Expression::BoolValue(true).spanned(current_token_span),
            Kfalse => Expression::BoolValue(false).spanned(current_token_span),
            LParen => {
                self.advance();
                if self.optional(RParen) {
                    return Ok(Expression::UnitValue.start_end(current_token_span, self.get_span()));
                }
                let expr = self.expr()?.data;
                let end_span = self.expect(RParen)?;
                return Ok(expr.start_end(current_token_span, end_span));
            }
            Bang | Minus => {
                let op = self.current_token().into();
                self.advance();
                let operand = self.product()?;
                let end_span = operand.span.clone();
                return Ok(Expression::Unary {
                    op,
                    operand: Box::new(operand),
                }
                .start_end(current_token_span, end_span));
            }
            unexpected_token => {
                return Err(
                    ParseError::UnknownStartOfAnExpression(unexpected_token.clone())
                        .spanned(current_token_span),
                )
            }
        };
        self.advance();

        Ok(expr)
    }

    fn call_expr(&mut self) -> ParseResult {
        use Token::*;

        let mut expr = self.product()?;
        #[allow(clippy::while_let_loop)]
        loop {
            match self.current_token() {
                LParen => {
                    let start_span = expr.span.clone();
                    let args = self.parse_comma_seperated(Self::expr)?;
                    expr = Expression::FunctionCall {
                        f: Box::new(expr),
                        args,
                    }
                    .start_end(start_span, self.get_span());
                }
                _ => break,
            }
        }

        Ok(expr)
    }

    #[rustfmt::skip]    binary_expr_precedence_level!(term,       call_expr,  Token::Star        | Token::Slash,        LEFT_ASSOC);
    #[rustfmt::skip]    binary_expr_precedence_level!(arithmetic, term,       Token::Plus        | Token::Minus,        LEFT_ASSOC);
    #[rustfmt::skip]    binary_expr_precedence_level!(comparison, arithmetic, Token::Less        | Token::LessEqual |
                                                                              Token::Greater     | Token::GreaterEqual, NO_ASSOC);
    #[rustfmt::skip]    binary_expr_precedence_level!(equality,   comparison, Token::DoubleEqual | Token::BangEqual,    NO_ASSOC);
    #[rustfmt::skip]    binary_expr_precedence_level!(bool_and,   equality,   Token::Kand,                              LEFT_ASSOC);
    #[rustfmt::skip]    binary_expr_precedence_level!(bool_or,    bool_and,   Token::Kor,                               LEFT_ASSOC);
    #[rustfmt::skip]    binary_expr_precedence_level!(assignment, bool_or,    Token::Equal,                             NO_ASSOC);
    #[rustfmt::skip]    binary_expr_precedence_level!(sequence,   assignment, Token::SemiColon,                         LEFT_ASSOC);

    fn let_expr(&mut self) -> ParseResult {
        use Token::*;

        let start_span = self.expect(Klet)?;
        let name = self.expect_identifier()?;

        let type_annot = match self.optional(Colon) {
            true => Some(self.type_expr()?),
            false => None,
        };

        self.expect(Equal)?;
        let value = Box::new(self.expr()?);
        self.expect(Kin)?;
        let expr = Box::new(self.expr()?);

        Ok(Expression::Let {
            name,
            type_annot,
            value,
            expr,
        }
        .start_end(start_span, self.get_span()))
    }

    fn fun_expr(&mut self) -> ParseResult {
        use Token::*;

        let start_span = self.expect(Kfun)?;
        let name = self.expect_identifier()?;
        let args = self.parse_comma_seperated(Self::typed_identifier)?;
        let return_type = match self.optional(Arrow) {
            true => Some(self.type_expr()?),
            false => None,
        };
        self.expect(Equal)?;
        let expr = Box::new(self.expr()?);
        self.expect(Kin)?;
        let in_expr = Box::new(self.expr()?);

        Ok(Expression::Fun {
            name,
            args,
            return_type,
            expr,
            in_expr,
        }
        .start_end(start_span, self.get_span()))
    }

    fn if_expr(&mut self) -> ParseResult {
        use Token::*;

        let start_span = self.expect(Kif)?;
        let condition = Box::new(self.expr()?);
        let true_expr = Box::new(self.expr()?);
        let false_expr = match self.optional(Kelse) {
            true => Some(Box::new(self.expr()?)),
            false => None,
        };

        Ok(Expression::If {
            condition,
            true_expr,
            false_expr,
        }
        .start_end(start_span, self.get_span()))
    }

    fn return_expr(&mut self) -> ParseResult {
        use Token::*;

        let start_span = self.expect(Kreturn)?;
        let expr = Box::new(self.expr()?);
        Ok(Expression::Return(expr).start_end(start_span, self.get_span()))
    }

    fn expr(&mut self) -> ParseResult {
        use Token::*;

        match self.current_token() {
            Klet => self.let_expr(),
            Kfun => self.fun_expr(),
            Kif => self.if_expr(),
            Kreturn => self.return_expr(),
            _ => self.sequence(),
        }
    }

    pub fn parse_expr(&mut self) -> ParseResult {
        let ast = self.expr()?;

        if self.index != self.tokens.len() - 1 {
            return Err(
                ParseError::UnconsumedTokenOrTokens(self.current_token().clone())
                    .spanned(self.get_span()),
            );
        }

        Ok(ast)
    }

    pub fn parse_top_level(&mut self) -> Result<Vec<Spanned<TopLevel>>, Spanned<ParseError>> {
        use Token::*;

        let mut definitions = vec![];
        loop {
            let def = match self.current_token() {
                Klet => self.top_level_let()?,
                Kfun => self.top_level_fun()?,
                End => break,
                invalid => {
                    return Err(ParseError::InvalidTopLevelElement(invalid.clone())
                        .spanned(self.get_span()))
                }
            };
            definitions.push(def);
        }
        Ok(definitions)
    }

    fn top_level_let(&mut self) -> Result<Spanned<TopLevel>, Spanned<ParseError>> {
        use Token::*;

        let start_span = self.expect(Klet)?;
        let name = self.expect_identifier()?;
        let type_annot = match self.optional(Colon) {
            true => Some(self.type_expr()?),
            false => None,
        };
        self.expect(Equal)?;
        let value = Box::new(self.expr()?);

        Ok(TopLevel::Let {
            name,
            type_annot,
            value,
        }
        .start_end(start_span, self.get_span()))
    }

    fn top_level_fun(&mut self) -> Result<Spanned<TopLevel>, Spanned<ParseError>> {
        use Token::*;

        let start_span = self.expect(Kfun)?;
        let name = self.expect_identifier()?;
        let args = self.parse_comma_seperated(Self::typed_identifier)?;
        let return_type = match self.optional(Arrow) {
            true => Some(self.type_expr()?),
            false => None,
        };
        self.expect(Equal)?;
        let expr = self.expr()?;

        Ok(TopLevel::Fun {
            name,
            args,
            return_type,
            expr,
        }
        .start_end(start_span, self.get_span()))
    }

    fn typed_identifier(
        &mut self,
    ) -> Result<(Spanned<Symbol>, Spanned<TypeExpr>), Spanned<ParseError>> {
        let arg_name = self.expect_identifier()?;
        self.expect(Token::Colon)?;
        let type_annot = self.type_expr()?;
        Ok((arg_name, type_annot))
    }

    fn parse_comma_seperated<T>(
        &mut self,
        f: fn(&mut Self) -> Result<T, Spanned<ParseError>>,
    ) -> Result<Vec<T>, Spanned<ParseError>> {
        use Token::*;

        let mut values = vec![];
        self.expect(LParen)?;
        if !self.optional(RParen) {
            values.push(f(self)?);
            while !self.optional(RParen) {
                self.expect(Comma)?;
                values.push(f(self)?);
            }
        }
        Ok(values)
    }
}

type ParseResult = Result<Spanned<Expression>, Spanned<ParseError>>;

#[derive(Debug)]
pub enum ParseError {
    UnknownStartOfAnExpression(Token),
    ExpectedADifferentToken { found: Token, expected: Token },
    UnconsumedTokenOrTokens(Token),
    ExpectedAnIdentifier(Token),
    UnkownStartOfATypeExpression(Token),
    InvalidTopLevelElement(Token),
}

impl HasSpan for ParseError {}

impl Error for ParseError {
    fn message(&self) -> String {
        use ParseError::*;
        match self {
            UnknownStartOfAnExpression(token) => {
                format!("No expression starts with this token: `{token}`")
            }
            ExpectedADifferentToken { found, expected } => {
                if let Token::RParen = expected {
                    "Unclosed parentheses".to_string()
                } else {
                    format!("Expected a `{expected}`, instead found `{found}`")
                }
            }
            UnconsumedTokenOrTokens(token) => {
                format!("Parser couldn't consume all tokens. First of unconsumed tokens: `{token}`")
            }
            ExpectedAnIdentifier(token) => {
                format!("Expected an `Identifier`, instead found `{token}`")
            }
            UnkownStartOfATypeExpression(token) => {
                format!("No type expression starts with this token: `{token}`")
            }
            InvalidTopLevelElement(token) => format!("Only `let` and `fun` definitions are allowed for top-level, invalid token: `{token}`"),
        }
    }
}

#[derive(Clone)]
pub enum Expression {
    Identifier(Symbol),
    NaturalNumber(Symbol),
    RealNumber(Symbol),
    BoolValue(bool),
    UnitValue,
    Binary {
        op: BinaryOp,
        left: Box<Spanned<Expression>>,
        right: Box<Spanned<Expression>>,
    },
    Unary {
        op: UnaryOp,
        operand: Box<Spanned<Expression>>,
    },
    Let {
        name: Spanned<Symbol>,
        type_annot: Option<Spanned<TypeExpr>>,
        value: Box<Spanned<Expression>>,
        expr: Box<Spanned<Expression>>,
    },
    Fun {
        name: Spanned<Symbol>,
        args: Vec<(Spanned<Symbol>, Spanned<TypeExpr>)>,
        return_type: Option<Spanned<TypeExpr>>,
        expr: Box<Spanned<Expression>>,
        in_expr: Box<Spanned<Expression>>,
    },
    FunctionCall {
        f: Box<Spanned<Expression>>,
        args: Vec<Spanned<Expression>>,
    },
    If {
        condition: Box<Spanned<Expression>>,
        true_expr: Box<Spanned<Expression>>,
        false_expr: Option<Box<Spanned<Expression>>>,
    },
    Return(Box<Spanned<Expression>>),
}

impl HasSpan for Expression {}

#[derive(Clone)]
pub enum TopLevel {
    Let {
        name: Spanned<Symbol>,
        type_annot: Option<Spanned<TypeExpr>>,
        value: Box<Spanned<Expression>>,
    },
    Fun {
        name: Spanned<Symbol>,
        args: Vec<(Spanned<Symbol>, Spanned<TypeExpr>)>,
        return_type: Option<Spanned<TypeExpr>>,
        expr: Spanned<Expression>,
    },
}

impl HasSpan for TopLevel {}

impl Spanned<Expression> {
    pub fn pretty_print(&self) {
        self._pretty_print(0);
    }

    fn _pretty_print(&self, depth: Nat) {
        use Expression::*;

        const PRETTY_PRINT_INDENT_LENGTH: usize = 2;

        macro_rules! pprint {
            ($($arg:tt)*) => {{
                let string = format!($($arg)*);
                println!("{:indent$}{string} [{span:?}]", "", indent = depth * PRETTY_PRINT_INDENT_LENGTH, span = self.span)
            }};
        }

        match &self.data {
            Identifier(symbol) => pprint!("Ident: {symbol}"),
            NaturalNumber(nat) => pprint!("Nat: {nat}"),
            RealNumber(real) => pprint!("Real: {real}"),
            BoolValue(value) => pprint!("Bool: {value}"),
            UnitValue => pprint!("()"),
            Binary { op, left, right } => {
                pprint!("Binary: {op:?}");
                left._pretty_print(depth + 1);
                right._pretty_print(depth + 1);
            }
            Unary { op, operand } => {
                pprint!("Unary: {op:?}");
                operand._pretty_print(depth + 1);
            }
            Let {
                name,
                type_annot,
                value,
                expr,
            } => {
                // TODO
                pprint!("Let: ident: {} {:?}", name.data, type_annot);
                value._pretty_print(depth + 1);
                expr._pretty_print(depth + 1);
            }
            // TODO
            Fun {
                name: _,
                args: _,
                return_type: _,
                expr: _,
                in_expr: _,
            } => todo!(),
            FunctionCall { f, args } => {
                // TODO
                pprint!("Function Call:");
                f._pretty_print(depth + 1);
                for arg in args {
                    arg._pretty_print(depth + 2);
                }
            }
            // TODO
            If {
                condition: _,
                true_expr: _,
                false_expr: _,
            } => todo!(),
            Return(value) => {
                pprint!("Return:");
                value._pretty_print(depth + 1);
            }
        }
    }
}

#[derive(Debug, Clone)]
pub enum BinaryOp {
    Addition,
    Subtraction,
    Multiplication,
    Division,
    And,
    Or,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Sequence,
    Assignment,
}

impl From<&Token> for BinaryOp {
    fn from(value: &Token) -> Self {
        use Token::*;

        match value {
            Plus => Self::Addition,
            Minus => Self::Subtraction,
            Star => Self::Multiplication,
            Slash => Self::Division,
            Kand => Self::And,
            Kor => Self::Or,
            DoubleEqual => Self::Equal,
            BangEqual => Self::NotEqual,
            Less => Self::Less,
            LessEqual => Self::LessEqual,
            Greater => Self::Greater,
            GreaterEqual => Self::GreaterEqual,
            SemiColon => Self::Sequence,
            Equal => Self::Assignment,
            _ => unreachable!(),
        }
    }
}

#[derive(Debug, Clone)]
pub enum UnaryOp {
    Negation,
    Not,
}

impl From<&Token> for UnaryOp {
    fn from(value: &Token) -> Self {
        use Token::*;

        match value {
            Minus => Self::Negation,
            Bang => Self::Not,
            _ => unreachable!(),
        }
    }
}

#[derive(Debug, Clone)]
pub enum TypeExpr {
    Identifier(Symbol),
    Function {
        arg_types: Vec<Spanned<TypeExpr>>,
        return_type: Option<Box<Spanned<TypeExpr>>>,
    },
    UnitValue,
}

impl HasSpan for TypeExpr {}
