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
    let args = std::env::args().collect::<Vec<String>>();

    match args.len() {
        0 => unreachable!(),
        1 => repl()?,
        _ => from_file(args.get(1).unwrap())?,
    }

    Ok(())
}

fn from_file(file_name: &String) -> io::Result<()> {
    let file = std::fs::read_to_string(file_name)?;

    let tokens = file_handle_error!(Lexer::new(&file).collect());
    let astree = file_handle_error!(Parser::new(tokens).parse_top_level());
    let e_type = file_handle_error!(TypeCheker::new().verify_top_level(&astree));
    let result = file_handle_error!(Engine::new().evaluate_top_level(&astree));

    println!("= {result} : {e_type}\n");
    Ok(())
}

fn repl() -> io::Result<()> {
    let mut engine = Engine::new();
    let mut typechecker = TypeCheker::new();

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
        let astree = repl_handle_error!(Parser::new(tokens).parse_expr());
        let e_type = repl_handle_error!(typechecker.verify_type(&astree));
        let result = repl_handle_error!(engine.evaluate(&astree));

        if show_ast {
            println!();
            astree.pretty_print();
            println!();
        }

        println!("= {result} : {e_type}\n");
    }

    Ok(())
}
