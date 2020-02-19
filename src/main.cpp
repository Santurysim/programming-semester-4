#include "forth.h"
#include "words.h"

#include <stdio.h>

#define MAX_DATA 16384
#define MAX_STACK 16384

int main(void){
    Forth forth(stdin, MAX_DATA, MAX_STACK);
    addWords(forth);
	forth.run();
    return 0;
}
