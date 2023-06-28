use crate::common::*;

pub struct Lexer {
    chars: Vec<char>,
    index: Nat,
}

impl Iterator for Lexer {
    type Item = LexResult;

    fn next(&mut self) -> Option<Self::Item> {
        use Token::*;

        let start = self.index;

        if self.index >= self.chars.len() {
            return None;
        }

        let token = match self.current_char() {
            '+' => Plus,
            '-' => Minus,
            '*' => Star,
            '/' => Slash,
            #[rustfmt::skip]
            '!' => if self.peek_is('=') { BangEqual } else { Bang },
            #[rustfmt::skip]
            '=' => if self.peek_is('=') { DoubleEqual } else { Equal },
            #[rustfmt::skip]
            '<' => if self.peek_is('=') { LessEqual } else { Less },
            #[rustfmt::skip]
            '>' => if self.peek_is('=') { GreaterEqual } else { Greater },
            '(' => LParen,
            ')' => RParen,
            '\0' => End,

            ' ' | '\t' | '\r' | '\n' => {
                self.advance();
                return self.next();
            }
            '0'..='9' => return Some(self.lex_number()),
            ch if ch.is_alphabetic() || ch == &'_' => return Some(self.lex_symbol()),
            unknown_start => {
                return Some(Err(
                    LexError::UnknownStartOfAToken(*unknown_start).spanned(start..self.index)
                ))
            }
        };
        self.advance();

        Some(Ok(token.spanned(start..self.index)))
    }
}

impl Lexer {
    pub fn new(source: &str) -> Self {
        let chars = {
            let mut chars: Vec<_> = source.chars().collect();
            chars.push('\0');
            chars
        };
        Self { chars, index: 0 }
    }

    fn current_char(&self) -> &char {
        self.chars.get(self.index).unwrap()
    }

    fn advance(&mut self) {
        self.index += 1;
    }

    fn advance_if(&mut self, expected: char) -> bool {
        let res = self.current_char() == &expected;
        if res {
            self.advance();
        }
        res
    }

    fn peek_is(&mut self, expected: char) -> bool {
        if self.chars[self.index + 1] == expected {
            self.advance();
            true
        } else {
            false
        }
    }

    fn lex_number(&mut self) -> LexResult {
        use Token::*;

        let start = self.index;

        while let '0'..='9' = self.current_char() {
            self.advance();
        }

        let mut is_real = false;
        if self.advance_if('.') {
            is_real = true;
            while let '0'..='9' = self.current_char() {
                self.advance();
            }
        }

        if self.valid_symbol_character() {
            return Err(LexError::InvalidNumericLiteral.spanned(start..self.index + 1));
        }

        let number = self.chars[start..self.index]
            .iter()
            .collect::<String>()
            .into_boxed_str();

        Ok(match is_real {
            true => RealNumber(number),
            false => NaturalNumber(number),
        }
        .spanned(start..self.index))
    }

    fn lex_symbol(&mut self) -> LexResult {
        use Token::*;

        let start = self.index;

        while self.valid_symbol_character() {
            self.advance();
        }

        let symbol = self.chars[start..self.index].iter().collect::<String>();

        Ok(match symbol.as_str() {
            "true" => Ktrue,
            "false" => Kfalse,
            "and" => Kand,
            "or" => Kor,
            _ => Identifier(symbol.into_boxed_str()),
        }
        .spanned(start..self.index))
    }

    fn valid_symbol_character(&self) -> bool {
        self.current_char().is_alphanumeric() || self.current_char() == &'_'
    }

    pub fn collect(self) -> Result<Vec<Spanned<Token>>, Spanned<LexError>> {
        Iterator::collect(self)
    }
}

type LexResult = Result<Spanned<Token>, Spanned<LexError>>;

#[derive(Debug)]
pub enum LexError {
    UnknownStartOfAToken(char),
    InvalidNumericLiteral,
}

impl HasSpan for LexError {}

impl Error for LexError {
    fn message(&self) -> String {
        use LexError::*;

        match self {
            UnknownStartOfAToken(ch) => format!("Encountered an unknown start of a token: `{ch}`"),
            InvalidNumericLiteral => "Numeric Literal contains alphabetic character(s)".to_string(),
        }
    }
}

#[derive(Debug, PartialEq, Clone)]
pub enum Token {
    NaturalNumber(Symbol),
    RealNumber(Symbol),
    Identifier(Symbol),

    Plus,
    Minus,
    Star,
    Slash,
    Bang,
    Equal,
    DoubleEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    LParen,
    RParen,

    Ktrue,
    Kfalse,

    Kand,
    Kor,

    End,
}

impl HasSpan for Token {}

impl std::fmt::Display for Token {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use Token::*;

        match self {
            NaturalNumber(_) => write!(f, "Natural Literal"),
            RealNumber(_) => write!(f, "Real Literal"),
            Identifier(_) => write!(f, "Identifier"),
            Plus => write!(f, "+"),
            Minus => write!(f, "-"),
            Star => write!(f, "*"),
            Slash => write!(f, "/"),
            Bang => write!(f, "!"),
            Equal => write!(f, "="),
            DoubleEqual => write!(f, "=="),
            BangEqual => write!(f, "!="),
            Less => write!(f, "<"),
            LessEqual => write!(f, "<="),
            Greater => write!(f, ">"),
            GreaterEqual => write!(f, ">="),
            LParen => write!(f, "("),
            RParen => write!(f, ")"),
            Ktrue => write!(f, "true"),
            Kfalse => write!(f, "false"),
            Kand => write!(f, "and"),
            Kor => write!(f, "or"),
            End => write!(f, "END"),
        }
    }
}
