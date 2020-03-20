#include "forth.h"
#include "words.h"

#include <cstdio>
#include <cstring>

#define MAX_DATA 16384
#define MAX_STACK 16384
#define MAX_RETURN 16384

int main(int argc, char **argv){
	FILE *in;
    Forth forth(stdin, MAX_DATA, MAX_STACK, MAX_RETURN);
    forth.addMachineWords();
	for(int i = 1; i < argc; i++){
		if(!strncmp(argv[i], "-", 1))
			break;
		in = fopen(argv[i], "r");
		if(!in){
			printf("Unable to open file %s! Returning to interpreter mode!\n", argv[i]);
			break;
		}
		forth.setInput(in);
		try{
			forth.run();
		} catch (ForthException e) {
			printf("Error: %s", e.getCause());
			return 1;
		}
	}
	forth.setInput(stdin);
	try{
		forth.run();
	}
	catch(ForthException e){
		printf("Error: %s", e.getCause());
		return 1;
	}
	
    return 0;
}
