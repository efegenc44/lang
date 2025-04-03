#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"

Parser Parser_new(Lexer lexer, ExprArray *expr_array) {
    LexResult peek = Lexer_next(&lexer);

    return (Parser) {
        .lexer = lexer,
        .peek = peek,
        .expr_array = expr_array
    };
}

ParseResult Parser_expr(Parser *parser) {
    return Parser_binary(parser, 0);
}

ParseResult Parser_binary(Parser *parser, size_t min_prec) {
    BINDParse(lhs, Parser_primary(parser));
    for (LexResult result = Parser_peek_token(parser);
         result.kind != LEX_RESULT_DONE;
         result = Parser_peek_token(parser))
    {
        CHECK_LEX_ERROR(result);
        Token token = result.as.token;
        switch (token.kind) {
            case PLUS:
            case STAR:
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

ParseResult Parser_primary(Parser *parser) {
    BINDLex(token, Parser_advance_token(parser));
    switch (token.kind) {
        case EXPR_INTEGER:
            Expr expr_int = Expr_integer(token.as.integer, token.span);
            return ParseResult_success_expr_index(
                ExprArray_append(parser->expr_array, expr_int)
            );
        case EXPR_IDENTIFIER:
            Expr expr_ident = Expr_identifier(token.as.lexeme_id, token.span);
            return ParseResult_success_expr_index(
                ExprArray_append(parser->expr_array, expr_ident)
            );
        case LEFT_PAREN:
            return Parser_finish_paren(parser);
        default:
            ParseError error = ParseError_ut(token);
            return ParseResult_error(error);
    }
}

ParseResult Parser_finish_paren(Parser *parser) {
    BINDParse(expr, Parser_expr(parser));
    DOParse(Parser_expect_kind(parser, RIGHT_PAREN));

    return ParseResult_success_expr_index(expr);
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

ParseResult ParseResult_error(ParseError error) {
    return (ParseResult) {
        .kind = PARSE_RESULT_ERROR,
        .as.error = error
    };
}
