#include <stdio.h>

/* input buffer */
static char input[2048];

int main(int argc, char** argv){
	puts("RyLisp Version 0.0.0.0.0.0.1");
	puts("Press Ctrl-C to Exit\n");

	while(1){
		//display prompt
		fputs("RyLisp> ", stdout);

		//read user input up to buffer size
		fgets(input, 2048, stdin);

		//echo it back out
		printf("You said %s", input);
	}

	return 0;
}