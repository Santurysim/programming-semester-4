#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_WORD 32

// struct forth;
typedef intptr_t cell;

typedef void (*function)(Forth* forth); // TODO

enum ForthResult {
    FORTH_OK,
    FORTH_EOF,
    FORTH_WORD_NOT_FOUND,
    FORTH_BUFFER_OVERFLOW,
};

class ForthException{}; // TODO

class Word{
	private:
		Word *next;
		uint8_t length;
		char *name;
	public:
		Word();
		~Word();
		Word* getNextWord() const;
		uint8_t getLength() const;
		char* getName() const;
		const void* getCode() const;
		const Word* find(char *name, uint8_t length) const;
		// TODO
};

class Forth{
	private:
		cell *stackPointer;
		cell *memory;

		Word *latest;
    
		FILE* input;

		cell *freeMemory;
		cell *sp0;
		size_t memorySize;
		size_t dataSize;

		void runNumber(const char *wordBuffer, size_t length);
	public:
		Forth(FILE *_input, size_t _memorySize, size_t stackSize);
		~Forth();
		void push(cell value);
		cell pop();
		cell* top();
		void emit(cell value);
		void addCodeword(const char *name, const function handler);
		ForthResult run();

		Word* addWord(const char *name, uint8_t length);

		// Default words from words.h  TODO
};

void printCell(cell c);

ForthResult readWord(FILE* source,
    char* buffer, size_t bufferSize, size_t* length);
