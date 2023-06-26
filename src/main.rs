mod common;
mod lexer;
mod parser;
mod typechecker;

use lexer::Lexer;
use parser::Parser;
use typechecker::TypeCheker;

fn main() {
    let source = "01.3 * (12 + 23) * 34";
    let tokens = Lexer::new(source).collect().unwrap();
    let ast = Parser::new(tokens).parse().unwrap();
    let ty = TypeCheker::new().verify_type(&ast).unwrap();
    println!("Type of Expression: {ty:?}");
    ast.pretty_print()
}
