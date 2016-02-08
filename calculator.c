/*
 * calculator.c
 *
 *  Created on: Dec 17, 2012
 *      Author: rskreikebaum
 */

#include "calculator.h"

/*void parseAndPrint(char* number) {
	Rkfloat num = newRkFloatFromString(number);
	printRkFloatRaw(num);
	printRkFloat(num);
	num->delete(num);
}
*/

bool evaluateExpression(char* expression) {
	Rkfloat leftFloat;
	Rkfloat rightFloat;
	Rkfloat result;
	char* leftStr;
	char* rightStr;
	char* resStr;
	char* operator;
	bool isValid = true;

	leftStr = strtok(expression, " ");
	if (leftStr == NULL)
		return false;
	operator = strtok(NULL, " ");
	if (operator == NULL)
		return false;
	rightStr = strtok(NULL, " ");
	if (rightStr == NULL)
		return false;

	leftFloat = newRkFloatFromString(leftStr);
	rightFloat = newRkFloatFromString(rightStr);

	if (leftFloat == NULL || rightFloat == NULL)
		return false;

	switch (operator[0]) {
		case '+':	result = rkAdd(leftFloat, rightFloat);
					break;
		case '-':	result = rkSubtract(leftFloat, rightFloat);
					break;
		case 'c':	printf("%d\n", compareRkFloats(leftFloat, rightFloat));
					result = rkAdd(leftFloat, rightFloat);
					break;
		case '*':	result = rkMultiply(leftFloat, rightFloat);
					break;
		case '/':	result = rkDivide(leftFloat, rightFloat, 30);
					break;
		case 'r':	result = rkReciprocal(leftFloat, 30);
					break;
		case 's':	result = rkSqrt(leftFloat, 30);
					break;
		case 'p':	result = rkPi2(100);
					break;
		case 'i':	result = rkISqrt(leftFloat, 30);
					break;
		default:	leftFloat->delete(leftFloat);
					rightFloat->delete(rightFloat);
					return false;
	}

	// printRkFloatRaw(leftFloat);
	// printRkFloatRaw(rightFloat);
	// printRkFloatRaw(result);
	printf("= ");
	printRkFloat(result);

	leftFloat->delete(leftFloat);
	rightFloat->delete(rightFloat);
	result->delete(result);

	return true;
}
