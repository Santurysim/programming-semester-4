#pragma once

#include "forth.h"

void addWords(Forth &forth);

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
