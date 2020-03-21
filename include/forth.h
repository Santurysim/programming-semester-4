#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct forth;
typedef uint16_t cell;
typedef uint16_t offset;

#define WORD_PTR(forth, word_offset) ((struct word*)((forth)->memory + (word_offset)))

#define MAX_WORD 32

struct word {
    offset next;
    uint8_t compiled;
    uint8_t hidden;
    uint8_t immediate;
    uint8_t length;
    char name[];
};

struct forth {
    offset executing;
    offset memory_free;
    offset sp;
    offset rp;
    offset latest;
    offset stopword;
    uint8_t is_compiling;

    FILE* input;

    cell *memory;
    cell *sp0;
    cell *rp0;
    uint16_t memory_size;
    uint16_t return_size;
    uint16_t data_size;
};


offset word_add(struct forth *forth,
    uint8_t length, const char name[length]);

offset word_code(struct forth *forth, offset word);

offset word_find(struct forth *forth, offset first,
    uint8_t length, const char name[length]);

typedef void (*function)(struct forth *forth);

int forth_init(struct forth *forth, FILE* input,
    uint16_t memory, uint16_t stack, uint16_t ret);

void forth_free(struct forth *forth);

void forth_push(struct forth *forth, cell value);

cell forth_pop(struct forth *forth);

offset forth_top(struct forth *forth);

void forth_iptr_push(struct forth *forth, intptr_t value);

intptr_t forth_iptr_pop(struct forth *forth);

void forth_push_return(struct forth *forth, cell value);

cell forth_pop_return(struct forth *forth);

void forth_emit(struct forth *forth, cell value);

void forth_iptr_emit(struct forth *forth, intptr_t value);

void forth_add_codeword(struct forth *forth,
    const char* name, const function handler);

int forth_add_compileword(struct forth *forth,
    const char* name, const char** words);

void cell_print(cell c);

enum forth_result {
    FORTH_OK,
    FORTH_EOF,
    FORTH_WORD_NOT_FOUND,
    FORTH_BUFFER_OVERFLOW,
};

enum forth_result read_word(FILE* source,
    size_t buffer_size, char buffer[buffer_size], size_t *length);

enum forth_result forth_run(struct forth* forth);