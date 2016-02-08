/*
 * kreikey_math.h
 *
 *  Created on: Dec 15, 2012
 *      Author: rskreikebaum
 */

//todo: boolean sign, mantissa instead of significand, length, capacity
// meh

#ifndef KREIKEY_MATH_H_
#define KREIKEY_MATH_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

enum Signedness {POSITIVE, NEGATIVE};

typedef struct rkfloat* Rkfloat;

struct rkfloat {
	char* mantissa;
	int exponent;
	enum Signedness sign;
	int size;
	int padding;
	int capacity;
	void (*delete) (Rkfloat);
};

/*struct rkfloat {
	struct rkfloat_type *myFloat;
	int exponent;
	enum Signedness sign;
	int size;
	int padding;
	int capacity;
};
struct rkfloat_type {
	struct rkfloat* (*new)();
	void (*delete)(*rkfloat);
}RKF = {
	.new = rkfloat_new,
	.delete = rkfloat_delete
};*/


// Public
Rkfloat newRkFloat(int length);
Rkfloat newRkFloatFromArgs(char* significand, int mantLen, int exponent, enum Signedness sign);
Rkfloat newRkFloatFromString(char* input);
Rkfloat copyRkFloat(Rkfloat right);
void printRkFloat(Rkfloat number);
void printRkFloatRaw(Rkfloat number);

int compareRkFloats(Rkfloat left, Rkfloat right);
Rkfloat rkAdd(Rkfloat left, Rkfloat right);
Rkfloat rkSubtract(Rkfloat left, Rkfloat right);
Rkfloat rkMultiply(Rkfloat left, Rkfloat right);
Rkfloat rkDivide(Rkfloat left, Rkfloat right, int precision);
Rkfloat rkSqrt(Rkfloat number, int precision);
Rkfloat rkISqrt(Rkfloat number, int precision);
Rkfloat rkPi(int precision);
Rkfloat rkPi2(int precision);
Rkfloat rkReciprocal(Rkfloat num, int precision);

char* rkMantissaToString(Rkfloat num);
char* rkFloatToString(Rkfloat number);

#endif /* KREIKEY_MATH_H_ */
