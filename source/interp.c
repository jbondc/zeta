#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "interp.h"
#include "parser.h"
#include "vm.h"

#define MAX_LOCALS 64

/**
Interpreter stack frame
*/
typedef struct
{
    size_t numLocals;

    value_t locals[MAX_LOCALS];

} frame_t;

/// Evaluate the boolean value of a value
bool eval_truth(value_t value)
{
    switch (value.tag)
    {
        case TAG_FALSE:
        return false;

        case TAG_TRUE:
        return true;

        case TAG_INT64:
        return value.word.int64 != 0;

        case TAG_FLOAT64:
        return value.word.float64 != 0;

        case TAG_RAW_PTR:
        return value.word.heapptr != 0;

        case TAG_STRING:
        return ((string_t*)value.word.heapptr)->len > 0;

        case TAG_ARRAY:
        return ((array_t*)value.word.heapptr)->len > 0;

        default:
        return true;
    }
}

value_t eval_expr(heapptr_t expr)
{
    // Switch on the expression's tag
    switch (get_tag(expr))
    {
        case TAG_AST_CONST:
        {
            ast_const_t* cst = (ast_const_t*)expr;
            return cst->val;
        }

        case TAG_STRING:
        {
            return value_from_heapptr(expr);
        }

        // Array literal expression
        case TAG_ARRAY:
        {
            // TODO: must create new array with evaluated expressions

            assert (false);
        }

        // Binary operator (e.g. a + b)
        case TAG_AST_BINOP:
        {
            ast_binop_t* binop = (ast_binop_t*)expr;

            value_t v0 = eval_expr(binop->left_expr);
            value_t v1 = eval_expr(binop->right_expr);
            int64_t i0 = v0.word.int64;
            int64_t i1 = v1.word.int64;

            if (binop->op == &OP_INDEX)
                return array_get((array_t*)v0.word.heapptr, i1);

            if (binop->op == &OP_ADD)
                return value_from_int64(i0 + i1);
            if (binop->op == &OP_SUB)
                return value_from_int64(i0 - i1);
            if (binop->op == &OP_MUL)
                return value_from_int64(i0 * i1);
            if (binop->op == &OP_DIV)
                return value_from_int64(i0 / i1);
            if (binop->op == &OP_MOD)
                return value_from_int64(i0 % i1);

            if (binop->op == &OP_LT)
                return (i0 < i1)? VAL_TRUE:VAL_FALSE;
            if (binop->op == &OP_LE)
                return (i0 <= i1)? VAL_TRUE:VAL_FALSE;
            if (binop->op == &OP_GT)
                return (i0 > i1)? VAL_TRUE:VAL_FALSE;
            if (binop->op == &OP_GE)
                return (i0 >= i1)? VAL_TRUE:VAL_FALSE;

            if (binop->op == &OP_EQ)
                return (memcmp(&v0, &v1, sizeof(v0)) == 0)? VAL_TRUE:VAL_FALSE;
            if (binop->op == &OP_NE)
                return (memcmp(&v0, &v1, sizeof(v0)) != 0)? VAL_TRUE:VAL_FALSE;
        }

        // If expression
        case TAG_AST_IF:
        {
            ast_if_t* ifexpr = (ast_if_t*)expr;

            value_t t = eval_expr(ifexpr->test_expr);

            if (eval_truth(t))
                return eval_expr(ifexpr->then_expr);
            else
                return eval_expr(ifexpr->else_expr);
        }

        // Call expression
        case TAG_AST_CALL:
        {
            ast_call_t* callexpr = (ast_call_t*)expr;
            heapptr_t fun_expr = callexpr->fun_expr;
            array_t* arg_exprs = callexpr->arg_exprs;

            if (get_tag(fun_expr) == TAG_STRING && arg_exprs->len == 1)
            {
                string_t* fun_name = (string_t*)fun_expr;
                char* name_cstr = fun_name->data;

                heapptr_t arg_expr = array_get(arg_exprs, 0).word.heapptr;
                value_t arg_val = eval_expr(arg_expr);
                string_t* arg_str = (string_t*)arg_val.word.heapptr;

                if (strncmp(name_cstr, "$print_i64", 10) == 0)
                {
                    printf("%ld", arg_val.word.int64);
                    return VAL_TRUE;
                }

                if (strncmp(name_cstr, "$print_str", 10) == 0)
                {
                    string_print(arg_str);
                    return VAL_TRUE;
                }
            }

            assert (false);
        }

        // Function/closure expression
        case TAG_AST_FUN:
        {
            // For now, return the function unchanged
            return value_from_heapptr(expr);
        }

        // TODO: use special error value, not accessible to user code
        // run_error_t
        default:
        printf("unknown tag=%ld\n", get_tag(expr));
        return VAL_FALSE;
    }
}

/// Evaluate the code in a given string
value_t eval_str(char* cstr)
{
    size_t len = strlen(cstr);

    // Allocate a hosted string object
    string_t* str = string_alloc(len);

    strncpy(str->data, cstr, len);

    // Create a parser input stream object
    input_t input = input_from_string(str);

    // Until the end of the input is reached
    for (;;)
    {
        // Parse one expression
        heapptr_t expr = parse_expr(&input);

        if (expr == NULL)
        {
            char buf[64];
            printf(
                "failed to parse expression, at %s\n",
                srcpos_to_str(input.pos, buf)
            );

            return VAL_FALSE;
        }

        // Evaluate the expression
        value_t value = eval_expr(expr);

        // If this is the end of the input, stop
        input_eat_ws(&input);
        if (input_eof(&input))
            return value;
    }
}

void test_eval(char* cstr, value_t expected)
{
    value_t value = eval_str(cstr);

    if (memcmp(&value, &expected, sizeof(value)) != 0)
    {
        printf(
            "value doesn't match expected for input:\n%s\n",
            cstr
        );

        exit(-1);
    }
}

void test_eval_int(char* cstr, int64_t expected)
{
    test_eval(cstr, value_from_int64(expected));
}

void test_eval_true(char* cstr)
{
    test_eval(cstr, VAL_TRUE);
}

void test_eval_false(char* cstr)
{
    test_eval(cstr, VAL_FALSE);
}

void test_interp()
{
    test_eval_int("0", 0);
    test_eval_int("7", 7);
    test_eval_int("0xFF", 255);
    test_eval_int("0b101", 5);
    test_eval_true("true");
    test_eval_false("false");

    // Comparisons
    test_eval_true("0 < 5");
    test_eval_true("0 <= 5");
    test_eval_true("0 <= 0");
    test_eval_true("0 == 0");
    test_eval_true("0 != 1");
    test_eval_true("true == true");
    test_eval_false("true == false");

    // If expression
    test_eval_int("if true then 1 else 0", 1);
    test_eval_int("if false then 1 else 0", 0);
    test_eval_int("if 0 < 10 then 7 else 3", 7);
    test_eval_int("if 0 then 1 else 0", 0);
    test_eval_int("if 1 then 777", 777);
    test_eval_int("if 'abc' then 777", 777);
    test_eval_int("if '' then 777 else 0", 0);







}

