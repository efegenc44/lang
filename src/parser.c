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

Parser Parser_new(Lexer lexer, ExprArray *expr_array, DeclMap *decl_map) {
    LexResult peek = Lexer_next(&lexer);

    return (Parser) {
        .lexer = lexer,
        .peek = peek,
        .expr_array = expr_array,
        .decl_map = decl_map
    };
}

ParseResult Parser_decls(Parser *parser) {
    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);
        DOParse(Parser_decl(parser));
    }

    return ParseResult_success();
}

ParseResult Parser_decl(Parser *parser) {
    Token token = Parser_peek_token(parser).as.token;
    switch (token.kind) {
        case TOKEN_KEYWORD_DEF:
            return Parser_finish_bind(parser);
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
    // TODO: We use the same InternId twice, in Decl and in DeclPair
    // maybe we don't need InternId in Decl.
    DeclMap_add(parser->decl_map, token.as.lexeme_id, bind);

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
                lhs = ExprArray_append(parser->expr_array, Expr_binary(lhs, bop, rhs, token.span));
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
    BINDParse(function, Parser_primary(parser));
    WHILE_STILL_TOKEN_LEFT(result) {
        CHECK_LEX_ERROR(result);
        Token token = result.as.token;
        switch (token.kind) {
            // Primary expression starting tokens
            case TOKEN_INTEGER:
            case TOKEN_IDENTIFIER:
            case TOKEN_LEFT_PAREN:
                BINDParse(argument, Parser_primary(parser));
                function = ExprArray_append(parser->expr_array, Expr_application(function, argument, token.span));
                break;
            default:
                goto end;
        }
    }
end:
    return ParseResult_success_expr_index(function);
}

ParseResult Parser_primary(Parser *parser) {
    BINDLex(token, Parser_advance_token(parser));
    switch (token.kind) {
        case TOKEN_INTEGER:
            Expr expr_int = Expr_integer(token.as.integer, token.span);
            return ParseResult_success_expr_index(
                ExprArray_append(parser->expr_array, expr_int)
            );
        case TOKEN_IDENTIFIER:
            Expr expr_ident = Expr_identifier(token.as.lexeme_id, token.span);
            return ParseResult_success_expr_index(
                ExprArray_append(parser->expr_array, expr_ident)
            );
        case TOKEN_LEFT_PAREN:
            return Parser_finish_paren(parser);
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
    ExprIndex index = ExprArray_append(parser->expr_array, let);

    return ParseResult_success_expr_index(index);
}

ParseResult Parser_finish_lambda(Parser *parser) {
    Parser_advance_token(parser);
    BINDParseT(variable, Parser_expect_kind(parser, TOKEN_IDENTIFIER));
    BINDParse(expr, Parser_expr(parser));
    Expr lambda = Expr_lambda(variable.as.lexeme_id, expr, variable.span);
    ExprIndex index = ExprArray_append(parser->expr_array, lambda);

    return ParseResult_success_expr_index(index);
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

ParseResult ParseResult_success_expr_index(ExprIndex expr_index) {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
        .as.expr_index = expr_index
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
