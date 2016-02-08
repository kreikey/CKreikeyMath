/*
 ============================================================================
 Name        : RKreikebaumCalculator.c
 Author      : Rick Kreikebaum
 Version     : 0.5 (about halfway done, with add and subtract implemented)
 Copyright   : Copyright 2012
 Description : A calculator using an arbitrary precision math library
 ============================================================================
 */


#include "calculator.h"
#include "kreikeys_io.h"

int main(void) {
	char expression[BUFSIZ+1];
	bool valid = true;

	Rkfloat result;

	setvbuf(stdout, NULL, _IONBF, 0);
	puts("Ready");

	do {
		fgets(expression, BUFSIZ, stdin);
		trimString(expression);
		// printf("%s\t%s\n", leftFloat, righFloat);
		if (strcmp(expression, "exit") == 0 || strcmp(expression, "quit") == 0 || strcmp(expression, "q") == 0)
			continue;
		valid = evaluateExpression(expression);
		if(!valid) {
			printf("Invalid expression. Try again.\n");
			continue;
		}
		// parseAndPrint(expression);
		puts("---------------------------");
	} while (strcmp(expression, "exit") != 0 && strcmp(expression, "quit") != 0 && strcmp(expression, "q") != 0);

	puts("Complete");
	return EXIT_SUCCESS;
}