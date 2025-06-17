#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "type_checker.h"
#include "arena.h"
#include "expr.h"
#include "bound.h"
#include "decl.h"

TypeChecker TypeChecker_new() {
    return (TypeChecker) {
        .locals = TypeArray_new(),
        .global_names = StringArray_new(),
        .name_type_exprs = OffsetArray_new(),
        .global_type_names = StringArray_new(),
        .type_name_type_exprs = OffsetArray_new(),
    };
}

void TypeChecker_free(TypeChecker *checker) {
    free(checker->locals.types);
    free(checker->global_names.strings);
    free(checker->name_type_exprs.offsets);
    free(checker->global_type_names.strings);
    free(checker->type_name_type_exprs.offsets);
}

TypeCheckResult TypeChecker_collect_types(TypeChecker *checker, OffsetArray *decls) {
        for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, decls->offsets[i]);
        switch (decl->kind) {
            case DECL_BIND: continue;
            case DECL_DECL: {
                DeclDecl *decldecl = &decl->as.decldecl;
                StringArray_append(&checker->global_names, decldecl->name);
                OffsetArray_append(&checker->name_type_exprs, decldecl->type_expr);
                break;
            }
            case DECL_TYPE: {
                DeclType *type = &decl->as.type;
                StringArray_append(&checker->global_type_names, type->name);
                OffsetArray_append(&checker->type_name_type_exprs, type->type_expr);
                break;
            }
        }
    }

    return TypeCheckResult_success();
}

TypeCheckResult TypeChecker_decls(TypeChecker *checker, OffsetArray *decls) {
    DOTypeCheck(TypeChecker_collect_types(checker, decls));
    for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, decls->offsets[i]);
        switch (decl->kind) {
            case DECL_BIND:
                Bind *bind = &decl->as.bind;
                BINDTypeCheck(t, TypeChecker_get_global_name(checker, bind->name));
                DOTypeCheck(TypeChecker_check(checker, bind->expr, t, decl->sign_span));
                break;
            case DECL_DECL:
                DeclDecl *decldecl = &decl->as.decldecl;
                DOTypeCheck(TypeChecker_eval(checker, decldecl->type_expr));
                break;
            case DECL_TYPE:
                DeclType *type = &decl->as.type;
                DOTypeCheck(TypeChecker_eval(checker, type->type_expr));
                break;
        }
    }

    return TypeCheckResult_success();
}

TypeCheckResult TypeChecker_eval(TypeChecker *checker, Offset type_expr_index) {
    TypeExpr *texpr = Arena_get(TypeExpr, type_expr_index);
    switch (texpr->kind) {
        case TYPE_EXPR_IDENTIFIER:
            TypeIdentifier *identifier = &texpr->as.type_ident;
            Bound bound = identifier->bound;
            switch (bound.kind) {
                case BOUND_LOCAL:
                    if (bound.id < 0 || bound.id >= checker->locals.length) {
                        assert(0);
                    }
                    Type local = checker->locals.types[checker->locals.length - 1 - bound.id];
                    return TypeCheckResult_success_type(local);
                case BOUND_GLOBAL: {
                    BINDTypeCheck(t, TypeChecker_get_global_type(checker, bound.id));
                    return TypeCheckResult_success_type(t);
                }
                case BOUND_UNDETERMINED:
                    assert(0);
            }
        case TYPE_EXPR_ARROW:
            TypeArrow *arrow = &texpr->as.type_arrow;
            BINDTypeCheck(from, TypeChecker_eval(checker, arrow->from));
            BINDTypeCheck(to, TypeChecker_eval(checker, arrow->to));
            Offset from_offset = Arena_put(from);
            Offset to_offset = Arena_put(to);
            Type tarrow = Type_arrow(from_offset, to_offset);
            return TypeCheckResult_success_type(tarrow);
        case TYPE_EXPR_PRODUCT:
            TypeProduct *product = &texpr->as.type_product;
            // TODO: Memory leak
            TypeArray types = TypeArray_new();
            StringArray names = StringArray_new();
            for (size_t i = 0; i < product->type_exprs.length; i++) {
                BINDTypeCheck(type, TypeChecker_eval(checker, product->type_exprs.offsets[i]));
                TypeArray_append(&types, type);
                StringArray_append(&names, product->names.strings[i]);
            }
            return TypeCheckResult_success_type(Type_product(names, types));
        case TYPE_EXPR_LAMBDA:
            // TODO: memory leak
            Type forall_type = Type_forall(texpr->as.type_lambda.variable, texpr->as.type_lambda.expr, TypeArray_clone(&checker->locals));
            return TypeCheckResult_success_type(forall_type);
        case TYPE_EXPR_APPLICATION:
            TypeApplication *application = &texpr->as.type_applicaton;
            BINDTypeCheck(function, TypeChecker_eval(checker, application->function));
            if (function.kind != TYPE_FORALL) {
                return TypeCheckResult_error_umk(texpr->sign_span);
            }

            BINDTypeCheck(argument, TypeChecker_eval(checker, application->argument));
            for (size_t i = 0; i < function.as.forall.closure.length; i++) {
                TypeArray_append(&checker->locals, function.as.forall.closure.types[i]);
            }
            TypeArray_append(&checker->locals, argument);

                BINDTypeCheck(result, TypeChecker_eval(checker, function.as.forall.body_expr));

            TypeArray_pop(&checker->locals);
            for (size_t i = 0; i < function.as.forall.closure.length; i++) {
                TypeArray_pop(&checker->locals);
            }

            return TypeCheckResult_success_type(result);
    }
    assert(0);
}

TypeCheckResult TypeChecker_infer(TypeChecker *checker, Offset expr_index) {
    Expr *expr = Arena_get(Expr, expr_index);
    switch (expr->kind) {
        case EXPR_INTEGER:
            return TypeCheckResult_success_type(Type_isize());
        case EXPR_IDENTIFIER:
            Identifier *identifier = &expr->as.identifier;
            Bound bound = identifier->bound;
            switch (bound.kind) {
                case BOUND_LOCAL: {
                    Type t = checker->locals.types[checker->locals.length - 1 - bound.id];
                    return TypeCheckResult_success_type(t);
                }
                case BOUND_GLOBAL: {
                    BINDTypeCheck(t, TypeChecker_get_global_name(checker, bound.id));
                    return TypeCheckResult_success_type(t);
                }
                case BOUND_UNDETERMINED:
                    assert(0);
            }
        case EXPR_BINARY:
            Binary *binary = &expr->as.binary;
            DOTypeCheck(TypeChecker_check(checker, binary->lhs, Type_isize(), expr->sign_span));
            DOTypeCheck(TypeChecker_check(checker, binary->rhs, Type_isize(), expr->sign_span));
            return TypeCheckResult_success_type(Type_isize());
        case EXPR_LET:
            Let* let = &expr->as.let;
            BINDTypeCheck(vtype, TypeChecker_infer(checker, let->vexpr));
            TypeArray_append(&checker->locals, vtype);
                BINDTypeCheck(rtype, TypeChecker_infer(checker, let->rexpr));
            TypeArray_pop(&checker->locals);
            return TypeCheckResult_success_type(rtype);
        case EXPR_LAMBDA:
            assert(0);
        case EXPR_APPLICATION:
            Application *app = &expr->as.application;
            BINDTypeCheck(ftype, TypeChecker_infer(checker, app->function));
            if (ftype.kind != TYPE_ARROW) {
                return TypeCheckResult_error_ef(ftype, expr->sign_span);
            }
            Type *input = Arena_get(Type, ftype.as.function.input);
            Type *output = Arena_get(Type, ftype.as.function.output);

            DOTypeCheck(TypeChecker_check(checker, app->argument, *input, expr->sign_span));
            return TypeCheckResult_success_type(*output);
        case EXPR_PRODUCT:
            Product *product = &expr->as.product;
            // TODO: Memory leak
            TypeArray types = TypeArray_new();
            StringArray names = StringArray_new();
            for (size_t i = 0; i < product->exprs.length; i++) {
                BINDTypeCheck(type, TypeChecker_infer(checker, product->exprs.offsets[i]));
                TypeArray_append(&types, type);
                StringArray_append(&names, product->names.strings[i]);
            }
            return TypeCheckResult_success_type(Type_product(names, types));
        case EXPR_PROJECTION:
            Projection *projection = &expr->as.projection;
            BINDTypeCheck(etype, TypeChecker_infer(checker, projection->expr));
            if (etype.kind != TYPE_PRODUCT) {
                return TypeCheckResult_error_ep(etype, expr->sign_span);
            }
            for (size_t i = 0; i < etype.as.product.names.length; i++) {
                if (etype.as.product.names.strings[i] == projection->name) {
                    return TypeCheckResult_success_type(etype.as.product.types.types[i]);
                }
            }
            return TypeCheckResult_error_no_field(projection->name, expr->sign_span);
    }
    assert(0);
}

TypeCheckResult TypeChecker_check(TypeChecker *checker, Offset expr_index, Type expected, Span span) {
    Expr *expr = Arena_get(Expr, expr_index);
    switch (expr->kind) {
        case EXPR_INTEGER: {
            if (expected.kind == TYPE_ISIZE) {
                return TypeCheckResult_success();
            } else {
                return TypeCheckResult_error_unmatched(expected, Type_isize(), expr->sign_span);
            }
        }
        case EXPR_IDENTIFIER: {
            BINDTypeCheck(t, TypeChecker_infer(checker, expr_index));
            if (Type_eq(&t, &expected)) {
                return TypeCheckResult_success();
            } else {
                return TypeCheckResult_error_unmatched(expected, t, expr->sign_span);
            }
        }
        case EXPR_BINARY: {
            Binary *binary = &expr->as.binary;
            DOTypeCheck(TypeChecker_check(checker, binary->lhs, Type_isize(), expr->sign_span));
            DOTypeCheck(TypeChecker_check(checker, binary->rhs, Type_isize(), expr->sign_span));
            if (expected.kind == TYPE_ISIZE) {
                return TypeCheckResult_success();
            } else {
                // TODO: fix error message
                return TypeCheckResult_error_unmatched(expected, Type_isize(), expr->sign_span);
            }
        }
        case EXPR_LET: {
            BINDTypeCheck(t, TypeChecker_infer(checker, expr_index));
            if (Type_eq(&t, &expected)) {
                return TypeCheckResult_success();
            } else {
                return TypeCheckResult_error_unmatched(expected, t, expr->sign_span);
            }
        }
        case EXPR_LAMBDA: {
            if (expected.kind != TYPE_ARROW) {
                // TODO: fix error message, it is reversed
                return TypeCheckResult_error_ef(expected, expr->sign_span);
            }
            Lambda *lambda = &expr->as.lambda;
            Type *input = Arena_get(Type, expected.as.function.input);
            Type *output = Arena_get(Type, expected.as.function.output);
            TypeArray_append(&checker->locals, *input);
                DOTypeCheck(TypeChecker_check(checker, lambda->expr, *output, span));
            TypeArray_pop(&checker->locals);
            return TypeCheckResult_success();
        }
        case EXPR_APPLICATION: {
            BINDTypeCheck(app, TypeChecker_infer(checker, expr_index))
            if (Type_eq(&app, &expected)) {
                return TypeCheckResult_success();
            } else {
                return TypeCheckResult_error_unmatched(expected, app, expr->sign_span);
            }
        }
        case EXPR_PRODUCT: {
            BINDTypeCheck(product, TypeChecker_infer(checker, expr_index))
            if (Type_eq(&product, &expected)) {
                return TypeCheckResult_success();
            } else {
                return TypeCheckResult_error_unmatched(expected, product, expr->sign_span);
            }
        }
        case EXPR_PROJECTION: {
            Projection *projection = &expr->as.projection;
            BINDTypeCheck(etype, TypeChecker_infer(checker, projection->expr));
            if (etype.kind != TYPE_PRODUCT) {
                return TypeCheckResult_error_ep(etype, expr->sign_span);
            }
            for (size_t i = 0; i < etype.as.product.names.length; i++) {
                if (etype.as.product.names.strings[i] == projection->name) {
                    Type found = etype.as.product.types.types[i];
                    if (Type_eq(&found, &expected)) {
                        return TypeCheckResult_success();
                    } else {
                        return TypeCheckResult_error_unmatched(expected, found, expr->sign_span);
                    }
                }
            }
            return TypeCheckResult_error_no_field(projection->name, expr->sign_span);
        }
    }
    assert(0);
}

TypeCheckResult TypeChecker_get_global_name(TypeChecker *checker, InternId name) {
    for (size_t i = 0; i < checker->global_names.length; i++) {
        if (checker->global_names.strings[i] == name) {
            Offset offset = checker->name_type_exprs.offsets[i];
            return TypeChecker_eval(checker, offset);
        }
    }

    assert(0);
}

TypeCheckResult TypeChecker_get_global_type(TypeChecker *checker, InternId name) {
    for (size_t i = 0; i < checker->global_type_names.length; i++) {
        if (checker->global_type_names.strings[i] == name) {
            Offset offset = checker->type_name_type_exprs.offsets[i];
            return TypeChecker_eval(checker, offset);
        }
    }

    assert(0);
}

void TypeCheckResult_display(TypeCheckError *error, char *source, char *source_name) {
    Span_display_location(&error->span, source_name);
    printf(": error: ");

    switch (error->kind) {
        case TYPE_CHECK_ERROR_UNMATCHED_TYPES:
            printf("Expected '");
            Type_display(&error->as.unexpected.expected);
            printf("' but found '");
            Type_display(&error->as.unexpected.found);
            printf("'");
            break;
        case TYPE_CHECK_ERROR_EXPECTED_FUNCTION:
            printf("Expected a function but found '");
            Type_display(&error->as.found);
            printf("'");
            break;
        case TYPE_CHECK_ERROR_EXPECTED_PRODUCT:
            printf("Expected a product but found '");
            Type_display(&error->as.found);
            printf("'");
            break;
        case TYPE_CHECK_ERROR_DOES_NOT_HAVE_FIELD:
            printf("Product has no field named '");
            printf("%s", Interner_get(error->as.field));
            printf("'");
            break;
        case TYPE_CHECK_ERROR_UNMATCHED_KIND:
            // TODO: better error message here
            printf("Unexpected kind");
            break;
    }

    printf(" (at type checking)\n");
    Span_display_in_source(&error->span, source);
    printf("\n");
}

TypeCheckResult TypeCheckResult_success() {
    return (TypeCheckResult) {
        .kind = TYPE_CHECK_RESULT_SUCCESS,
    };}

TypeCheckResult TypeCheckResult_success_type(Type type) {
    return (TypeCheckResult) {
        .kind = TYPE_CHECK_RESULT_SUCCESS,
        .as.type = type,
    };
}

TypeCheckResult TypeCheckResult_error_unmatched(Type expected, Type found, Span span) {
    return (TypeCheckResult) {
        .kind = TYPE_CHECK_RESULT_ERROR,
        .as.error = {
            .kind = TYPE_CHECK_ERROR_UNMATCHED_TYPES,
            .as.unexpected = {
                .expected = expected,
                .found = found
            },
            .span = span
        }
    };
}

TypeCheckResult TypeCheckResult_error_ef(Type found, Span span) {
    return (TypeCheckResult) {
        .kind = TYPE_CHECK_RESULT_ERROR,
        .as.error = {
            .kind = TYPE_CHECK_ERROR_EXPECTED_FUNCTION,
            .as.found = found,
            .span = span
        }
    };
}

TypeCheckResult TypeCheckResult_error_ep(Type found, Span span) {
    return (TypeCheckResult) {
        .kind = TYPE_CHECK_RESULT_ERROR,
        .as.error = {
            .kind = TYPE_CHECK_ERROR_EXPECTED_PRODUCT,
            .as.found = found,
            .span = span
        }
    };
}

TypeCheckResult TypeCheckResult_error_no_field(InternId field, Span span) {
    return (TypeCheckResult) {
        .kind = TYPE_CHECK_RESULT_ERROR,
        .as.error = {
            .kind = TYPE_CHECK_ERROR_DOES_NOT_HAVE_FIELD,
            .as.field = field,
            .span = span
        }
    };
}

TypeCheckResult TypeCheckResult_error_umk(Span span) {
    return (TypeCheckResult) {
        .kind = TYPE_CHECK_RESULT_ERROR,
        .as.error = {
            .kind = TYPE_CHECK_ERROR_UNMATCHED_KIND,
            .span = span
        }
    };
}
