#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_WORD 32

class Forth;
typedef intptr_t cell;

typedef void (*function)(Forth&);

enum ForthResult {
    FORTH_OK,
    FORTH_EOF,
    FORTH_WORD_NOT_FOUND,
    FORTH_BUFFER_OVERFLOW
};

class ForthException{
	protected:
		const char *cause;
	public:
		ForthException(): cause("") {}
		ForthException(const char *_cause): cause(_cause) {}
		const char *getCause() const { return cause; }
};

class WordPropertyException: public ForthException {
	public:
		WordPropertyException(): ForthException("") {}
		WordPropertyException(const char *_cause): ForthException(_cause) {}
};

class ForthIllegalArgumentException: public ForthException {
	public:
		ForthIllegalArgumentException(): ForthException("") {}
		ForthIllegalArgumentException(const char *_cause): ForthException(_cause) {}
};

class ForthOutOfMemoryException: public ForthException {
	public:
		ForthOutOfMemoryException(): ForthException("") {}
		ForthOutOfMemoryException(const char *_cause): ForthException(_cause) {}
};

class ForthIllegalStateException: public ForthException {
	public:
		ForthIllegalStateException(): ForthException("") {}
		ForthIllegalStateException(const char *_cause): ForthException(_cause) {}
};

class ForthEmptyStackException: public ForthException {
	public:
		ForthEmptyStackException(): ForthException("") {}
		ForthEmptyStackException(const char *_cause): ForthException(_cause) {}
};

class Word{
	private:
		Word *next;
		bool compiled;
   		bool hidden;
		bool immediate;
		uint8_t length;

	public:
		Word(Word *_next=NULL, bool _compiled=false, bool _hidden=false, bool _immediate=false);

		Word* getNextWord() const;
		void setNextWord(Word *newWord);

		uint8_t getNameLength() const;

		const char* getName() const;
		void setName(const char *newName, uint8_t newLength);

		void setCompiled(bool _compiled);
		bool isCompiled() const;

		void setHidden(bool _hidden);
		bool isHidden() const;

		void setImmediate(bool _immediate);
		bool isImmediate() const;

		void* getCode();
		const void* getConstCode() const;
		const Word* find(const char *name, uint8_t length) const;
};

class Forth{
	private:
		friend void here(Forth& forth);
	    Word *const* executing;
    	cell *returnStackPointer;
		cell *stackPointer;
		cell *memory;

		cell *freeMemory;
		cell *stackBottom;
		cell *returnStackBottom;

		bool compiling;

		Word *latest;
		Word *stopWord;
    
		FILE* input;

		size_t memorySize;
		size_t dataSize;
		size_t returnStackSize;
	public:
		Forth(FILE *_input, size_t _memorySize, size_t _stackSize, size_t _returnStackSize);
		~Forth();
		void addMachineWords();

		void push(cell value);
		cell pop();
		cell* top();

		void emit(cell value);
		void addCodeword(const char *name, const function handler);
		Word* addWord(const char *name, uint8_t length, bool isCompiled);
		void addBaseWords();
		int addCompiledWord(const char*, const char**);

		ForthResult run();

		Word* getLatest() const;

		cell* getStackBottom() const;
		cell* getStackPointer() const;

		cell* getMemory() const;
		cell* getFreeMemory() const;

		cell *getReturnStackPointer() const;
		cell *getReturnStackBottom() const;

		Word*const* getInstructionPointer() const;
		void setInstructionPointer(Word**);
		void rewindInstructionPointer(size_t);

		void pushReturn(cell);
		cell popReturn();

		void setInput(FILE*); // TODO
		FILE* getInput();

		void setCompiling(bool _compiling);
		void runWord(const Word*);
		void runNumber(const char *wordBuffer, size_t length);
};

void printCell(cell c);

ForthResult readWord(FILE* source,
    char* buffer, size_t bufferSize, size_t* length);
