#include <ctype.h>
#include <inttypes.h>
#include <iso646.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "forth.h"
#include "words.h"

static uintptr_t align(uintptr_t value, uint8_t alignment);
static intptr_t strtoiptr(const char* ptr, char** endptr, int base);

// C++ implementation

// Forth class

// Constructor and destructor

Forth::Forth(FILE *_input, size_t _memorySize, size_t _stackSize, size_t _returnStackSize):
	input(_input), memorySize(_memorySize), dataSize(_stackSize), returnStackSize(_returnStackSize){
	this->memory = new cell[_memorySize];
	this->freeMemory = this->memory;

	this->stackBottom = new cell[_stackSize];
	this->stackPointer = this->stackBottom;

	this->returnStackBottom = new cell[_returnStackSize];
	this->returnStackPointer = this->returnStackBottom;

	this->latest = NULL;
	this->executing = NULL;
	this->compiling = false;
	
	if(!(this->memory) || !(this->stackBottom))
		throw ForthException();
}

Forth::~Forth(){
	delete [] this->stackBottom;
	delete [] this->memory;
	delete [] this->returnStackBottom;
}

void Forth::addMachineWords(){
	this->addCodeword("drop", drop);
    this->addCodeword("dup", _dup);
    this->addCodeword("+", add);
    this->addCodeword("-", sub);
    this->addCodeword("*", mul);
    this->addCodeword("/", _div);
    this->addCodeword("%", mod);
    this->addCodeword("swap", swap);
    this->addCodeword("rot", rot);
    this->addCodeword("-rot", rot_back);
    this->addCodeword("show", show);
    this->addCodeword("over", over);
    this->addCodeword("true", _true);
    this->addCodeword("false", _false);
    this->addCodeword("xor", _xor);
    this->addCodeword("or", _or);
    this->addCodeword("and", _and);
    this->addCodeword("not", _not);
    this->addCodeword("=", _eq);
    this->addCodeword("<", lt);
    this->addCodeword("within", within);
}

// Data stack management

void Forth::push(cell value){
	// Ensure we have room for new data
	if(this->stackPointer == this->stackBottom + this->dataSize)
		throw ForthOutOfMemoryException();
	*(this->stackPointer) = value;
	this->stackPointer += 1;
}

cell Forth::pop(){
	if (this->stackPointer == this->stackBottom){
		throw ForthEmptyStackException();
	}
	
	this->stackPointer -= 1;
	return *this->stackPointer;
}

cell* Forth::top(){
	return this->stackPointer - 1;
}

// Word management

void Forth::addCodeword(const char *name, const function handler){
	if(strlen(name) >= 32)
		throw ForthIllegalArgumentException();
	if((uint8_t*)this->freeMemory + align(sizeof(Word) + 1 + strlen(name), sizeof(cell)) >
		(uint8_t*)(this->memory + this->memorySize)){
		throw ForthOutOfMemoryException();
	}
	this->addWord(name, strlen(name), false);
	this->emit((cell)handler);
}

void Forth::emit(cell value){
	*(this->freeMemory) = value;
	this->freeMemory += 1;
}

Word* Forth::addWord(const char *name, uint8_t length, bool isCompiled){
	Word newWord(this->latest, isCompiled);
	Word *word = reinterpret_cast<Word*>(this->freeMemory);
	*word = newWord;
	word->setName(name, length);
    this->freeMemory = (cell*)(word->getCode());
	this->latest = word;
	return word;
}

int Forth::addCompiledWord(const char *name, const char **words){
	this->addWord(name, strlen(name), true);
	while(*words) {
		const Word *word = this->latest->find(*words, strlen(*words));
		if(!word) {
			return 1;
		}
		this->emit((cell)word);
		word += 1;
	}
	return 0;
}

// Executing

ForthResult Forth::run(){
	size_t length;
	ForthResult readResult;
	char wordBuffer[MAX_WORD + 1] = {0};
	while((readResult = readWord(this->input, wordBuffer, 
					sizeof(wordBuffer), &length)) == FORTH_OK){
		const Word *word = this->latest->find(wordBuffer, length);
		if(!word)
			this->runNumber(wordBuffer, length);
        else if(word->isImmediate() || !this->compiling)
            this->runWord(word);
		else
            this->emit((cell)word);
	}
	return readResult;

}

void Forth::runNumber(const char *wordBuffer, size_t length){
	char *end;
	intptr_t number = strtoiptr(wordBuffer, &end, 10); // TODO
	if(end - wordBuffer < (int)length){
		fprintf(stderr, "Unknown word: '%.*s'\n", (int)length, wordBuffer);
	} else if(!this->compiling)
		this->push(number);
    else{
        const Word *word = this->latest->find("lit", strlen("lit"));
        if(!word)
            throw ForthIllegalStateException();
        this->emit((cell)word);
        this->emit(number);
    }
}

void Forth::runWord(const Word* word){
    do{
        if(*this->executing != this->stopWord)
            this->executing += 1;
        if(!word->isCompiled()){
            // ISO C forbids conversion of object pointer to function pointer type
            // But C++ does not
            const function code = *(function*)word->getCode();
			code(*this);
        } else{
            this->pushReturn((cell)this->executing);
            this->executing = (Word**)word->getCode();
        }
    } while(word != this->stopWord);
}

// TODO: just mark some word handlers friends

cell* Forth::getStackBottom() const{
    return this->stackBottom;
}

// For test purposes
// TODO: remove on macro

cell* Forth::getStackPointer() const{
    return this->stackPointer;
}

cell* Forth::getMemory() const{
    return this->memory;
}

cell* Forth::getFreeMemory() const{
    return this->freeMemory;
}

Word* Forth::getLatest() const{
    return this->latest;
}

// Return stack management

void Forth::pushReturn(cell value){
	if(this->returnStackPointer == this->returnStackBottom + this->returnStackSize)
		throw ForthOutOfMemoryException();
	*(this->returnStackPointer) = value;
	this->returnStackPointer++;
}

cell Forth::popReturn(){
	if(this->returnStackPointer == this->returnStackBottom)
		throw ForthEmptyStackException();
	this->returnStackPointer--;
	return *(this->returnStackPointer);
}

// End of Forth implementation

// Word class

Word::Word(Word *_next, bool _compiled, bool _hidden, bool _immediate):
    next(_next), compiled(_compiled), hidden(_hidden), immediate(_immediate), length(0){}

//Word::Word(const char *_name, uint8_t _length, Word *_next):
//	length(_length), next(_next) {
//	memcpy(this->name, _name, _length);
//}

Word* Word::getNextWord() const {
	return this->next;
}

void Word::setNextWord(Word *newWord){
	this->next = newWord;
}

uint8_t Word::getNameLength() const{
	return this->length;
}

const char* Word::getName() const {
	return (const char *)((uint8_t*)this + sizeof(Word));
}

void Word::setName(const char *newName, uint8_t newLength){
	if(this->length != 0)
		throw WordPropertyException();
	char *name = (char*)((uint8_t*)this + sizeof(Word));
	memcpy(name, newName, newLength + 1);
	this->length = newLength;
}

const void* Word::getCode() const {
	uintptr_t size = align(sizeof(Word) + 1 + this->length, sizeof(cell));
	return (const void*)((uint8_t*)this + size);
}

const Word* Word::find(const char *name, uint8_t length) const {
	const Word *word = this;
	while(word){
		if(!word->hidden && length == word->length && !strncmp(word->getName(), name, length)){
			return word;
		}
		word = (const Word*)word->next;
	}
	return NULL;
}

// End of Word implementation

// Miscellaneous functions

void printCell(cell c) {
    printf("%" PRIdPTR " ", c);
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

static intptr_t strtoiptr(const char* ptr, char** endptr, int base){
    if (sizeof(intptr_t) <= sizeof(long)) {
        return (intptr_t)strtol(ptr, endptr, base);
    } else {
        return (intptr_t)strtoll(ptr, endptr, base);
    }
}

static uintptr_t align(uintptr_t value, uint8_t alignment){
    return ((value - 1) | (alignment - 1)) + 1;
}

