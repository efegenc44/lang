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
            '*' => Star,
            '(' => LParen,
            ')' => RParen,
            '\0' => End,

            ' ' | '\t' | '\r' | '\n' => {
                self.advance();
                return self.next();
            }
            '0'..='9' => return Some(self.lex_natural_number()),
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

    fn lex_natural_number(&mut self) -> LexResult {
        let start = self.index;

        while let '0'..='9' = self.current_char() {
            self.advance();
        }

        let nat = self.chars[start..self.index]
            .iter()
            .collect::<String>()
            .parse::<Nat>()
            .map_err(|parse_err| {
                // Other errors should not be able to happen
                assert!(matches!(
                    parse_err.kind(),
                    std::num::IntErrorKind::PosOverflow
                ));
                LexError::NaturalNumberLiteralIsTooLarge.spanned(start..self.index)
            })?;

        Ok(Token::NaturalNumber(nat).spanned(start..self.index))
    }

    pub fn collect(self) -> Result<Vec<Spanned<Token>>, Spanned<LexError>> {
        Iterator::collect(self)
    }
}

type LexResult = Result<Spanned<Token>, Spanned<LexError>>;

#[derive(Debug)]
pub enum LexError {
    UnknownStartOfAToken(char),
    NaturalNumberLiteralIsTooLarge,
}

impl HasSpan for LexError {}

#[derive(Debug, PartialEq, Clone)]
pub enum Token {
    NaturalNumber(Nat),
    Plus,
    Star,

    LParen,
    RParen,

    End,
}

impl HasSpan for Token {}
