mod common;
mod engine;
mod lexer;
mod parser;
mod typechecker;

use engine::Engine;
use lexer::Lexer;
use parser::Parser;
use typechecker::TypeCheker;

fn main() {
    let source = "545989634576987463797687639759437684 * 4583767346";
    let tokens = Lexer::new(source).collect().unwrap();
    let ast = Parser::new(tokens).parse().unwrap();
    let ty = TypeCheker::new().verify_type(&ast).unwrap();
    let result = Engine::new().evaluate(&ast).unwrap();
    println!("\n------------------------------------------");
    println!("Type of Expression: {ty:?}");
    ast.pretty_print();
    println!();
    println!("Result: {result}");
    println!("------------------------------------------");
    println!();
}
