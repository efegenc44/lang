#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "token.h"

Lexer Lexer_new(char *source) {
    return (Lexer) {
        .source = source,
        .cursor = 0,
        .row = 1,
        .column = 1,
    };
}

LexResult Lexer_next(Lexer *lexer) {
    while (isspace(Lexer_peek(lexer))) {
        Lexer_advance(lexer);
    }
    char current_char = Lexer_peek(lexer);

    if (current_char == '\0') {
        return LexResult_done();
    }

    if (isdigit(current_char)) {
        return Lexer_integer(lexer);
    }

    if (isalpha(current_char)) {
        return Lexer_identifier(lexer);
    }

    size_t start = lexer->column;
    TokenKind kind;
    switch (Lexer_advance(lexer)) {
        case '(':
            kind = TOKEN_LEFT_PAREN;
            break;
        case ')':
            kind = TOKEN_RIGHT_PAREN;
            break;
        case '{':
            kind = TOKEN_LEFT_CURLY;
            break;
        case '}':
            kind = TOKEN_RIGHT_CURLY;
            break;
        case '+':
            kind = TOKEN_PLUS;
            break;
        case '*':
            kind = TOKEN_STAR;
            break;
        case '=':
            kind = TOKEN_EQUALS;
            break;
        case '\\':
            kind = TOKEN_BACKSLASH;
            break;
        case ':':
            kind = TOKEN_COLON;
            break;
        case ';':
            kind = TOKEN_SEMICOLON;
            break;
        case '.':
            kind = TOKEN_DOT;
            break;
        case '-':
            if (Lexer_peek(lexer) == '>') {
                Lexer_advance(lexer);
                kind = TOKEN_RIGHT_ARROW;
            } else {
                kind = TOKEN_MINUS;
            }
            break;
        default:
            Span span = Lexer_span(lexer, start);
            LexError error = LexError_uts(current_char, span);
            return LexResult_error(error);
    }

    Span span = Lexer_span(lexer, start);
    Token token = Token_kind(kind, span);
    return LexResult_success(token);
}

LexResult Lexer_integer(Lexer *lexer) {
    size_t start = lexer->column;

    size_t integer = Lexer_advance(lexer) - '0';
    while (isdigit(Lexer_peek(lexer))) {
        integer = 10*integer + (Lexer_advance(lexer) - '0');
    }

    Span span = Lexer_span(lexer, start);
    Token token = Token_integer(integer, span);
    return LexResult_success(token);
}

LexResult Lexer_identifier(Lexer *lexer) {
    size_t start = lexer->column;
    size_t start_index = lexer->cursor;
    while (isalnum(Lexer_peek(lexer))) {
        Lexer_advance(lexer);
    }
    size_t length = lexer->cursor - start_index;

    Token token;
    char *lexeme = &lexer->source[start_index];
    Span span = Lexer_span(lexer, start);
    if (strncmp(lexeme, "let", 3) == 0) {
        token = Token_kind(TOKEN_KEYWORD_LET, span);
    } else if (strncmp(lexeme, "in", 2) == 0) {
        token = Token_kind(TOKEN_KEYWORD_IN, span);
    } else if (strncmp(lexeme, "defn", 4) == 0) {
        token = Token_kind(TOKEN_KEYWORD_DEFN, span);
    } else if (strncmp(lexeme, "decl", 4) == 0) {
        token = Token_kind(TOKEN_KEYWORD_DECL, span);
    } else if (strncmp(lexeme, "type", 4) == 0) {
        token = Token_kind(TOKEN_KEYWORD_TYPE, span);
    } else {
        char *lexeme = strndup(&lexer->source[start_index], length);
        InternId lexeme_id = Interner_register(lexeme);
        token = Token_identifier(lexeme_id, span);
    }

    return LexResult_success(token);
}

char Lexer_advance(Lexer *lexer) {
    char current = Lexer_peek(lexer);
    if (current == '\n') {
        lexer->row += 1;
        lexer->column = 1;
    } else {
        lexer->column += 1;
    }
    lexer->cursor += 1;

    return current;
}

char Lexer_peek(Lexer *lexer) {
    return lexer->source[lexer->cursor];
}

Span Lexer_span(Lexer *lexer, size_t start) {
    return (Span) {
        .line = lexer->row,
        .start = start,
        .end = lexer->column
    };
}

LexError LexError_uts(char ch, Span span) {
    return (LexError) {
        .kind = LEX_ERROR_UNKNOWN_TOKEN_START,
        .data = ch,
        .span = span
    };
}

void LexError_display(LexError *error, char *source, char *source_name) {
    Span_display_location(&error->span, source_name);
    printf(": error: ");

    switch (error->kind) {
        case LEX_ERROR_UNKNOWN_TOKEN_START:
            printf("Unknown start of a token : '%c'", error->data);
            break;
    }

    printf(" (at tokenizing)\n");
    Span_display_in_source(&error->span, source);
    printf("\n");
}

LexResult LexResult_success(Token token) {
    return (LexResult) {
        .kind = LEX_RESULT_SUCCESS,
        .as.token = token
    };
}

LexResult LexResult_done() {
    return (LexResult) {
        .kind = LEX_RESULT_DONE,
    };
}

LexResult LexResult_error(LexError error) {
    return (LexResult) {
        .kind = LEX_RESULT_ERROR,
        .as.error = error
    };
}
