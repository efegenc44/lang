use crate::{common::*, lexer::Token};

pub struct Parser {
    tokens: Vec<Spanned<Token>>,
    index: Nat,
}

macro_rules! binary_expr_precedence_level {
    ( $name:ident; $inferior:ident; $operators:pat ) => {
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

    fn product(&mut self) -> ParseResult {
        use Token::*;

        let current_token_span = self.get_span();
        let expr = match self.current_token() {
            NaturalNumber(nat) => {
                Expression::NaturalNumber(nat.clone()).spanned(current_token_span)
            }
            RealNumber(real) => Expression::RealNumber(real.clone()).spanned(current_token_span),
            LParen => {
                self.advance();
                let expr = self.expr()?.data;
                let end_span = self.expect(RParen)?;
                return Ok(expr.start_end(current_token_span, end_span));
            }
            Minus => {
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

    binary_expr_precedence_level!(term; product; Token::Star | Token::Slash);
    binary_expr_precedence_level!(arithmetic; term; Token::Plus | Token::Minus);

    #[inline]
    fn expr(&mut self) -> ParseResult {
        self.arithmetic()
    }

    #[inline]
    pub fn parse(&mut self) -> ParseResult {
        self.arithmetic()
    }
}

type ParseResult = Result<Spanned<Expression>, Spanned<ParseError>>;

#[derive(Debug)]
pub enum ParseError {
    UnknownStartOfAnExpression(Token),
    ExpectedADifferentToken { found: Token, expected: Token },
}

impl HasSpan for ParseError {}

impl Error for ParseError {
    fn message(&self) -> String {
        use ParseError::*;
        match self {
            UnknownStartOfAnExpression(token) => {
                format!("No expression starts with this token: `{token:?}`")
            }
            ExpectedADifferentToken { found, expected } => {
                if let Token::RParen = expected {
                    "Unclosed parentheses".to_string()
                } else {
                    format!("Expected a `{expected:?}`, instead found `{found:?}`")
                }
            }
        }
    }
}

pub enum Expression {
    NaturalNumber(Symbol),
    RealNumber(Symbol),
    Binary {
        op: BinaryOp,
        left: Box<Spanned<Expression>>,
        right: Box<Spanned<Expression>>,
    },
    Unary {
        op: UnaryOp,
        operand: Box<Spanned<Expression>>,
    },
}

impl HasSpan for Expression {}

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
            NaturalNumber(nat) => pprint!("Nat: {nat}"),
            RealNumber(real) => pprint!("Real: {real}"),
            Binary { op, left, right } => {
                pprint!("Binary: {op:?}");
                left._pretty_print(depth + 1);
                right._pretty_print(depth + 1);
            }
            Unary { op, operand } => {
                pprint!("Unary: {op:?}");
                operand._pretty_print(depth + 1);
            }
        }
    }
}

#[derive(Debug)]
pub enum BinaryOp {
    Addition,
    Subtraction,
    Multiplication,
    Division,
}

impl From<&Token> for BinaryOp {
    fn from(value: &Token) -> Self {
        use Token::*;

        match value {
            Plus => Self::Addition,
            Minus => Self::Subtraction,
            Star => Self::Multiplication,
            Slash => Self::Division,
            _ => unreachable!(),
        }
    }
}

#[derive(Debug)]
pub enum UnaryOp {
    Negation,
}

impl From<&Token> for UnaryOp {
    fn from(value: &Token) -> Self {
        use Token::*;

        match value {
            Minus => Self::Negation,
            _ => unreachable!(),
        }
    }
}
