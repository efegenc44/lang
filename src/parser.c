#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"

Parser parser_new(Lexer lexer) {
    LexResult peek = lexer_next(&lexer);

    return (Parser) {
        .lexer = lexer,
        .peek = peek
    };
}

ParseResult parser_expr(Parser *parser) {
    return parser_binary(parser, 0);
}

ParseResult parser_binary(Parser *parser, size_t min_prec) {
    BINDParse(lhs, parser_primary(parser));
    for (LexResult result = parser_peek_token(parser);
         result.kind != DONE;
         result = parser_peek_token(parser))
    {
        CHECK_LEX_ERROR(result);
        Token token = result.as.token;
        switch (token.kind) {
            case PLUS:
            case STAR:
                BOp bop = bop_from_token_kind(token.kind);
                if (PrecTable[bop] < min_prec) goto end;
                parser_advance_token(parser);
                size_t prec = PrecTable[bop] + (AssocTable[bop] != ASSOC_RIGHT);
                BINDParse(rhs, parser_binary(parser, prec));
                lhs = expr_new_binary(lhs, bop, rhs, token.span);
                if (AssocTable[bop] == ASSOC_NONE) goto end;
                break;
            default:
                goto end;
        }
    }
end:
    return parse_result_new_success_expr(lhs);
}

ParseResult parser_primary(Parser *parser) {
    BINDLex(token, parser_advance_token(parser));
    switch (token.kind) {
        case EXPR_INTEGER:
            Expr expr_int = expr_new_integer(token.as.integer, token.span);
            return parse_result_new_success_expr(expr_int);
        case EXPR_IDENTIFIER:
            Expr expr_ident = expr_new_identifier(token.as.lexeme, token.span);
            return parse_result_new_success_expr(expr_ident);
        case LEFT_PAREN:
            return parser_finish_paren(parser);
        default:
            ParseError error = parse_error_new_ut(token);
            return parse_result_new_error(error);
    }
}

ParseResult parser_finish_paren(Parser *parser) {
    BINDParse(expr, parser_expr(parser));
    DOParse(parser_expect_kind(parser, RIGHT_PAREN));

    return parse_result_new_success_expr(expr);
}

LexResult parser_advance_token(Parser *parser) {
    LexResult peek = parser_peek_token(parser);
    parser->peek = lexer_next(&parser->lexer);

    return peek;
}

LexResult parser_peek_token(Parser *parser) {
    return parser->peek;
}

ParseResult parser_expect_kind(Parser *parser, TokenKind kind) {
    BINDLex(token, parser_advance_token(parser));
    if (token.kind == kind) {
        return parse_result_new_success_token(token);
    } else {
        ParseError error = parse_error_new_ut(token);
        return parse_result_new_error(error);
    }
}

ParseError parse_error_new_ut(Token token) {
    return (ParseError) {
        .kind = UNEXPECTED_TOKEN,
        .as.token = token
    };
}

ParseError parse_error_new_ueof() {
    return (ParseError) {
        .kind = UNEXPECTED_EOF,
    };
}

ParseError parse_error_new_lex_error(LexError error) {
    return (ParseError) {
        .kind = LEX_ERROR,
        .as.lex_error = error
    };
}

void parse_error_display(ParseError *error, char *source) {
    if (error->kind == LEX_ERROR) {
        return lex_error_display(&error->as.lex_error, source);
    }

    if (error->kind == UNEXPECTED_EOF) {
        error->as.token.span = span_new(1, 1, 2);
    }

    span_display_start(&error->as.token.span);
    printf(" | ");

    switch (error->kind) {
        case UNEXPECTED_TOKEN:
            printf("Unexpected token : '");
            token_display(&error->as.token);
            printf("'\n");
            break;
        case UNEXPECTED_EOF:
            printf("Unknown end of line\n");
            break;
        case LEX_ERROR:
            assert(false);
    }

    size_t cursor = 0;
    for (size_t row = 1; row < error->as.token.span.line; row++) {
        while (source[cursor] != '\n') cursor++;
        cursor++;
    }

    for (; source[cursor] != '\n' && source[cursor] != '\0'; cursor++) {
        printf("%c", source[cursor]);
    }
    printf("\n");

    for (size_t i = 1; i < error->as.token.span.start; i++) printf(" ");
    for (size_t i = error->as.token.span.start; i < error->as.token.span.end; i++) printf("^");
    printf("\n");
}

ParseResult parse_result_new_success_expr(Expr expr) {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
        .as.expr = expr
    };
}

ParseResult parse_result_new_success_token(Token token) {
    return (ParseResult) {
        .kind = PARSE_RESULT_SUCCESS,
        .as.token = token
    };
}

ParseResult parse_result_new_error(ParseError error) {
    return (ParseResult) {
        .kind = PARSE_RESULT_ERROR,
        .as.error = error
    };
}
