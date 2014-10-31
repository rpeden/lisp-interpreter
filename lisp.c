#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

int main(int argc, char** argv){
	puts("RyLisp Version 0.0.0.0.0.0.1");
	puts("Press Ctrl-C to Exit\n");

	while(1){
		//display prompt and read input
		char* input = readline("RyLisp> ");

		//add to history
		add_history(input);

		//echo it back out
		printf("You said %s\n", input);

		//free input
		free(input);
	}

	return 0;
}