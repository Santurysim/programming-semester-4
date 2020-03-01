#pragma once

#include "forth.h"

void drop(Forth &forth);
void _dup(Forth &forth);
void add(Forth &forth);
void sub(Forth &forth);
void mul(Forth &forth);
void _div(Forth &forth);
void mod(Forth &forth);
void swap(Forth &forth);
void rot(Forth &forth);
void rot_back(Forth &forth);
void show(Forth &forth);
void over(Forth &forth);

void _true(Forth &forth);
void _false(Forth &forth);
void _xor(Forth &forth);
void _or(Forth &forth);
void _and(Forth &forth);
void _not(Forth &forth);
void lt(Forth &forth);
void _eq(Forth &forth);
void within(Forth &forth);

/*void forth_exit(Forth &forth);
void literal(Forth &forth);
void compile_start(Forth &forth);
void compile_end(Forth &forth);

void rpush(Forth &forth);
void rpop(Forth &forth);
void rtop(Forth &forth);
void rshow(Forth &forth);

void memory_read(Forth &forth);
void memory_write(Forth &forth);
void here(Forth &forth);
void branch(Forth &forth);
void branch0(Forth &forth);
void immediate(Forth &forth);

void next_word(Forth &forth);
void find(Forth &forth);
void _word_code(Forth &forth);
void comma(Forth &forth);

void next(Forth &forth);
void interpreter_stub(Forth &forth);*/