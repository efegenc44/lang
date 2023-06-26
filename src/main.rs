mod common;
mod engine;
mod lexer;
mod parser;
mod typechecker;

use std::io::{self, Write};

use engine::Engine;
use lexer::Lexer;
use parser::Parser;
use typechecker::TypeCheker;

fn main() -> io::Result<()> {
    let engine = Engine::new();
    let typechecker = TypeCheker::new();

    let mut show_ast = false;
    let mut stdout = io::stdout();
    let stdin = io::stdin();
    loop {
        print!("> ");
        stdout.flush()?;

        let mut input = String::new();
        stdin.read_line(&mut input)?;
        let input = input.trim_end();

        match input {
            ".exit" => break,
            ".ast" => {
                show_ast = !show_ast;
                continue;
            }
            _ => (),
        }

        let tokens = Lexer::new(input).collect().unwrap();
        let astree = Parser::new(tokens).parse().unwrap();
        let e_type = typechecker.verify_type(&astree).unwrap();
        let result = engine.evaluate(&astree).unwrap();

        if show_ast {
            println!();
            astree.pretty_print();
            println!();
        }

        println!("= {result} : {e_type:?}\n");
    }

    Ok(())
}
