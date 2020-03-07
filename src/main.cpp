#include "forth.h"
#include "words.h"

#include <stdio.h>

#define MAX_DATA 16384
#define MAX_STACK 16384
#define MAX_RETURN 16384

int main(void){
    Forth forth(stdin, MAX_DATA, MAX_STACK, MAX_RETURN);
    forth.addMachineWords();
	try{
		forth.run();
	} catch (ForthException e) {
		printf("Error: %s", e.getCause());
		return 1;
	}
    return 0;
}
