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
	
	if(!(this->memory) || !(this->stackBottom) || !(this->returnStackBottom))
		throw ForthException("Forth constructor: failed to allocate memory");
}

Forth::~Forth(){
	delete [] this->stackBottom;
	delete [] this->memory;
	delete [] this->returnStackBottom;
}

void Forth::addMachineWords(){
	int status = 0;
	static const char *square[] = { "dup", "*", "exit", NULL};
	this->addCodeword("interpret", interpreter_stub);
	this->stopWord = this->latest;
	this->executing = (Word *const*)&this->stopWord;
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

	this->addCodeword("exit", forth_exit);
	this->addCodeword("lit", literal);
	this->addCodeword(":", compile_start);
	this->addCodeword(";", compile_end);
	this->latest->setImmediate(true);
	this->addCodeword("'", literal);

	this->addCodeword(">r", rpush);
	this->addCodeword("r>", rpop);
	this->addCodeword("i", rtop);
	this->addCodeword("rshow", rtop);
	this->addCodeword("@", memory_read);
	this->addCodeword("!", memory_write);
	this->addCodeword("here", here);
	this->addCodeword("branch", branch);
	this->addCodeword("0branch", branch0);
	this->addCodeword("immediate", immediate);
	this->latest->setImmediate(true);

	this->addCodeword("word", next_word);
	this->addCodeword(">cfa", _word_code);
	this->addCodeword("find", find);
	this->addCodeword(",", comma);
	this->addCodeword("next", next);
	
	status = this->addCompiledWord("square", square);
	if(status)
		throw ForthIllegalStateException("addMachineWords: failed to add square");
}

// Data stack management

void Forth::push(cell value){
	// Ensure we have room for new data
	if(this->stackPointer == this->stackBottom + this->dataSize)
		throw ForthOutOfMemoryException("push: data stack full");
	*(this->stackPointer) = value;
	this->stackPointer += 1;
}

cell Forth::pop(){
	if (this->stackPointer == this->stackBottom){
		throw ForthEmptyStackException("pop: data stack empty");
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
		throw ForthIllegalArgumentException("addCodeword: too long name");
	if((uint8_t*)this->freeMemory + align(sizeof(Word) + 1 + strlen(name), sizeof(cell)) >
		(uint8_t*)(this->memory + this->memorySize)){
		throw ForthOutOfMemoryException("addCodeword: dictionary is full");
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
	Word *newWord = this->addWord(name, strlen(name), true);
	newWord->setHidden(true);
	while(*words) {
		const Word *word = this->latest->find(*words, strlen(*words));
		if(!word) {
			return 1;
		}
		this->emit((cell)word);
		words += 1;
	}
	newWord->setHidden(false);
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
            throw ForthIllegalStateException("runNumber: literal word missing");
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
            const function code = *(const function*)word->getConstCode();
			code(*this);
        } else{
            this->pushReturn((cell)this->executing);
            this->executing = (Word *const*)word->getConstCode();
        }
		word = *this->executing;
    } while(word != this->stopWord);
}

cell* Forth::getStackBottom() const{
    return this->stackBottom;
}

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

cell* Forth::getReturnStackPointer() const{
	return this->returnStackPointer;
}

cell* Forth::getReturnStackBottom() const{
	return this->returnStackBottom;
}

Word*const* Forth::getInstructionPointer() const{
	return this->executing;
}

void Forth::setInstructionPointer(Word** newInstructionPointer){
	this->executing = newInstructionPointer;
}

void Forth::rewindInstructionPointer(size_t offset){
	this->executing += offset;
}

FILE* Forth::getInput(){
	return this->input;
}

void Forth::setInput(FILE *newInput){
	this->input = newInput;
}

void Forth::setCompiling(bool _compiling){
	this->compiling = _compiling;
}

// Return stack management

void Forth::pushReturn(cell value){
	if(this->returnStackPointer == this->returnStackBottom + this->returnStackSize)
		throw ForthOutOfMemoryException("pushReturn: return stack full");
	*(this->returnStackPointer) = value;
	this->returnStackPointer++;
}

cell Forth::popReturn(){
	if(this->returnStackPointer == this->returnStackBottom)
		throw ForthEmptyStackException("popReturn: return stack empty");
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
	return (const char *)((const uint8_t*)this + sizeof(Word));
}

void Word::setName(const char *newName, uint8_t newLength){
	if(this->length != 0)
		throw WordPropertyException("setName: attempt to overwrite name");
	char *name = (char*)((uint8_t*)this + sizeof(Word));
	memcpy(name, newName, newLength + 1);
	this->length = newLength;
}

void* Word::getCode() {
	uintptr_t size = align(sizeof(Word) + 1 + this->length, sizeof(cell));
	return (void*)((uint8_t*)this + size);
}

const void* Word::getConstCode() const {
	uintptr_t size = align(sizeof(Word) + 1 + this->length, sizeof(cell));
	return (const void*)((const uint8_t*)this + size);
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

bool Word::isImmediate() const{
	return this->immediate;
}

bool Word::isCompiled() const{
	return this->compiled;
}

void Word::setCompiled(bool _compiled){
	this->compiled = _compiled;
}

void Word::setImmediate(bool _immediate){
	this->immediate = _immediate;
}

void Word::setHidden(bool _hidden){
	this->hidden = _hidden;
}

bool Word::isHidden() const{
	return this->hidden;
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

