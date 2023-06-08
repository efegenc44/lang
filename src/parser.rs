use crate::{common::*, lexer::Token};

pub struct Parser {
    tokens: Vec<Token>,
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
                left = Expression::Binary {
                    op,
                    left: Box::new(left),
                    right: Box::new(right),
                }
            }
            Ok(left)
        }
    };
}

impl Parser {
    pub fn new(tokens: Vec<Token>) -> Self {
        Self { tokens, index: 0 }
    }

    fn current_token(&self) -> &Token {
        self.tokens.get(self.index).unwrap()
    }

    fn advance(&mut self) {
        self.index += 1;
    }

    fn expect(&mut self, expected: Token) -> FailsWith<ParseError> {
        if self.current_token() != &expected {
            return Err(ParseError::UnexpectedToken(self.current_token().clone()));
        }
        self.advance();
        Ok(())
    }

    fn product(&mut self) -> ParseResult {
        use Token::*;

        let expr = match self.current_token() {
            NaturalNumber(nat) => Expression::NaturalNumber(*nat),
            LParen => {
                self.advance();
                let expr = self.expr()?;
                self.expect(RParen)?;
                return Ok(expr);
            }
            unexpected_token => return Err(ParseError::UnexpectedToken(unexpected_token.clone())),
        };
        self.advance();

        Ok(expr)
    }

    binary_expr_precedence_level!(term; product; Token::Star);
    binary_expr_precedence_level!(arithmetic; term; Token::Plus);

    #[inline]
    fn expr(&mut self) -> ParseResult {
        self.arithmetic()
    }

    #[inline]
    pub fn parse(&mut self) -> ParseResult {
        self.arithmetic()
    }
}

type ParseResult = Result<Expression, ParseError>;

#[derive(Debug)]
pub enum ParseError {
    UnexpectedToken(Token),
}

#[derive(PartialEq)]
pub enum Expression {
    NaturalNumber(Nat),
    Binary {
        op: BinaryOp,
        left: Box<Expression>,
        right: Box<Expression>,
    },
}

impl Expression {
    pub fn pretty_print(&self) {
        self._pretty_print(0);
    }

    fn _pretty_print(&self, depth: Nat) {
        use Expression::*;

        const PRETTY_PRINT_INDENT_LENGTH: usize = 2;

        macro_rules! pprint {
            ($($arg:tt)*) => {{
                let string = format!($($arg)*);
                println!("{:indent$}{string}", "", indent = depth * PRETTY_PRINT_INDENT_LENGTH)
            }};
        }

        match self {
            NaturalNumber(nat) => pprint!("Nat: {nat}"),
            Binary { op, left, right } => {
                pprint!("Binary: {op:?}");
                left._pretty_print(depth + 1);
                right._pretty_print(depth + 1);
            }
        }
    }
}

#[derive(Debug, PartialEq)]
pub enum BinaryOp {
    Addition,
    Multiplication,
}

impl From<&Token> for BinaryOp {
    fn from(value: &Token) -> Self {
        use Token::*;

        match value {
            Plus => Self::Addition,
            Star => Self::Multiplication,
            _ => unreachable!(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::Lexer;

    #[test]
    fn basic_precedence() {
        let source = "1 + 2 * 3";
        let tokens = Lexer::new(source).collect().unwrap();
        let ast = Parser::new(tokens).parse();

        assert!(ast.is_ok_and(|ast| ast
            == Expression::Binary {
                op: BinaryOp::Addition,
                left: Box::new(Expression::NaturalNumber(1)),
                right: Box::new(Expression::Binary {
                    op: BinaryOp::Multiplication,
                    left: Box::new(Expression::NaturalNumber(2)),
                    right: Box::new(Expression::NaturalNumber(3))
                })
            }))
    }

    #[test]
    fn paren_precedence() {
        let source = "(1 + 2) * 3";
        let tokens = Lexer::new(source).collect().unwrap();
        let ast = Parser::new(tokens).parse();

        assert!(ast.is_ok_and(|ast| ast
            == Expression::Binary {
                op: BinaryOp::Multiplication,
                left: Box::new(Expression::Binary {
                    op: BinaryOp::Addition,
                    left: Box::new(Expression::NaturalNumber(1)),
                    right: Box::new(Expression::NaturalNumber(2))
                }),
                right: Box::new(Expression::NaturalNumber(3))
            }))
    }
}
