#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "words.h"

void drop(Forth &forth) {
    forth.pop();
}

void _dup(Forth &forth) {
    forth.push(*forth.top());
}

void add(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a + b);
}

void sub(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a - b);
}

void mul(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a * b);
}

void _div(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a / b);
}

void mod(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a % b);
}

void swap(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(b);
    forth.push(a);
}

void rot_back(Forth &forth) {
    cell a, b, c;
    c = forth.pop();
    b = forth.pop();
    a = forth.pop();
    forth.push(c);
    forth.push(a);
    forth.push(b);
}

void rot(Forth &forth) {
    cell a, b, c;
    c = forth.pop();
    b = forth.pop();
    a = forth.pop();
    forth.push(b);
    forth.push(c);
    forth.push(a);
}

void show(Forth &forth) {
    const cell *c = forth.getStackBottom();
    while (c <= forth.top()) {
        printCell(*c);
        c += 1;
    }
    printf("(top)\n");
}

void over(Forth &forth) {
    if(forth.top() - 1 < forth.getStackBottom())
        throw ForthIllegalStateException("over: not enough values in data stack");
    forth.push(*(forth.top()-1));
}

void _true(Forth &forth) {
    forth.push(-1);
}

void _false(Forth &forth) {
    forth.push(0);
}

void _xor(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a ^ b);
}

void _or(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a | b);
}

void _and(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a & b);
}

void _not(Forth &forth) {
    cell c = forth.pop();
    forth.push(~c);
}

void _eq(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a == b ? -1 : 0);
}

void lt(Forth &forth) {
    cell a, b;
    b = forth.pop();
    a = forth.pop();
    forth.push(a < b ? -1 : 0);
}

void within(Forth &forth) {
    cell a, l, r;
    r = forth.pop();
    l = forth.pop();
    a = forth.pop();
    forth.push(l <= a && a < r ? -1 : 0);
}

void forth_exit(Forth &forth){
	forth.setInstructionPointer((Word**)forth.popReturn());
}

void literal(Forth &forth){
	cell value = *(cell*)forth.getInstructionPointer();
	forth.rewindInstructionPointer(1);
	forth.push(value);
}

void compile_start(Forth &forth){
	char buffer[MAX_WORD+1];
	Word *word;
	size_t length = 0;
	readWord(forth.getInput(), buffer, MAX_WORD, &length);
	if(length == 0)
		throw ForthIllegalStateException("compile_start: failed to read word");
	word = forth.addWord(buffer, (uint8_t)length, true);
	forth.setCompiling(true);
	word->setHidden(true);
}

void compile_end(Forth &forth){
	const Word *exit = forth.getLatest()->find("exit", strlen("exit"));
	if(!exit)
		throw ForthIllegalStateException("compile_end: exit word not found");
	forth.emit((cell)exit);
	forth.setCompiling(false);
	forth.getLatest()->setHidden(false);
}

void rpush(Forth &forth){
	forth.pushReturn(forth.pop());
}

void rpop(Forth &forth){
	forth.push(forth.popReturn());
}

void rtop(Forth &forth){
	if(forth.getReturnStackPointer() <= forth.getReturnStackBottom() + 1)
		throw ForthIllegalStateException("rtop: not enough values in return stack");
	forth.push(forth.getReturnStackPointer()[-2]);
}

void rshow(Forth &forth){
	const cell *c = forth.getReturnStackBottom();
	while(c < forth.getReturnStackPointer()){
		printCell(*c);
		c += 1;
	}
	printf("(r-top)\n");
}

void memory_read(Forth &forth){
	forth.push(*(cell*)forth.pop());
}

void memory_write(Forth &forth){
	cell *address = (cell*)forth.pop();
	cell value = forth.pop();
	*(cell*)address = value;
}

void here(Forth &forth){
	forth.push((cell)&forth.freeMemory);
}

void branch(Forth &forth){
	forth.rewindInstructionPointer((size_t)forth.getInstructionPointer()[0] / sizeof(cell));
}

void branch0(Forth &forth){
	cell offset = *(cell*)forth.getInstructionPointer();
	cell value = forth.pop();
	if(!value)
		forth.rewindInstructionPointer(offset / sizeof(cell));
	else
		forth.rewindInstructionPointer(1);
}

void immediate(Forth &forth){
	forth.getLatest()->setImmediate(!forth.getLatest()->isImmediate());
}

void next_word(Forth &forth){
	size_t length;
	static char buffer[MAX_WORD + 1];
	readWord(forth.getInput(), buffer, MAX_WORD + 1, &length);
	forth.push((cell)buffer);
	forth.push((cell)length);
}

void find(Forth &forth){
	uint8_t length = (uint8_t)forth.pop();
	const char *name = (const char*)forth.pop();
	const Word *word = forth.getLatest()->find(name, length);
	forth.push((cell)word);
}

void _word_code(Forth &forth){
	const Word *word = (const Word*)forth.pop();
	const void *code = word->getCode();
	forth.push((cell)code);
}

void comma(Forth &forth){
	forth.emit(forth.pop());
}

void next(Forth &forth){
	forth.rewindInstructionPointer(1);
}

void interpreter_stub(Forth&){
	printf("ERROR: return stack underflow (must return to interpreter)\n");
	exit(2);
}
