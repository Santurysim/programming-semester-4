#include "forth.h"
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static uintptr_t align(uintptr_t value, uint8_t alignment);
static intptr_t strtoiptr(const char* ptr, char** endptr, int base);

int forth_init(struct forth *forth, FILE *input,
    size_t memory, size_t stack, size_t ret)
{
    forth->memory_size = memory;
    forth->memory = malloc(forth->memory_size * sizeof(cell));
    forth->memory_free = forth->memory;

// Forth class

    forth->return_size = ret;
    forth->rp0 = malloc(forth->return_size * sizeof(cell));
    forth->rp = forth->rp0;

    forth->latest = NULL;
    forth->executing = NULL;
    forth->is_compiling = false;
    forth->input = input;

	this->sp0 = new cell[_memorySize];
	this->stackPointer = this->sp0;
	this->latest = NULL;
	
	if(!(this->memory) || !(this->sp0))
		throw ForthException();
}

Forth::~Forth(){
	delete [] this->sp0;
	delete [] this->memory;
}

void Forth::push(cell value){
	// code and data should not intersect?
	assert(this->stackPointer < this->sp0 + this->dataSize);
	*(this->stackPointer) = value;
	this->stackPointer += 1;
}

cell Forth::pop(){
	assert(this->stackPointer > this->sp0);
	this->stackPointer -= 1;
	return this->stackPointer;
}

void forth_push_return(struct forth *forth, cell value)
{
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

void forth_emit(struct forth *forth, cell value)
{
    *(forth->memory_free) = value;
    forth->memory_free += 1;
}

void Forth::emit(cell value){
	*(this->freeMemory) = value;
	this->freeMemory += 1;
}

Word* Forth::addWord(const char *name, uint8_t length){
	// TODO
}

struct word* word_add(struct forth *forth,
    uint8_t length, const char name[length])
{
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

ForthResult Forth::run(){
	size_t length;
	ForthResult readResult;
	char wordBuffer[MAX_WORD + 1] = {0};
	while((readResult = readWord(this->input, wordBuffer, 
					sizeof(wordBuffer), &length)) == FORTH_OK){
		const Word *word = this->latest.find(wordBuffer, length);
		if(!word)
			this->runNumber(wordBuffer, length);
		else{
            // ISO C forbids conversion of object pointer to function pointer type
            const function code = ((struct { function fn; }*)word->getCode())->fn; // TODO
		}
	}
	return readResult;

}

const struct word* word_find(const struct word* word,
    uint8_t length, const char name[length])
{
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

void Forth::addCodeword(const char *name. const function handler){
	this->addWord(name, strlen(name));
	assert(strlen(name) < 32);
	this->emit(forth(cell)handler);
}

void forth_add_codeword(struct forth *forth,
    const char* name, const function handler)
{
    struct word *word = word_add(forth, strlen(name), name);
    word->compiled = false;
    assert(strlen(name) <= 32);
    forth_emit(forth, (cell)handler);
}

// End of Word implementation

// Miscellaneous functions

int forth_add_compileword(struct forth *forth,
    const char *name, const char** words)
{
    struct word *word = word_add(forth, strlen(name), name);
    word->compiled = true;
    while (*words) {
        const struct word* word = word_find(forth->latest, strlen(*words), *words);
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
    printf("%"PRIdPTR" ", cell);
}

ForthResult readWord(FILE* source,
    char* buffer, size_t bufferSize, size_t* length){
    size_t l = 0;
    int c; 
    while ((c = fgetc(source)) != EOF && l < bufferSize) {
        // isspace(c) → l == 0
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

    if (l > 0 && l < bufferSize) {
        *length = l;
        buffer[l] = 0;
        return FORTH_OK;
    }

    if (l >= bufferSize) {
        return FORTH_BUFFER_OVERFLOW;
    }
    
    return FORTH_EOF;
}

static void forth_run_word(struct forth *forth, const struct word *word);
static void forth_run_number(struct forth *forth,
    size_t length, const char word_buffer[length]);

static void forth_run_number(struct forth *forth,
    size_t length, const char word_buffer[length])
{
    char* end;
    intptr_t number = strtoiptr(word_buffer, &end, 10); // FIXME: BASE can be internal variable
    if (end - word_buffer < (int)length) {
        fprintf(stderr, "Unknown word: '%.*s'\n", (int)length, word_buffer);
    } else {
        forth_push(forth, number);
    }
}

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

void forth_add_codeword(struct forth *forth,
    const char* name, const function handler)
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
    return NULL;
}
*/
