use crate::common::*;

pub struct Lexer {
    chars: Vec<char>,
    index: Nat,
}

impl Iterator for Lexer {
    type Item = LexResult;

    fn next(&mut self) -> Option<Self::Item> {
        use Token::*;

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
            unknown_start => return Some(Err(LexError::UnknownStartOfAToken(*unknown_start))),
        };
        self.advance();

        Some(Ok(token))
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
                LexError::NaturalNumberLiteralIsTooLarge
            })?;

        Ok(Token::NaturalNumber(nat))
    }

    pub fn collect(self) -> Result<Vec<Token>, LexError> {
        Iterator::collect::<Result<Vec<Token>, LexError>>(self)
    }
}

type LexResult = Result<Token, LexError>;

#[derive(Debug)]
pub enum LexError {
    UnknownStartOfAToken(char),
    NaturalNumberLiteralIsTooLarge,
}

#[derive(Debug, PartialEq, Clone)]
pub enum Token {
    NaturalNumber(Nat),
    Plus,
    Star,

    LParen,
    RParen,

    End,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn natural_number_literal_is_too_large() {
        #[cfg(target_pointer_width = "64")]
        let source = "18446744073709551616"; // = 2^64

        #[cfg(target_pointer_width = "32")]
        let source = "4294967296"; // = 2^32

        let result = Lexer::new(source).collect();
        assert!(result.is_err_and(|err| matches!(err, LexError::NaturalNumberLiteralIsTooLarge)))
    }

    #[test]
    fn lex_test() {
        let source = "01 * (12 + 23) * 34";
        let expected = [
            Token::NaturalNumber(1),
            Token::Star,
            Token::LParen,
            Token::NaturalNumber(12),
            Token::Plus,
            Token::NaturalNumber(23),
            Token::RParen,
            Token::Star,
            Token::NaturalNumber(34),
            Token::End,
        ];
        let result = Lexer::new(source).collect().unwrap();
        assert_eq!(result, expected)
    }
}
