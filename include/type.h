#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stddef.h>

#include "interner.h"
#include "arena.h"

#define TYPE_ARRAY_DEFAULT_CAPACITY 256

typedef struct Type Type;

typedef struct {
    Type *types;
    size_t capacity;
    size_t length;
} TypeArray;

typedef enum {
    TYPE_ISIZE,
    TYPE_ARROW,
    TYPE_PRODUCT,
    TYPE_FORALL,
    TYPE_KIND,
    TYPE_VAR,
} TypeKind;

typedef union {
    struct {
        Offset input;
        Offset output;
    } function;
    struct {
        StringArray names;
        TypeArray types;
    } product;
    struct {
        InternId variable;
        Offset body_expr;
        Offset kind;
        TypeArray closure;
    } forall;
    struct {
        StringArray symbols;
        Offset kind;
    } type_var;
} TypeData;

struct Type {
    TypeKind kind;
    TypeData as;
};

TypeArray TypeArray_new();
void TypeArray_free(TypeArray *array);
void TypeArray_append(TypeArray *array, Type type);
Type TypeArray_pop(TypeArray *array);
TypeArray TypeArray_clone(TypeArray *array);

Type Type_isize();
Type Type_product(StringArray names, TypeArray types);
Type Type_arrow(Offset input, Offset output);
Type Type_forall(InternId variable, Offset body_expr, Offset kind, TypeArray closure);
Type Type_kind();
Type Type_var(StringArray symbols, Offset kind);
bool Type_eq(Type *lhs, Type *rhs);
void Type_display(Type *type);

#endif // TYPE_H

