#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "type.h"
#include "span.h"
#include "arena.h"

#define CHECK_TYPE_CHECK_ERROR(result)  \
    switch ((result).kind) {            \
        case TYPE_CHECK_RESULT_ERROR:   \
            return (result);            \
        case TYPE_CHECK_RESULT_SUCCESS: \
    }                                   \

#define BINDTypeCheck(name, expr)           \
    TypeCheckResult name##_result = (expr); \
    CHECK_TYPE_CHECK_ERROR(name##_result);  \
    Type name = (name##_result).as.type;    \

#define DOTypeCheck(expr)                \
    {                                    \
        TypeCheckResult result = (expr); \
        CHECK_TYPE_CHECK_ERROR(result);  \
    }                                    \

typedef struct {
    TypeArray locals;
    // TODO: Use HashMap or associative array
    StringArray global_names;
    OffsetArray name_type_exprs;

    StringArray global_type_names;
    OffsetArray type_name_type_exprs;
} TypeChecker;

typedef enum {
    TYPE_CHECK_ERROR_UNMATCHED_TYPES,
    TYPE_CHECK_ERROR_EXPECTED_FUNCTION,
    TYPE_CHECK_ERROR_EXPECTED_PRODUCT,
    TYPE_CHECK_ERROR_DOES_NOT_HAVE_FIELD,
} TypeCheckErrorKind;

typedef union {
    struct {
        Type expected;
        Type found;
    } unexpected;
    Type found;
    InternId field;
} TypeCheckErrorData;

typedef struct {
    TypeCheckErrorKind kind;
    TypeCheckErrorData as;
    Span span;
} TypeCheckError;

typedef enum {
    TYPE_CHECK_RESULT_SUCCESS,
    TYPE_CHECK_RESULT_ERROR,
} TypeCheckResultKind;

typedef union {
    Type type;
    TypeCheckError error;
} TypeCheckResultData;

typedef struct {
    TypeCheckResultKind kind;
    TypeCheckResultData as;
} TypeCheckResult;

TypeChecker TypeChecker_new();
void TypeChecker_free(TypeChecker *checker);
TypeCheckResult TypeChecker_collect_types(TypeChecker *checker, OffsetArray *decls);
TypeCheckResult TypeChecker_decls(TypeChecker *checker, OffsetArray *decls);
TypeCheckResult TypeChecker_eval(TypeChecker *checker, Offset type_expr_index);
TypeCheckResult TypeChecker_infer(TypeChecker *checker, Offset expr_index);
TypeCheckResult TypeChecker_expect(TypeChecker *checker, Offset expr_index, Type expected, Span span);
TypeCheckResult TypeChecker_check(TypeChecker *checker, Offset expr_index, Type expected, Span span);
TypeCheckResult TypeChecker_get_global_name(TypeChecker *checker, InternId name);
TypeCheckResult TypeChecker_get_global_type(TypeChecker *checker, InternId name);
void TypeCheckResult_display(TypeCheckError *error, char *input, char *source_name);


TypeCheckResult TypeCheckResult_success();
TypeCheckResult TypeCheckResult_success_type(Type type);
TypeCheckResult TypeCheckResult_error_unmatched(Type expected, Type found, Span span);
TypeCheckResult TypeCheckResult_error_ef(Type found, Span span);
TypeCheckResult TypeCheckResult_error_ep(Type found, Span span);
TypeCheckResult TypeCheckResult_error_no_field(InternId field, Span span);

#endif // TYPE_CHECKER_H