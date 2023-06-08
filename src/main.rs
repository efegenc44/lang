mod common;
mod lexer;
mod parser;

use lexer::Lexer;
use parser::Parser;

fn main() {
    let source = "01 * (12 + 23) * 34";
    let tokens = Lexer::new(source).collect().unwrap();
    let ast = Parser::new(tokens).parse().unwrap();
    ast.pretty_print()
}
