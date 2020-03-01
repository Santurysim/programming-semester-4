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
        throw ForthIllegalStateException();
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
