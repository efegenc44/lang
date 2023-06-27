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
            "" => continue,
            _ => (),
        }

        let tokens = repl_handle_error!(Lexer::new(input).collect());
        let astree = repl_handle_error!(Parser::new(tokens).parse());
        let e_type = repl_handle_error!(typechecker.verify_type(&astree));
        let result = repl_handle_error!(engine.evaluate(&astree));

        if show_ast {
            println!();
            astree.pretty_print();
            println!();
        }

        println!("= {result} : {e_type:?}\n");
    }

    Ok(())
}
