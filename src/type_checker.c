#include <assert.h>
#include <stdio.h>

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

TypeCheckResult TypeChecker_collect_types(TypeChecker *checker, OffsetArray *decls, Arena *arena) {
        for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, arena, decls->offsets[i]);
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

TypeCheckResult TypeChecker_decls(TypeChecker *checker, OffsetArray *decls, Arena *arena) {
    DOTypeCheck(TypeChecker_collect_types(checker, decls, arena));
    for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, arena, decls->offsets[i]);
        switch (decl->kind) {
            case DECL_BIND:
                Bind *bind = &decl->as.bind;
                BINDTypeCheck(t, TypeChecker_get_global_name(checker, bind->name, arena));
                DOTypeCheck(TypeChecker_expect(checker, arena, bind->expr, t, decl->sign_span));
                break;
            case DECL_DECL:
                DeclDecl *decldecl = &decl->as.decldecl;
                DOTypeCheck(TypeChecker_eval(checker, arena, decldecl->type_expr));
                break;
            case DECL_TYPE:
                DeclType *type = &decl->as.type;
                DOTypeCheck(TypeChecker_eval(checker, arena, type->type_expr));
                break;
        }
    }

    return TypeCheckResult_success();
}

TypeCheckResult TypeChecker_eval(TypeChecker *checker, Arena *arena, Offset type_expr_index) {
    TypeExpr *texpr = Arena_get(TypeExpr, arena, type_expr_index);
    switch (texpr->kind) {
        case TYPE_EXPR_IDENTIFIER:
            TypeIdentifier *identifier = &texpr->as.type_ident;
            Bound bound = identifier->bound;
            switch (bound.kind) {
                case BOUND_LOCAL:
                    assert(0);
                case BOUND_GLOBAL: {
                    BINDTypeCheck(t, TypeChecker_get_global_type(checker, bound.id, arena));
                    return TypeCheckResult_success_type(t);
                }
                case BOUND_UNDETERMINED:
                    assert(0);
            }
        case TYPE_EXPR_ARROW:
            TypeArrow *arrow = &texpr->as.type_arrow;
            BINDTypeCheck(from, TypeChecker_eval(checker, arena, arrow->from));
            BINDTypeCheck(to, TypeChecker_eval(checker, arena, arrow->to));
            Offset from_offset = Arena_put(arena, from);
            Offset to_offset = Arena_put(arena, to);
            Type tarrow = Type_arrow(from_offset, to_offset);
            return TypeCheckResult_success_type(tarrow);
        case TYPE_EXPR_PRODUCT:
            TypeProduct *product = &texpr->as.type_product;
            // TODO: Memory leak
            TypeArray types = TypeArray_new();
            StringArray names = StringArray_new();
            for (size_t i = 0; i < product->type_exprs.length; i++) {
                BINDTypeCheck(type, TypeChecker_eval(checker, arena, product->type_exprs.offsets[i]));
                TypeArray_append(&types, type);
                StringArray_append(&names, product->names.strings[i]);
            }
            return TypeCheckResult_success_type(Type_product(names, types));
    }
    assert(0);
}

TypeCheckResult TypeChecker_expr(TypeChecker *checker, Arena *arena, Offset expr_index) {
    Expr *expr = Arena_get(Expr, arena, expr_index);
    switch (expr->kind) {
        case EXPR_INTEGER:
            return TypeCheckResult_success_type(Type_isize());
        case EXPR_IDENTIFIER:
            Identifier *identifier = &expr->as.identifier;
            Bound bound = identifier->bound;
            switch (bound.kind) {
                case BOUND_LOCAL: {
                    Type t = checker->locals.types[checker->locals.length - bound.id];
                    return TypeCheckResult_success_type(t);
                }
                case BOUND_GLOBAL: {
                    BINDTypeCheck(t, TypeChecker_get_global_name(checker, bound.id, arena));
                    return TypeCheckResult_success_type(t);
                }
                case BOUND_UNDETERMINED:
                    assert(0);
            }
        case EXPR_BINARY:
            Binary *binary = &expr->as.binary;
            DOTypeCheck(TypeChecker_expect(checker, arena, binary->lhs, Type_isize(), expr->sign_span));
            DOTypeCheck(TypeChecker_expect(checker, arena, binary->rhs, Type_isize(), expr->sign_span));
            return TypeCheckResult_success_type(Type_isize());
        case EXPR_LET:
            Let* let = &expr->as.let;
            BINDTypeCheck(vtype, TypeChecker_expr(checker, arena, let->vexpr));
            TypeArray_append(&checker->locals, vtype);
                BINDTypeCheck(rtype, TypeChecker_expr(checker, arena, let->rexpr));
            TypeArray_pop(&checker->locals);
            return TypeCheckResult_success_type(rtype);
        case EXPR_LAMBDA:
            assert(0);
        case EXPR_APPLICATION:
            Application *app = &expr->as.application;
            BINDTypeCheck(ftype, TypeChecker_expr(checker, arena, app->function));
            BINDTypeCheck(atype, TypeChecker_expr(checker, arena, app->argument));
            if (ftype.kind != TYPE_ARROW) {
                return TypeCheckResult_error_ef(ftype, expr->sign_span);
            }

            Type *input = Arena_get(Type, arena, ftype.as.function.input);
            Type *output = Arena_get(Type, arena, ftype.as.function.output);

            if (Type_eq(input, &atype)) {
                return TypeCheckResult_success_type(*output);
            } else {
                return TypeCheckResult_error_unmatched(*input, atype, expr->sign_span);
            }
        case EXPR_PRODUCT:
            Product *product = &expr->as.product;
            // TODO: Memory leak
            TypeArray types = TypeArray_new();
            StringArray names = StringArray_new();
            for (size_t i = 0; i < product->exprs.length; i++) {
                BINDTypeCheck(type, TypeChecker_expr(checker, arena, product->exprs.offsets[i]));
                TypeArray_append(&types, type);
                StringArray_append(&names, product->names.strings[i]);
            }
            return TypeCheckResult_success_type(Type_product(names, types));
        case EXPR_PROJECTION:
            Projection *projection = &expr->as.projection;
            BINDTypeCheck(etype, TypeChecker_expr(checker, arena, projection->expr));
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

TypeCheckResult TypeChecker_expect(TypeChecker *checker, Arena *arena, Offset expr_index, Type expected, Span span) {
    BINDTypeCheck(found, TypeChecker_expr(checker, arena, expr_index));
    if (Type_eq(&found, &expected)) {
        return TypeCheckResult_success_type(expected);
    } else {
        return TypeCheckResult_error_unmatched(expected, found, span);
    }
}

TypeCheckResult TypeChecker_get_global_name(TypeChecker *checker, InternId name, Arena *arena) {
    for (size_t i = 0; i < checker->global_names.length; i++) {
        if (checker->global_names.strings[i] == name) {
            Offset offset = checker->name_type_exprs.offsets[i];
            return TypeChecker_eval(checker, arena, offset);
        }
    }

    assert(0);
}

TypeCheckResult TypeChecker_get_global_type(TypeChecker *checker, InternId name, Arena *arena) {
    for (size_t i = 0; i < checker->global_type_names.length; i++) {
        if (checker->global_type_names.strings[i] == name) {
            Offset offset = checker->type_name_type_exprs.offsets[i];
            return TypeChecker_eval(checker, arena, offset);
        }
    }

    assert(0);
}

void TypeCheckResult_display(TypeCheckError *error, Interner *interner, char *source, char *source_name) {
    Span_display_location(&error->span, source_name);
    printf(": error: ");

    switch (error->kind) {
        case TYPE_CHECK_ERROR_UNMATCHED_TYPES:
            printf("Expected '");
            printf("' but found '");
            printf("'");
            break;
        case TYPE_CHECK_ERROR_EXPECTED_FUNCTION:
            printf("Expected a function but found '");
            printf("'");
            break;
        case TYPE_CHECK_ERROR_EXPECTED_PRODUCT:
            printf("Expected a product but found '");
            printf("'");
            break;
        case TYPE_CHECK_ERROR_DOES_NOT_HAVE_FIELD:
            printf("Product has no field names '");
            printf("%s", Interner_get(interner, error->as.field));
            printf("'");
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
