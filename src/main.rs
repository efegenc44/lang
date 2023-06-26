mod common;
mod lexer;
mod parser;
mod typechecker;

use lexer::Lexer;
use parser::Parser;
use typechecker::TypeCheker;

fn main() {
    let source = "367 / 55";
    let tokens = Lexer::new(source).collect().unwrap();
    let ast = Parser::new(tokens).parse().unwrap();
    let ty = TypeCheker::new().verify_type(&ast).unwrap();
    println!("\nType of Expression: {ty:?}");
    ast.pretty_print();
    println!();
}
