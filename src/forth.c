#include "forth.h"
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static uint16_t align(uint16_t value, uint8_t alignment);
//static intptr_t strtoiptr(const char* ptr, char** endptr, int base);
static uint16_t strtou16t(const char *ptr, char** endptr, int base);

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
    assert(forth->sp < forth->data_size - 1);
    forth->sp0[forth->sp] = value;
    forth->sp += 1;
}

void forth_iptr_push(struct forth *forth, intptr_t value)
{
    assert(forth->sp < forth->data_size - sizeof(intptr_t)/sizeof(uint16_t));
    *(intptr_t*)(forth->sp0 + forth->sp) = value;
    forth->sp += sizeof(intptr_t)/sizeof(cell);
}

cell forth_pop(struct forth *forth)
{
    assert(forth->sp > 0);
    forth->sp -= 1;
    return *(forth->sp0 + forth->sp);
}

intptr_t forth_iptr_pop(struct forth *forth)
{
    assert(forth->sp >= sizeof(intptr_t) / sizeof(cell));
    forth->sp -= sizeof(intptr_t) / sizeof(cell);
    return *(intptr_t*)(forth->sp0 + forth->sp);
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

void forth_iptr_emit(struct forth *forth, intptr_t value){
    *(intptr_t*)(forth->memory + forth->memory_free) = value;
    forth->memory_free += sizeof(intptr_t)/sizeof(cell);
}

offset forth_top(struct forth *forth) {
    assert(forth->sp > 0);
    return forth->sp - 1;
}

offset word_add(struct forth *forth,
    uint8_t length, const char name[length])
{
    uint16_t word_offset = forth->memory_free;
    struct word* word = (struct word*)(forth->memory + word_offset);
    word->next = forth->latest;
    word->length = length;
    word->hidden = 0;
    word->immediate = 0;
    memcpy(word->name, name, length);
    forth->memory_free = word_code(forth, word_offset);
    assert((char*)(forth->memory + forth->memory_free) >= word->name + length);
    forth->latest = word_offset;
    return word_offset;
}

offset word_code(struct forth *forth, offset word) // TODO
{
    uint16_t size = align(sizeof(struct word) + 1 + ((struct word*)(forth->memory + word))->length, sizeof(cell)) >> 1;
    return (word + size);
}

offset word_find(struct forth *forth, offset word,
    uint8_t length, const char name[length])
{
    while (word) {
        struct word *word_ptr = (struct word*)(forth->memory + word);
        if (!word_ptr->hidden
            && length == word_ptr->length
            && ! strncmp(word_ptr->name, name, length)) {
            return word;
        }
        word = word_ptr->next;
    }
    return 0;
}

static uint16_t align(uint16_t value, uint8_t alignment) // TODO
{
    return ((value - 1) | (alignment - 1)) + 1;
}

void forth_add_codeword(struct forth *forth,
    const char* name, const function handler)
{
    assert(strlen(name) <= 32);
    offset word = word_add(forth, strlen(name), name);
    WORD_PTR(forth, word)->compiled = 0;
    forth_iptr_emit(forth, (intptr_t)handler); // TODO
}


int forth_add_compileword(struct forth *forth,
    const char *name, const char** words)
{
    offset new_word = word_add(forth, strlen(name), name);
    WORD_PTR(forth, new_word)->compiled = 1;
    while (*words) {
        offset word = word_find(forth, forth->latest, strlen(*words), *words);
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
    printf("%"PRId16" ", cell);
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
    uint16_t number = strtou16t(word_buffer, &end, 10); // FIXME: BASE can be internal variable
    if (end - word_buffer < (int)length) {
        fprintf(stderr, "Unknown word: '%.*s'\n", (int)length, word_buffer);
    } else if (!forth->is_compiling) {
        forth_push(forth, number);
    } else {
        offset word = word_find(forth, forth->latest, strlen("lit"), "lit");
        assert(word);
        forth_emit(forth, (cell)word);
        forth_emit(forth, number);
    }
}

static void forth_run_word(struct forth *forth, offset word)
{
    do {
        //printf("%.*s\n", (int)word->length, word->name);
        // FIXME: (1 балл) как избавиться от этого условия
        // и всегда безусловно увеличивать forth->executing на 1?
        if ((offset)forth->memory[forth->executing] != forth->stopword) {
            forth->executing += 1;
        }
        if (!WORD_PTR(forth, word)->compiled) {
            // ISO C forbids conversion of object pointer to function pointer type
            const function code = ((struct { function fn; }*)(forth->memory + word_code(forth, word)))->fn;
            code(forth);
        } else {
            forth_push_return(forth, (cell)forth->executing);
            forth->executing = word_code(forth, word);
        }

        word = forth->memory[forth->executing];
    } while (word != forth->stopword);
}

/*static intptr_t strtoiptr(const char* ptr, char** endptr, int base) {
    if (sizeof(intptr_t) <= sizeof(long)) {
        return (intptr_t)strtol(ptr, endptr, base);
    } else {
        return (intptr_t)strtoll(ptr, endptr, base);
    }
}*/

static uint16_t strtou16t(const char* ptr, char** endptr, int base){
    long res = strtol(ptr, endptr, base);
    if(res > UINT16_MAX){
        *endptr -= 1;
        return 0;
    }
    return (uint16_t)res;
}