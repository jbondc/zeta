#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "parser.h"
#include "vm.h"

input_t input_from_string(string_t* str)
{
    input_t input;
    input.str = str;
    input.idx = 0;
    return input;
}

/// Test if the end of file has been reached
bool input_eof(input_t* input)
{
    assert (input->str != NULL);
    return (input->idx >= input->str->len);
}

/// Peek at a character from the input
char input_peek_ch(input_t* input)
{
    assert (input->str != NULL);

    if (input->idx >= input->str->len)
        return '\0';

    return input->str->data[input->idx];
}

/// Read a character from the input
char input_read_ch(input_t* input)
{
    char ch = input_peek_ch(input);
    input->idx++;
    return ch;
}

/// Allocate an integer node
ast_int_t* ast_int_alloc(int64_t val)
{
    ast_int_t* node = (ast_int_t*)vm_alloc(sizeof(ast_int_t), DESC_AST_INT);
    node->val = val;
    return node;
}

/**
Parse an identifier
*/
heapptr_t parseIdent(input_t* input)
{
    size_t startIdx = input->idx;
    size_t len = 0;

    for (;;)
    {
        char ch = input_peek_ch(input);

        if (!isalnum(ch) && ch != '$' && ch != '_')
            break;

        // Consume this character
        input_read_ch(input);
        len++;
    }

    if (len == 0)
        return NULL;

    string_t* str = string_alloc(len);

    // Copy the characters
    strncpy(str->data, input->str->data + startIdx, len);

    return (heapptr_t)str;
}

/**
Parse an integer value
*/
heapptr_t parseInt(input_t* input)
{
    size_t numDigits = 0;
    int64_t intVal = 0;

    for (;;)
    {
        char ch = input_peek_ch(input);

        if (!isdigit(ch))
            break;

        intVal = 10 * intVal + (ch - '0');

        // Consume this digit
        input_read_ch(input);
        numDigits++;
    }

    if (numDigits == 0)
        return NULL;

    return (heapptr_t)ast_int_alloc(intVal);
}

/**
Parse a string value
*/
heapptr_t parseStr(input_t* input)
{
    // Strings should begin with a single quote
    char ch = input_read_ch(input);
    if (ch != '\'')
        return NULL;

    size_t len = 0;
    size_t cap = 64;

    char* buf = malloc(cap);

    for (;;)
    {
        // Consume this character
        char ch = input_read_ch(input);

        if (ch == '\'')
            break;

        buf[len] = ch;
        len++;

        if (len == cap)
        {
            cap *= 2;
            char* newBuf = malloc(cap);
            strncpy(newBuf, buf, len);
            free(buf);
        }
    }

    string_t* str = string_alloc(len);

    // Copy the characters
    strncpy(str->data, buf, len);

    return (heapptr_t)str;
}

// TODO: parseExprList
// needed for call

/**
Parse a call expression
ident(x y z ...)
*/
heapptr_t parseCall(input_t* input)
{
    // TODO




}

/**
Parse an atomic expression
*/
heapptr_t parseAtom(input_t* input)
{
    printf("idx=%d\n", input->idx);

    // Consume whitespace
    while (isspace(input_peek_ch(input)))
        input_read_ch(input);

    input_t sub;

    heapptr_t expr;

    // Try parsing an identifier
    sub = *input;
    expr = parseIdent(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Try parsing an integer
    sub = *input;
    expr = parseInt(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Try parsing a string
    sub = *input;
    expr = parseStr(&sub);
    if (expr != NULL)
    {
        *input = sub;
        return expr;
    }

    // Parsing failed
    return NULL;
}

/**
Parse an expression
*/
heapptr_t parseExpr(input_t* input)
{
    // TODO



    return parseAtom(input);



}

