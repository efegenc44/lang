#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"
#include "expr.h"
#include "decl.h"

#define WHILE_STILL_TOKEN_LEFT(result)                  \
    for (LexResult (result) = Parser_peek_token(parser);\
         (result).kind != LEX_RESULT_DONE;              \
         (result) = Parser_peek_token(parser))          \

Parser Parser_new(Lexer lexer, Arena *arena) {
    LexResult peek = Lexer_next(&lexer);

    return (Parser) {
        .lexer = lexer,
        .peek = peek,
        .decls = OffsetArray_new(),
        .arena = arena
    };
}

ParseResult Parser_type_expr(Parser *parser) {
    return Parser_type_arrow(parser);
}

ParseResult Parser_type_arrow(Parser *parser) {
    // Anonymous product and anonymous sum? types ?
    BINDParseTE(from, Parser_type_primary(parser));
    LexResult result = Parser_peek_token(parser);
    CHECK_LEX_ERROR(result);
    if (result.kind == LEX_RESULT_DONE) {
        return ParseResult_success_type_expr_index(from);
    }
    Token token = result.as.token;

    if (token.kind == TOKEN_RIGHT_ARROW) {
        Parser_advance_token(parser);
        BINDParseTE(to, Parser_type_arrow(parser));
        TypeExpr type_expr = TypeExpr_arrow(from, to, token.span);
        from = Arena_put(parser->arena, type_expr);
    }

    return ParseResult_success_type_expr_index(from);
}

ParseResult Parser_type_primary(Parser *parser) {
    BINDLex(token, Parser_advance_token(parser));
    switch (token.kind) {
        case TOKEN_IDENTIFIER:
            TypeExpr type_expr_ident = TypeExpr_identifier(token.as.lexeme_id, token.span);
            return ParseResult_success_expr_index(
                Arena_put(parser->arena, type_expr_ident)
            );
        case TOKEN_LEFT_PAREN:
            return Parser_finish_paren_type(parser);
        case TOKEN_LEFT_CURLY:
            return Parser_finish_product_type(parser, token.span);
        default:
            ParseError error = ParseError_ut(token);
            return ParseResult_error(error);
    }
}

ParseResult Parser_finish_paren_type(Parser *parser) {
    BINDParseTE(type_expr, Parser_type_expr(parser));
    DOParse(Parser_expect_kind(parser, TOKEN_RIGHT_PAREN));

    return ParseResult_success_type_expr_index(type_expr);
}

ParseResult Parser_finish_product_type(Parser *parser, Span span) {
    // TODO: Memory leak
    StringArray names = StringArray_new();
    OffsetArray type_exprs = OffsetArray_new();

    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);

        if (result.as.token.kind == TOKEN_RIGHT_CURLY) {
            break;
        }

        BINDParseT(token, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
        StringArray_append(&names, token.as.lexeme_id);
        DOParse(Parser_expect_kind(parser, TOKEN_COLON));
        BINDParseTE(type_expr, Parser_type_expr(parser));
        OffsetArray_append(&type_exprs, type_expr);

        BINDLex(peek, Parser_peek_token(parser));
        if (peek.kind == TOKEN_SEMICOLON) {
            Parser_advance_token(parser);
        } else {
            break;
        }
    }
    DOParse(Parser_expect_kind(parser, TOKEN_RIGHT_CURLY));

    TypeExpr product = TypeExpr_product(names, type_exprs, span);
    Offset offset = Arena_put(parser->arena, product);
    return ParseResult_success_type_expr_index(offset);
}

ParseResult Parser_decls(Parser *parser) {
    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);
        DOParse(Parser_decl(parser));
    }

    return ParseResult_success_decls(parser->decls);
}

ParseResult Parser_decl(Parser *parser) {
    Token token = Parser_peek_token(parser).as.token;
    switch (token.kind) {
        case TOKEN_KEYWORD_DEFN:
            return Parser_finish_bind(parser);
        case TOKEN_KEYWORD_DECL:
            return Parser_finish_decl(parser);
        case TOKEN_KEYWORD_TYPE:
            return Parser_finish_type(parser);
        default:
            ParseError error = ParseError_ut(token);
            return ParseResult_error(error);
    }
}

ParseResult Parser_finish_bind(Parser *parser) {
    Parser_advance_token(parser);
    BINDParseT(token, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
    DOParse(Parser_expect_kind(parser, TOKEN_EQUALS));
    BINDParse(expr, Parser_expr(parser));
    Decl bind = Decl_bind(token.as.lexeme_id, expr, token.span);
    Offset offset = Arena_put(parser->arena, bind);
    OffsetArray_append(&parser->decls, offset);

    return ParseResult_success();
}

ParseResult Parser_finish_decl(Parser *parser) {
    Parser_advance_token(parser);
    BINDParseT(token, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
    DOParse(Parser_expect_kind(parser, TOKEN_COLON));
    BINDParseTE(type_expr, Parser_type_expr(parser));
    Decl decldecl = Decl_decldecl(token.as.lexeme_id, type_expr, token.span);
    Offset offset = Arena_put(parser->arena, decldecl);
    OffsetArray_append(&parser->decls, offset);

    return ParseResult_success();
}

ParseResult Parser_finish_type(Parser *parser) {
    Parser_advance_token(parser);
    BINDParseT(token, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
    DOParse(Parser_expect_kind(parser, TOKEN_EQUALS));
    BINDParseTE(type_expr, Parser_type_expr(parser));
    Decl type = Decl_type(token.as.lexeme_id, type_expr, token.span);
    Offset offset = Arena_put(parser->arena, type);
    OffsetArray_append(&parser->decls, offset);


    return ParseResult_success();
}

ParseResult Parser_expr(Parser *parser) {
    // TODO: Check if exhausted all tokens
    BINDLex(token, Parser_peek_token(parser));
    switch (token.kind) {
        case TOKEN_KEYWORD_LET:
            return Parser_finish_let(parser);
        case TOKEN_BACKSLASH:
            return Parser_finish_lambda(parser);
        default:
            return Parser_binary(parser, 0);
    }
}

ParseResult Parser_binary(Parser *parser, size_t min_prec) {
    BINDParse(lhs, Parser_application(parser));
    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);
        Token token = result.as.token;
        switch (token.kind) {
            case TOKEN_PLUS:
            case TOKEN_STAR:
                BOp bop = bop_from_token_kind(token.kind);
                if (PrecTable[bop] < min_prec) goto end;
                Parser_advance_token(parser);
                size_t prec = PrecTable[bop] + (AssocTable[bop] != ASSOC_RIGHT);
                BINDParse(rhs, Parser_binary(parser, prec));
                Expr binary = Expr_binary(lhs, bop, rhs, token.span);
                lhs = Arena_put(parser->arena, binary);
                if (AssocTable[bop] == ASSOC_NONE) goto end;
                break;
            default:
                goto end;
        }
    }
end:
    return ParseResult_success_expr_index(lhs);
}

ParseResult Parser_application(Parser *parser) {
    BINDParse(lhs, Parser_projection(parser));
    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);
        Token token = result.as.token;
        switch (token.kind) {
            // Primary expression starting tokens
            case TOKEN_INTEGER:
            case TOKEN_IDENTIFIER:
            case TOKEN_LEFT_PAREN:
            case TOKEN_LEFT_CURLY:
                BINDParse(argument, Parser_projection(parser));
                Expr application = Expr_application(lhs, argument, token.span);
                lhs = Arena_put(parser->arena, application);
                break;
            default:
                goto end;
        }
    }
end:
    return ParseResult_success_expr_index(lhs);
}

ParseResult Parser_projection(Parser *parser) {
    BINDParse(lhs, Parser_primary(parser));
    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);
        Token token = result.as.token;
        switch (token.kind) {
            case TOKEN_DOT:
                Parser_advance_token(parser);
                BINDParseT(name, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
                Expr projection = Expr_projection(lhs, name.as.lexeme_id, token.span);
                lhs = Arena_put(parser->arena, projection);
                break;
            default:
                goto end;
        }
    }
end:
    return ParseResult_success_expr_index(lhs);
}

ParseResult Parser_primary(Parser *parser) {
    BINDLex(token, Parser_advance_token(parser));
    switch (token.kind) {
        case TOKEN_INTEGER:
            Expr expr_int = Expr_integer(token.as.integer, token.span);
            return ParseResult_success_expr_index(
                Arena_put(parser->arena, expr_int)
            );
        case TOKEN_IDENTIFIER:
            Expr expr_ident = Expr_identifier(token.as.lexeme_id, token.span);
            return ParseResult_success_expr_index(
                Arena_put(parser->arena, expr_ident)
            );
        case TOKEN_LEFT_PAREN:
            return Parser_finish_paren(parser);
        case TOKEN_LEFT_CURLY:
            return Parser_finish_product(parser, token.span);
        default:
            ParseError error = ParseError_ut(token);
            return ParseResult_error(error);
    }
}

ParseResult Parser_finish_paren(Parser *parser) {
    BINDParse(expr, Parser_expr(parser));
    DOParse(Parser_expect_kind(parser, TOKEN_RIGHT_PAREN));

    return ParseResult_success_expr_index(expr);
}

ParseResult Parser_finish_let(Parser *parser) {
    Parser_advance_token(parser);
    BINDParseT(variable, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
    DOParse(Parser_expect_kind(parser, TOKEN_EQUALS));
    BINDParse(vexpr, Parser_expr(parser));
    DOParse(Parser_expect_kind(parser, TOKEN_KEYWORD_IN));
    BINDParse(rexpr, Parser_expr(parser));
    Expr let = Expr_let(variable.as.lexeme_id, vexpr, rexpr, variable.span);
    ExprIndex index = Arena_put(parser->arena, let);

    return ParseResult_success_expr_index(index);
}

ParseResult Parser_finish_lambda(Parser *parser) {
    Parser_advance_token(parser);
    BINDParseT(variable, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
    BINDParse(expr, Parser_expr(parser));
    Expr lambda = Expr_lambda(variable.as.lexeme_id, expr, variable.span);
    ExprIndex index = Arena_put(parser->arena, lambda);

    return ParseResult_success_expr_index(index);
}

ParseResult Parser_finish_product(Parser *parser, Span span) {
    // TODO: Memory leak
    StringArray names = StringArray_new();
    OffsetArray exprs = OffsetArray_new();

    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);

        if (result.as.token.kind == TOKEN_RIGHT_CURLY) {
            break;
        }

        BINDParseT(token, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
        StringArray_append(&names, token.as.lexeme_id);
        DOParse(Parser_expect_kind(parser, TOKEN_EQUALS));
        BINDParse(expr, Parser_expr(parser));
        OffsetArray_append(&exprs, expr);

        BINDLex(peek, Parser_peek_token(parser));
        if (peek.kind == TOKEN_SEMICOLON) {
            Parser_advance_token(parser);
        } else {
            break;
        }
    }
    DOParse(Parser_expect_kind(parser, TOKEN_RIGHT_CURLY));

    Expr product = Expr_product(names, exprs, span);
    Offset offset = Arena_put(parser->arena, product);

    return ParseResult_success_expr_index(offset);
}

LexResult Parser_advance_token(Parser *parser) {
    LexResult peek = Parser_peek_token(parser);
    parser->peek = Lexer_next(&parser->lexer);

    return peek;
}

LexResult Parser_peek_token(Parser *parser) {
    return parser->peek;
}

ParseResult Parser_expect_kind(Parser *parser, TokenKind kind) {
    BINDLex(token, Parser_advance_token(parser));
    if (token.kind == kind) {
        return ParseResult_success_token(token);
    } else {
        ParseError error = ParseError_ut(token);
        return ParseResult_error(error);
    }
}

ParseError ParseError_ut(Token token) {
    return (ParseError) {
        .kind = UNEXPECTED_TOKEN,
        .as.token = token
    };
}

ParseError ParseError_ueof() {
    return (ParseError) {
        .kind = UNEXPECTED_EOF,
    };
}

ParseError ParseError_lex_error(LexError error) {
    return (ParseError) {
        .kind = LEX_ERROR,
        .as.lex_error = error
    };
}

void ParseError_display(ParseError *error, Interner *interner, char *source, char *source_name) {
    if (error->kind == LEX_ERROR) {
        return LexError_display(&error->as.lex_error, source, source_name);
    }

    Span_display_location(&error->as.token.span, source_name);
    printf(": error: ");

    switch (error->kind) {
        case UNEXPECTED_TOKEN:
            printf("Unexpected token : '");
            Token_display(&error->as.token, interner);
            printf("'");
            break;
        case UNEXPECTED_EOF:
            printf("Unexpected end of line\n");
            return;
        case LEX_ERROR:
            assert(false);
    }

    printf(" (at parsing)\n");
    Span_display_in_source(&error->as.token.span, source);
    printf("\n");
}

ParseResult ParseResult_success_type_expr_index(TypeExprIndex type_expr_index) {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
        .as.type_expr_index = type_expr_index
    };
}

ParseResult ParseResult_success_expr_index(ExprIndex expr_index) {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
        .as.expr_index = expr_index
    };
}

ParseResult ParseResult_success_decls(OffsetArray decls) {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
        .as.decls = decls
    };
}

ParseResult ParseResult_success_token(Token token) {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
        .as.token = token
    };
}

ParseResult ParseResult_success() {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
    };
}

ParseResult ParseResult_error(ParseError error) {
    return (ParseResult) {
        .kind = PARSE_RESULT_ERROR,
        .as.error = error
    };
}
