#include "forth.h"
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static uint16_t align(uint16_t value, uint16_t alignment);
static intptr_t strtoiptr(const char* ptr, char** endptr, int base);

int forth_init(struct forth *forth, FILE *input,
    uint16_t memory, uint16_t stack, uint16_t ret)
{
    forth->memory_size = memory;
    forth->memory = malloc(forth->memory_size * sizeof(cell));
    forth->memory_free = 0;

    forth->data_size = stack;
    forth->sp0 = malloc(forth->data_size * sizeof(cell));
    forth->sp = 0;

    forth->return_size = ret;
    forth->rp0 = malloc(forth->return_size * sizeof(cell));
    forth->rp = 0;

    forth->latest = 0;
    forth->executing = 0;
    forth->is_compiling = 0;
    forth->input = input;

    return forth->memory == NULL || forth->sp0 == NULL;
}

void forth_free(struct forth *forth)
{
    free(forth->sp0);
    free(forth->memory);
    free(forth->rp0);
    *forth = (struct forth) {0};
}

void forth_push(struct forth *forth, cell value)
{
    assert(forth->sp < forth->data_size);
    forth->sp0[forth->sp] = value;
    forth->sp += 1;
}

cell forth_pop(struct forth *forth)
{
    assert(forth->sp > 0);
    forth->sp -= 1;
    return *(forth->sp0 + forth->sp);
}

void forth_push_return(struct forth *forth, cell value)
{
    assert(forth->rp < forth->return_size);
    forth->rp0[forth->rp] = value;
    forth->rp += 1;
    //printf(">r %lx %ld\n", value, forth->rp - forth->rp0);
}

cell forth_pop_return(struct forth *forth) {
    assert(forth->rp > 0);
    forth->rp -= 1;
    //printf("r> %lx %ld\n", forth->rp[0], forth->rp - forth->rp0);
    return forth->rp0[forth->rp];
}

void forth_emit(struct forth *forth, cell value)
{
    forth->memory[forth->memory_free] = value;
    forth->memory_free += 1;
}

offset forth_top(struct forth *forth) {
    return forth->sp-1;
}

struct word* word_add(struct forth *forth,
    uint8_t length, const char name[length])
{
    uint16_t word_offset = forth->memory_free;
    struct word* word = (struct word*)(forth->memory[forth->memory_free]);
    word->next = forth->latest;
    word->length = length;
    word->hidden = 0;
    word->immediate = 0;
    memcpy(word->name, name, length);
    forth->memory_free = word_code(forth, word_offset);
    assert((char*)(forth->memory[forth->memory_free]) >= word->name + length);
    forth->latest = word_offset;
    return word;
}

offset word_code(struct forth *forth, offset word) // TODO
{
    uint16_t size = align(sizeof(struct word) + 1 + ((struct word*)(forth->memory + word))->length, sizeof(cell));
    return (word + size);
}

offset word_find(struct forth *forth, offset word,
    uint8_t length, const char name[length])
{
    while (word) {
        struct word *word_ptr = (struct word*)(forth->memory + word)
        if (!word_ptr->hidden
            && length == word_ptr->length
            && ! strncmp(word-ptr->name, name, length)) {
            return word;
        }
        word = word_ptr->next;
    }
    return NULL;
}

static uintptr_t align(uintptr_t value, uint8_t alignment) // TODO
{
    return ((value - 1) | (alignment - 1)) + 1;
}

void forth_add_codeword(struct forth *forth,
    const char* name, const function handler)
{
    offset word = word_add(forth, strlen(name), name);
    WORD_PTR(forth, word)->compiled = 0;
    assert(strlen(name) <= 32);
    forth_emit(forth, (cell)handler); // TODO
}


int forth_add_compileword(struct forth *forth,
    const char *name, const char** words)
{
    offset new_word = word_add(forth, strlen(name), name);
    WORD_PTR(forth, new_word)->compiled = 1;
    while (*words) {
        offset word = word_find(forth->latest, strlen(*words), *words);
        if (!word) {
            return 1;
        }
        // printf("Compile %s, writing %s as %p\n", name, *words, (void*)word);
        forth_emit(forth, (cell)word);
        words += 1;
    }
    return 0;
}

void cell_print(cell cell) {
    printf("%"PRId8" ", cell);
}

enum forth_result read_word(FILE* source,
    size_t buffer_size, char buffer[buffer_size], size_t *length)
{
    size_t l = 0;
    int c; 
    while ((c = fgetc(source)) != EOF && l < buffer_size) {
        // isspace(c) -> l == 0
        if (isspace(c)) {
            if (l == 0) {
                continue;
            } else {
                break;
            }
        }
        buffer[l] = c;
        l += 1;
    }

    if (l > 0 && l < buffer_size) {
        *length = l;
        buffer[l] = 0;
        return FORTH_OK;
    }

    if (l >= buffer_size) {
        return FORTH_BUFFER_OVERFLOW;
    }
    
    return FORTH_EOF;
}

static void forth_run_word(struct forth *forth, offset word);
static void forth_run_number(struct forth *forth,
    size_t length, const char word_buffer[length]);

enum forth_result forth_run(struct forth* forth)
{
    size_t length;
    enum forth_result read_result;
    char word_buffer[MAX_WORD+1] = {0};

    while ((read_result = read_word(
            forth->input,
            sizeof(word_buffer),
            word_buffer,
            &length
        )) == FORTH_OK) {

        offset word = word_find(forth, forth->latest, length, word_buffer);
        if (word == 0) {
            forth_run_number(forth, length, word_buffer);
        } else if (WORD_PTR(forth, word)->immediate || !forth->is_compiling) {
            forth_run_word(forth, word);
        } else {
            forth_emit(forth, (cell)word);
        }
    }
    return read_result;
}

static void forth_run_number(struct forth *forth,
    size_t length, const char word_buffer[length])
{
    char* end;
    intptr_t number = strtoiptr(word_buffer, &end, 10); // FIXME: BASE can be internal variable
    if (end - word_buffer < (int)length) {
        fprintf(stderr, "Unknown word: '%.*s'\n", (int)length, word_buffer);
    } else if (!forth->is_compiling) {
        forth_push(forth, number);
    } else {
        const struct word* word = word_find(forth->latest, strlen("lit"), "lit");
        assert(word);
        forth_emit(forth, (cell)word);
        forth_emit(forth, number);
    }
}

static void forth_run_word(struct forth *forth, const struct word *word)
{
    do {
        //printf("%.*s\n", (int)word->length, word->name);
        // FIXME: (1 балл) как избавиться от этого условия
        // и всегда безусловно увеличивать forth->executing на 1?
        if (*forth->executing != forth->stopword) {
            forth->executing += 1;
        }
        if (!word->compiled) {
            // ISO C forbids conversion of object pointer to function pointer type
            const function code = ((struct { function fn; }*)word_code(word))->fn;
            code(forth);
        } else {
            forth_push_return(forth, (cell)forth->executing);
            forth->executing = (struct word**)word_code(word);
        }

        word = *forth->executing;
    } while (word != forth->stopword);
}

static intptr_t strtoiptr(const char* ptr, char** endptr, int base) {
    if (sizeof(intptr_t) <= sizeof(long)) {
        return (intptr_t)strtol(ptr, endptr, base);
    } else {
        return (intptr_t)strtoll(ptr, endptr, base);
    }
}