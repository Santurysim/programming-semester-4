#include "forth.h"
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

static uintptr_t align(uintptr_t value, uint8_t alignment);
static intptr_t strtoiptr(const char* ptr, char** endptr, int base);

int forth_init(struct forth *forth, FILE *input, FILE *output,
    size_t memory, size_t stack, size_t ret){
    forth->memory_size = memory;
    forth->memory = malloc(forth->memory_size * sizeof(cell));
    forth->memory_free = forth->memory;

    forth->data_size = stack;
    forth->sp0 = malloc(forth->data_size * sizeof(cell));
    forth->sp = forth->sp0;

    forth->return_size = ret;
    forth->rp0 = malloc(forth->return_size * sizeof(cell));
    forth->rp = forth->rp0;

    forth->latest = NULL;
    forth->executing = NULL;
    forth->is_compiling = false;
    forth->input = input;
    forth->output = output;
    forth_configure_io(forth);

    return forth->memory == NULL || forth->sp0 == NULL || forth->rp0 == NULL;
}

void forth_set_input(struct forth *forth, FILE* input){
    forth->input = input;
    forth_configure_io(forth);
}

void forth_set_output(struct forth *forth, FILE* output){
    forth->output = output;
    forth_configure_io(forth);
}

void forth_configure_io(struct forth *forth){
    if(isatty(fileno(forth->input)) && isatty(fileno(forth->output))){
        forth->prompt = "cforth> ";
        forth->read_word_func = rl_read_word;
    }
    else{
        forth->prompt = "";
        forth->read_word_func = read_word;
    }    
}

void forth_free(struct forth *forth){
    free(forth->sp0);
    free(forth->memory);
    free(forth->rp0);
    *forth = (struct forth) {0};
}

void forth_push(struct forth *forth, cell value){
    assert(forth->sp < forth->sp0 + forth->data_size);
    *(forth->sp) = value;
    forth->sp += 1;
}

cell forth_pop(struct forth *forth){
    assert(forth->sp > forth->sp0);
    forth->sp -= 1;
    return *forth->sp;
}

void forth_push_return(struct forth *forth, cell value){
    assert(forth->rp < forth->rp0 + forth->return_size);
    forth->rp[0] = value;
    forth->rp += 1;
    //printf(">r %lx %ld\n", value, forth->rp - forth->rp0);
}

cell forth_pop_return(struct forth *forth) {
    assert(forth->rp > forth->rp0);
    forth->rp -= 1;
    //printf("r> %lx %ld\n", forth->rp[0], forth->rp - forth->rp0);
    return forth->rp[0];
}

void forth_emit(struct forth *forth, cell value){
    *(forth->memory_free) = value;
    forth->memory_free += 1;
}

cell* forth_top(struct forth *forth) {
    return forth->sp-1;
}

struct word* word_add(struct forth *forth,
    uint8_t length, const char name[length]){
    struct word* word = (struct word*)forth->memory_free;
    word->next = forth->latest;
    word->length = length;
    word->hidden = false;
    word->immediate = false;
    memcpy(word->name, name, length);
    forth->memory_free = (cell*)word_code(word);
    assert((char*)forth->memory_free >= word->name + length);
    forth->latest = word;
    return word;
}

const void* word_code(const struct word *word){
    uintptr_t size = align(sizeof(struct word) + 1 + word->length, sizeof(cell));
    return (const void*)((uint8_t*)word + size);
}

const struct word* word_find(const struct word* word,
    uint8_t length, const char name[length]){
    while (word) {
        if (!word->hidden
            && length == word->length
            && ! strncmp(word->name, name, length)) {
            return word;
        }
        word = word->next;
    }
    return NULL;
}

static uintptr_t align(uintptr_t value, uint8_t alignment){
    return ((value - 1) | (alignment - 1)) + 1;
}

void forth_add_codeword(struct forth *forth,
    const char* name, const function handler){
    struct word *word = word_add(forth, strlen(name), name);
    word->compiled = false;
    assert(strlen(name) <= 32);
    forth_emit(forth, (cell)handler);
}


int forth_add_compileword(struct forth *forth,
    const char *name, const char** words){
    struct word *word = word_add(forth, strlen(name), name);
    word->compiled = true;
    word->hidden = true;
    while (*words) {
        const struct word* word = word_find(forth->latest, strlen(*words), *words);
        if (!word) {
            return 1;
        }
        // printf("Compile %s, writing %s as %p\n", name, *words, (void*)word);
        forth_emit(forth, (cell)word);
        words += 1;
    }
    word->hidden = false;
    return 0;
}

void cell_print(FILE *output, cell cell) {
    fprintf(output, "%"PRIdPTR" ", cell);
}

enum forth_result read_word(FILE *source, const char *ignore,
    size_t buffer_size, char buffer[buffer_size], size_t *length){
    (void)ignore;
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

enum forth_result rl_read_word(FILE *source, const char *prompt,
    size_t buffer_size, char buffer[buffer_size], size_t *length){
    static char *line_read = NULL;
    static char *token = NULL;
    size_t l;
    rl_instream = source;
    if(token)
        token = strtok(NULL, " \t");
    if(!token){
        do{
            if(line_read) free(line_read);
            line_read = readline(prompt);
            if(!line_read)
                return FORTH_EOF;
            add_history(line_read);
            token = strtok(line_read, " \t");
        }while(!*line_read || !token);
    }
    l = strlen(token);
    if(l >= buffer_size){
        strncpy(buffer, token, buffer_size);
        return FORTH_BUFFER_OVERFLOW;
    }else{
        stpncpy(buffer, token, l);
        buffer[l] = 0;
        *length = l;
        return FORTH_OK;
    }
}

static void forth_run_word(struct forth *forth, const struct word *word);
static void forth_run_number(struct forth *forth,
    size_t length, const char word_buffer[length]);

enum forth_result forth_run(struct forth* forth){
    size_t length;
    enum forth_result read_result;
    char word_buffer[MAX_WORD+1] = {0};

    while ((read_result = forth->read_word_func(
            forth->input,
            forth->prompt,
            sizeof(word_buffer),
            word_buffer,
            &length
        )) == FORTH_OK) {

        const struct word* word = word_find(forth->latest, length, word_buffer);
        if (word == NULL) {
            forth_run_number(forth, length, word_buffer);
        } else if (word->immediate || !forth->is_compiling) {
            forth_run_word(forth, word);
        } else {
            forth_emit(forth, (cell)word);
        }
    }
    return read_result;
}

static void forth_run_number(struct forth *forth,
    size_t length, const char word_buffer[length]){
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

static void forth_run_word(struct forth *forth, const struct word *word){
	// Перед циклом и на первой итерации: *forth->executing == forth->stopword ???
	// Инвариант цикла: word указывает на исполняемое слово,
	// forth->executing - на следующее
    do {
        if (!word->compiled) {
            // ISO C forbids conversion of object pointer to function pointer type
            const function code = ((struct { function fn; }*)word_code(word))->fn;
            code(forth);
        } else {
            forth_push_return(forth, (cell)forth->executing);
            forth->executing = (struct word**)word_code(word);
        }

        word = *forth->executing;
        forth->executing += 1;
    } while (word != forth->stopword);
    forth->executing = &forth->stopword;
}

static intptr_t strtoiptr(const char* ptr, char** endptr, int base) {
#if INTPTR_MAX < LONG_MAX
        return (intptr_t)strtol(ptr, endptr, base);
#else 
        return (intptr_t)strtoll(ptr, endptr, base);
#endif
}
