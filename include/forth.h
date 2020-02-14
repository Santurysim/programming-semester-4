#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_WORD 32

// struct forth;
typedef intptr_t cell;

typedef void (*function)(Forth* forth); // TODO

struct word {
    struct word *next;
    bool compiled;
    bool hidden;
    bool immediate;
    uint8_t length;
    char name[];
};

struct forth {
    struct word **executing;
    cell *sp;
    cell *rp;
    cell *memory;
    struct word *latest;
    struct word *stopword;
    bool is_compiling;

    FILE* input;

    cell *memory_free;
    cell *sp0;
    cell *rp0;
    size_t memory_size;
    size_t return_size;
    size_t data_size;
};

class Forth{
	private:
		cell *stackPointer;
		cell *memory;

		Word *latest;
    
		FILE* input;

int forth_init(struct forth *forth, FILE* input,
    size_t memory, size_t stack, size_t ret);
void forth_free(struct forth *forth);

void forth_push(struct forth *forth, cell value);
cell forth_pop(struct forth *forth);
cell* forth_top(struct forth *forth);
void forth_push_return(struct forth *forth, cell value);
cell forth_pop_return(struct forth *forth);
void forth_emit(struct forth *forth, cell value);
void forth_add_codeword(struct forth *forth,
    const char* name, const function handler);
int forth_add_compileword(struct forth *forth,
    const char* name, const char** words);

		Word* addWord(const char *name, uint8_t length);

		// Default words from words.h  TODO
};

void printCell(cell c);

ForthResult readWord(FILE* source,
    char* buffer, size_t bufferSize, size_t* length);
