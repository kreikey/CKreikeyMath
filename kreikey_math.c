/*
 * kreikey_math.c
 *
 *  Created on: Dec 15, 2012
 *      Author: rskreikebaum
 */

#include "kreikey_math.h"

 // Padding makes everything so damn complicated, I've just about had it with it. Maybe I should get 
 // rid of it and replace it with a semantic length of sorts. Or provide a function to get the semantic
 // length. Either way, I find the current state of the code unacceptable.
 // Changed length member to size. Now implementing getLength which returns semantic length of mantissa,
 // definded as size - padding.

 // TODO: Get rid of padding. Introduce a "mantissaStart" pointer that keeps track of allocated memory.
 // Instead of having padding, use pointer arithmetic on "mantissa" to remove all trailing zeros.
 // Use "mantissaStart" to free the entire mantissa. So delete will still do the right thing.
 // rename "this->size" to "this->length".

 // TODO: Introduce a slice function that will take an existing float and return a slice of it with 
 // a certain length that is smaller than the length of the source.
 // Then fix rkSplit() so that it doesn't copy digits unnecessarily.

 // TODO: Introduce rounding to nearest. Round to even on 5. Don't make it more complicated than it 
 // needs to be. 
 
 // TODO: Make a proper namespace and rename all public functions. See StackOverflow "oop - can you write"

 // TODO: Fix KaratsubaMultiply so that it uses the midpoint of the longer number. If we run out
 // of digits for the "high" half of the smaller number, set it to zero. 
 

static Rkfloat rkAbsAdd(Rkfloat left, Rkfloat right);
static Rkfloat rkAbsSubtract(Rkfloat left, Rkfloat right);
static void rkAccumulate(Rkfloat result, Rkfloat addend);
static Rkfloat parseFloatString(char* floatString);
static bool inputIsValid(char* floatString);
static enum Signedness getSign(char* floatString);
static char* getMantissa(char* floatString);
static int getExponent(char* floatString);
static int getAddSubLen(Rkfloat left, Rkfloat right);
static bool isNeg(Rkfloat self);
static void swap(Rkfloat* left, Rkfloat* right);
static void negate(Rkfloat self);
static Rkfloat rkAbs(Rkfloat self);
static int compareAbsRkFloats(Rkfloat left, Rkfloat right);
static int compareMantissae(Rkfloat left, Rkfloat right);
static int mantissasEqualN(Rkfloat left, Rkfloat right, int n);
static void delete(Rkfloat self);
static bool isZero(Rkfloat self);
static void mantissaCopy(Rkfloat dest, Rkfloat source);
static int getPlaceValue(Rkfloat num, int index);
static int getPrecision(Rkfloat num);
static Rkfloat rkMultiplySimple(Rkfloat left, Rkfloat right);
static Rkfloat rkMultiplyKaratsuba(Rkfloat left, Rkfloat right);
static int max(int left, int right);
static int min(int left, int right);
static int getLength(Rkfloat self);
static void rkSplit(Rkfloat number, int index, Rkfloat* low, Rkfloat* high);
static void removeLeadingZeros(Rkfloat number);
static void removeTrailingZeros(Rkfloat number);
static void trimMantissa(Rkfloat number);
static Rkfloat rkDivideFast(Rkfloat left, Rkfloat right, int precision);
Rkfloat rkSqrtFast(Rkfloat number, int precision);
static void roundToDigits(Rkfloat number, int digits);
static void roundUpDigits(Rkfloat number, int digits);
static void trimToPrecision(Rkfloat number, int precision);
// static int getLogicalLength(Rkfloat number);

/***************************************************************************************************
 * Public interface 
 **************************************************************************************************/

Rkfloat newRkFloat(int capacity) {
	// Another idea I had for dealing with trailing zeros is to have another pointer
	// that points to the first nonZero element in mantissa. This eliminates the need
	// for a padding variable. I would still need to keep a pointer to the original
	// mantissa around, perhaps mantissaStart, so that I can free the mantissa on deallocation.

	if (capacity < 0)
		return NULL;

	Rkfloat self = malloc(sizeof(struct rkfloat));
	self->mantissa = malloc((capacity) * sizeof(char));
	self->exponent = 1;
	self->sign = POSITIVE;
	self->size = 0;
	self->padding = 0;
	self->capacity = capacity;
	self->delete = delete;

	return self;
}

Rkfloat newRkFloatFromArgs(char* mantissa, int mantLen, int exponent, enum Signedness sign) {
	int i = 0;

	Rkfloat self = newRkFloat(mantLen);
	self->exponent = exponent;
	self->sign = sign;
	self->size = mantLen;
	self->padding = 0;
	self->capacity = mantLen;
	self->delete = delete;
	self->mantissa = mantissa;

	return self;
}

Rkfloat newRkFloatFromString(char* floatString) {
	Rkfloat result;

	result = parseFloatString(floatString);

	return result;
}

Rkfloat copyRkFloat(Rkfloat right) {	
	Rkfloat self;
	int i, rpad, rlen;

	// self = newRkFloatFromArgs(right->mantissa, right->size, right->exponent, right->sign);
	// self->padding = right->padding;

	rpad = right->padding;
	rlen = getLength(right);

	self = newRkFloat(rlen);
	self->sign = right->sign;
	self->exponent = right->exponent;
	self->size = rlen;

	// for (i = 0; i < self->size; i++) {
	// 	self->mantissa[i] = right->mantissa[rpad + i];
	// }

	mantissaCopy(self, right);

	return self;
}

int compareRkFloats(Rkfloat left, Rkfloat right) {
	int result;

	// We will assume that if a numer is 0, its exponent is also 0
	result = compareAbsRkFloats(left, right);

	if (left->sign == right->sign) {
		if (left->sign == NEGATIVE)
			result = -result;
	}
	else if (!(isZero(left) && isZero(right))) {	// if left mantissa is NOT "0" OR left and right are NOT equal
		if (left->sign == NEGATIVE)						// if not (left mantissa is 0 and left and right are equal)
			result = -1;								// if not (left and right mantissa are both 0)
		else											// if (left mantissa is not zero) or (right mantissa is not zero)
			result = 1;
	}

	return result;
}

Rkfloat rkAdd(Rkfloat left, Rkfloat right) {
	Rkfloat result;

	if (left->sign == right->sign) {
		result = rkAbsAdd(left, right);
		result->sign = left->sign;
	} else {
		if (compareAbsRkFloats(left, right) > 0) {
			result = rkAbsSubtract(left, right);
			result->sign = left->sign;
		} else {
			result = rkAbsSubtract(right, left);
			result->sign = right->sign;
		}
	}
	if (isZero(result))		// avoid -0
		result->sign = POSITIVE;

	return result;
}

Rkfloat rkSubtract(Rkfloat left, Rkfloat right) {
	Rkfloat result;

	if (left->sign != right->sign) {
		result = rkAbsAdd(left, right);
		result->sign = left->sign;
	} else {
		if (compareAbsRkFloats(left, right) > 0) {
			result = rkAbsSubtract(left, right);
			result->sign = left->sign;
		} else {
			result = rkAbsSubtract(right, left);
			result->sign = !left->sign;
		}
	}
	if (isZero(result))		// avoid -0
		result->sign = POSITIVE;

	return result;
}

Rkfloat rkMultiply(Rkfloat left, Rkfloat right) {
	Rkfloat result;
	// result = rkMultiplySimple(left, right);
	result = rkMultiplyKaratsuba(left, right);

	if (left->sign != right->sign)
		result->sign = NEGATIVE;
	
	return result;
}

Rkfloat rkDivide(Rkfloat left, Rkfloat right, int precision) {
	Rkfloat result;
	result = rkDivideFast(left, right, precision);

	result->sign = left->sign ^ right->sign;

	return result;
}

// Need a more efficient method for calculating square root.
// Try iterative inverse square root method. 
// This is the babylonian method. It's actually a lot faster than it was before,
// for some reason
Rkfloat rkSqrt(Rkfloat number, int precision) {
	Rkfloat x0, x1, half, quot, sum;
	int n;

	precision += 2;

	half = newRkFloat(1);
	half->mantissa[0] = 5;
	half->exponent = -1;
	half->size = 1;

	x0 = newRkFloat(1);
	if (number->exponent % 2 == 0) {
		n = number->exponent / 2;
		x0->mantissa[0] = 2;
	} else {
		n = (number->exponent - 1) / 2;
		x0->mantissa[0] = 6;
	}
	x0->exponent = n;
	x0->size = 1;

	// printf("x0:\n");
	// printRkFloat(x0);

	while (true) {
		// printf("x0:\n");
		// printRkFloat(x0);
		quot = rkDivideFast(number, x0, precision);
		sum = rkAbsAdd(x0, quot);
		quot->delete(quot);
		x1 = rkMultiplySimple(half, sum);
		sum->delete(sum);
		removeTrailingZeros(x1);
		// if (precision < getLength(x1))
			// x1->padding = x1->size - precision;
		roundToDigits(x1, precision);
		// This is vulnerable to infinite loops if we use full precision.
		if (mantissasEqualN(x0, x1, precision - 1))
			break;
		x0->delete(x0);
		x0 = copyRkFloat(x1);
		x1->delete(x1);
	}

	x0->delete(x0);
	half->delete(half);

	precision -= 2;
	// x1->padding = x1->size - precision;
	// printf("precision: %d\n", precision);
	// printf("before round:\n");
	// printRkFloat(x1);
	roundToDigits(x1, precision);

	return x1;
}

// Xn+1 = Xn/2 * (3 - S * Xn^2)
Rkfloat rkISqrt(Rkfloat number, int precision) {
	Rkfloat x0, x1, three, half, square, prod, diff, res;
	int n;

	precision += 2;

	three = newRkFloat(1);
	three->mantissa[0] = 3;
	three->exponent = 0;
	three->size = 1;

	half = newRkFloat(1);
	half->mantissa[0] = 5;
	half->exponent = -1;
	half->size = 1;

	x0 = newRkFloat(2);
	if (number->exponent % 2 == 0) {
		n = number->exponent / 2;
		x0->mantissa[0] = 5;
		x0->size = 1;
	} else {
		n = (number->exponent - 1) / 2;
		x0->mantissa[0] = 1;
		x0->mantissa[1] = 7;
		x0->size = 2;
	}
	x0->exponent = -1 - n;

	while(true) {
		// printf("x0:\n");
		// printRkFloat(x0);
		square = rkMultiplyKaratsuba(x0, x0);
		prod = rkMultiplyKaratsuba(number, square);
		square->delete(square);
		diff = rkAbsSubtract(three, prod);
		prod->delete(prod);
		prod = rkMultiplyKaratsuba(x0, diff);
		diff->delete(diff);
		x1 = rkMultiplyKaratsuba(half, prod);
		removeTrailingZeros(x1);
		// if (precision < getLength(x1))
			// x1->padding = x1->size - precision;
		roundToDigits(x1, precision);
		// printf("x1:\n");
		// printRkFloat(x1);
		if (mantissasEqualN(x0, x1, precision - 1))
			break;
		x0->delete(x0);
		x0 = copyRkFloat(x1);
		x1->delete(x1);
	}

	x0->delete(x0);
	three->delete(three);
	half->delete(half);

	precision -= 2;

	// x1->padding = x1->size - precision;
	roundToDigits(x1, precision);

	return x1;
}

Rkfloat rkPi(int precision) {
	Rkfloat res, term, root, piece, sum, sum2, recip, half, four, two;
	int i = 0;

	precision += 2;

	half = newRkFloat(1);
	half->mantissa[0] = 5;
	half->exponent = -1;
	half->size = 1;

	four = newRkFloat(1);
	four->mantissa[0] = 4;
	four->exponent = 0;
	four->size = 1;

	two = newRkFloat(1);
	two->mantissa[0] = 2;
	two->exponent = 0;
	two->size = 1;	

	piece = copyRkFloat(two);
	root = rkSqrt(piece, precision);
	piece->delete(piece);
	recip = rkReciprocal(root, precision);
	sum = rkAbsAdd(recip, half);
	recip->delete(recip);
	term = copyRkFloat(sum);
	sum->delete(sum);

	while (true) {
		// printf("root:\n");
		// printRkFloat(root);
		// printf("two:\n");
		// printRkFloat(two);
		piece = rkAbsAdd(root, two);
		// printf("piece:\n");
		// printRkFloat(piece);
		root->delete(root);
		root = rkSqrt(piece, precision);
		// printf("got root\n");
		piece->delete(piece);
		recip = rkReciprocal(root, precision);
		// printf("got reciprocal\n");
		sum = rkAbsAdd(recip, half);
		recip->delete(recip);
		res = rkMultiplyKaratsuba(sum, term);
		// printf("got product\n");
		sum->delete(sum);
		// if (precision < getLength(res))
			// res->padding = res->size - precision;
		roundToDigits(res, precision);
		if (mantissasEqualN(res, term, precision - 1))
			break;

		term->delete(term);
		term = copyRkFloat(res);
		res->delete(res);
		printf("term:\n");
		printRkFloat(term);
	}

	printf("done with loop\n");
	// res->padding = res->size - precision;
	term->delete(term);
	term = copyRkFloat(res);
	res->delete(res);

	res = rkDivideFast(four, term, precision);
	precision -= 2;

	roundToDigits(res, precision);
	res->padding = res->size - precision;

	term->delete(term);
	root->delete(root);
	half->delete(half);
	four->delete(four);
	two->delete(two);

	return res;
}

Rkfloat rkPi2(int precision) {
	Rkfloat res, term, root, piece, sum, sum2, recip, half, four, two, prod;
	int i = 0, n = 0;
	// char str[8192];

	printf("working...\n");

	precision += 2;

	half = newRkFloat(1);
	half->mantissa[0] = 5;
	half->exponent = -1;
	half->size = 1;

	four = newRkFloat(1);
	four->mantissa[0] = 4;
	four->exponent = 0;
	four->size = 1;

	two = newRkFloat(1);
	two->mantissa[0] = 2;
	two->exponent = 0;
	two->size = 1;	

	piece = copyRkFloat(two);
	recip = rkISqrt(piece, precision);
	root = rkMultiplyKaratsuba(recip, piece);
		piece->delete(piece);
	prod = rkAbsAdd(recip, half);
		recip->delete(recip);
	piece = rkAbsAdd(root, two);

	while (true) {
		n++;
		printf("iteration %d\n", n);
		recip = rkISqrt(piece, precision);
		root = rkMultiplyKaratsuba(recip, piece);
			piece->delete(piece);
		sum = rkAbsAdd(recip, half);
			recip->delete(recip);
		res = rkMultiplyKaratsuba(sum, prod);
			sum->delete(sum);
		piece = rkAbsAdd(root, two);
			root->delete(root);
		// if (precision < getLength(res))
			// res->padding = res->size - precision;
		roundToDigits(res, precision);
		if (mantissasEqualN(res, prod, precision - 1))
			break;

		prod->delete(prod);
		prod = copyRkFloat(res);
		printf("prod:\n");
		printRkFloat(prod);
		// getchar();
		// gets(str);
	}
		
	// printf("res:\n");
	// printRkFloat(res);

	prod->delete(prod);
	prod = copyRkFloat(res);
	res->delete(res);

	res = rkDivideFast(four, prod, precision);
	precision -= 2;

	roundToDigits(res, precision);
	res->padding = res->size - precision;
	// printRkFloatRaw(res);

	prod->delete(prod);
	half->delete(half);
	four->delete(four);
	two->delete(two);

	return res;
}

char* rkMantissaToString(Rkfloat num) {
	char* mantString;
	int i;

	mantString = malloc((getLength(num) + 1) * sizeof(char));

	for (i = num->size - 1; i >= num->padding; i--) {
		mantString[num->size - 1 - i] = (num->mantissa[i] + '0');
	}

	mantString[getLength(num)] = '\0';
	return mantString;
}

char* rkFloatToString(Rkfloat self) {
	char* floatString;
	int leadingZeros;		// how many leading zeros
	int trailingZeros;
	int stringLen;				// the total length of the string to write
	int i = 0;
	int decNdx;				// the index of the decimal point
	int lzNdx = 0;		// The number of zeros written so far
	int sigNdx;			// The mantissa index
	int sigLen;				// the length of the mantissa
	int otherChars = 0;

	// Get length of the mantissa
	sigLen = getLength(self);

	// Find the index of the decimal 
	decNdx = (self->exponent < 1) ? 1 : (self->exponent + 1);

	// Figure out how many leading zeros we need for a negative exponent.
	leadingZeros = (self->exponent < 0) ? (-(self->exponent)) : 0;

	// Deal with trailing zeros
	trailingZeros = (decNdx > sigLen) ? (decNdx - sigLen) : 0;

	if ((self->exponent + 1) < sigLen)		// this is more general than using decNdx
		otherChars++;

	if (self->sign == NEGATIVE) {
		otherChars++;
		decNdx++;
	}

	// Get the length of our new floating-point string
	stringLen = sigLen + leadingZeros + trailingZeros + otherChars;	

	floatString = malloc((stringLen + 1) * sizeof(char));

	sigNdx = sigLen + self->padding - 1;

	if (self->sign == NEGATIVE)
		floatString[i++] = '-';
	while (i < stringLen) {
		if (i == decNdx)
			floatString[i] = '.';
		else if (lzNdx < leadingZeros) {
			floatString[i] = '0';
			lzNdx++;
		}
		else if (sigNdx >= 0)
			floatString[i] = self->mantissa[sigNdx--] + '0';
		else
			floatString[i] = '0';
		i++;
	}

	floatString[i] = '\0';

	return floatString;
}



/***************************************************************************************************
 * Private interface 
 **************************************************************************************************/

static Rkfloat rkAbsAdd(Rkfloat left, Rkfloat right) {
	int i = 0, j = 0, k = 0, lpr, rpr, resLen, moreDig, lessDig, resDig, offset, carry = 0, mpPad;
	// There's a bug in here that introduces junk characters into the mantissa of the result.
	// Sometimes.

	lpr = getLength(left) - left->exponent;
	rpr = getLength(right) - right->exponent;

	Rkfloat morePrecise, lessPrecise, result;
	resLen = getAddSubLen(left, right) + 1;			// one for carry
	result = newRkFloat(resLen);
	result->exponent = left->exponent > right->exponent ? left->exponent : right->exponent;

	if (lpr > rpr) { 
		morePrecise = left;
		lessPrecise = right;
	} else {
		morePrecise = right;
		lessPrecise = left;
	}

	rkAccumulate(result, morePrecise);
	rkAccumulate(result, lessPrecise);

	return result;
}

static Rkfloat rkAbsSubtract(Rkfloat bigger, Rkfloat smaller) {
	int i = 0, j = 0, k = 0, bigDig, smallDig, resDig, offset, bigPrec, smallPrec;
	bool borrow = false, lastBorrow = false;
	Rkfloat result;

	result = newRkFloat(getAddSubLen(bigger, smaller));
	result->exponent = bigger->exponent;
	bigPrec = getLength(bigger) - bigger->exponent;
	smallPrec = getLength(smaller) - smaller->exponent;
	offset = bigPrec - smallPrec;

	if (isZero(smaller))	// Best work-around ever!
		offset = 0;

	if (offset > 0) {
		i = bigger->padding;
		j = smaller->padding - offset;
	}
	else {
		i = bigger->padding + offset;
		j = smaller->padding;
	}

	while (i < bigger->size || j < smaller->size) {
		bigDig = (i >= bigger->padding) && (i < bigger->size) ? bigger->mantissa[i] : 0;
		smallDig = (j >= smaller->padding) && (j < smaller->size) ? smaller->mantissa[j] : 0;
		resDig = bigDig - smallDig - borrow;
		lastBorrow = borrow;
		borrow = resDig < 0 ? true : false;
		resDig += borrow ? 10 : 0;
		result->size++;			
		result->mantissa[k++] = resDig;
		i++;
		j++;
	}
	
	k--;
	while (result->mantissa[k] == 0 && k > result->padding) {
		result->size--;
		result->exponent--;
		k--;
	}

	k = 0;
	while (result->mantissa[k] == 0 && k < result->size - 1) {
		result->padding++;
		k++;
	}
	
	return result;
}

static Rkfloat parseFloatString(char* floatString) {
	int i, firstNdx = 0, lastNdx = 0, length = 0, j = 0, trailingZeros = 0, exponent, decNdx, firstZeroNdx;
	bool foundNonZero = false, isValid = true, foundDecimal = false, foundZero = false;
	char dig;
	char* mantissa;
	enum Signedness sign = POSITIVE;
	Rkfloat result;

	for (i = 0; i < strlen(floatString); i++) {
		dig = floatString[i];
		if (isdigit(dig)) {
			trailingZeros++;
			if (dig != '0') {
				if (!foundNonZero) {
					firstNdx = i;
					foundNonZero = true;
				}				
				trailingZeros = 0;
				lastNdx = i;
			} else if (!foundZero) {
				foundZero = true;
				firstZeroNdx = i;
			}
			if (foundNonZero)
				length++;
		} else if (dig == '.') {
			if (foundDecimal)
				isValid = false;
			else {
				foundDecimal = true;
				decNdx = i;
			}
		} else if (i == 0) {
			if (dig == '+')
				sign = POSITIVE;
			else if (dig == '-')
				sign = NEGATIVE;
			else
				isValid = false;
		} else {
			isValid = false;
		}
	}

	if (!foundDecimal)
		decNdx = lastNdx + trailingZeros + 1;

	if (!foundNonZero) {	// We must handle the case where the number represents zero in some form
		if (foundZero) {
			trailingZeros = -1;
			decNdx = 1;
			lastNdx = firstZeroNdx;
		} else {
			isValid = false;
		}
	}

	exponent = decNdx > firstNdx ? decNdx - firstNdx - 1 : decNdx - firstNdx;
	length -= trailingZeros;

	mantissa = malloc(length * sizeof(char));

	for (i = lastNdx; i >= firstNdx; i--) {
		if (isdigit(floatString[i]))
			mantissa[j++] = floatString[i] - '0';
	}

	if (isValid)
		result = newRkFloatFromArgs(mantissa, length, exponent, sign);
	else
		result = NULL;

	return result;
}

static int getAddSubLen(Rkfloat left, Rkfloat right) {
	// choose greater exponent
	// choose greater numdigs - exponent
	// add the two
	// The exponent represents the number of digits to the left of the decimal minus one.
	// numdigs - exponent represents the number of digits to the right of the decimal plus one.
	// Does this work for negative exponents? Yes.

	int bigExponent;
	int bigRemainingNumdigs;
	int leftRemainingNumDigs;
	int rightRemainingNumDigs;
	int resultNumDigs;

	if (left->exponent > right->exponent)
		bigExponent = left->exponent;
	else
		bigExponent = right->exponent;

	leftRemainingNumDigs = (getLength(left) - left->exponent);
	rightRemainingNumDigs = (getLength(right) - right->exponent);

	if (leftRemainingNumDigs > rightRemainingNumDigs)
		bigRemainingNumdigs = leftRemainingNumDigs;
	else
		bigRemainingNumdigs = rightRemainingNumDigs;

	resultNumDigs = bigExponent + bigRemainingNumdigs;

	return resultNumDigs;
}

static bool isNeg(Rkfloat self) {
	if (self->sign == NEGATIVE)
		return true;
	else
		return false;
}

static void swap(Rkfloat* left, Rkfloat* right) {
	Rkfloat temp;
	temp = *left;
	*left = *right;
	*right = temp;
}

static void negate(Rkfloat self) {
	if (self->sign == POSITIVE)
		self->sign = NEGATIVE;
	else
		self->sign = POSITIVE;
}

static Rkfloat rkAbs(Rkfloat self) {
	self->sign = POSITIVE;

	return self;
}

static int compareAbsRkFloats(Rkfloat left, Rkfloat right) {
	int result;
	bool lz, rz;

	lz = isZero(left);
	rz = isZero(right);

	if (lz || rz) {
		if (lz && rz)
			return 0;
		else if (rz)
			return 1;
		else if (lz)
			return -1;
	} else if (left->exponent > right->exponent)
		result = 1;
	else if (left->exponent < right->exponent)
		result = -1;
	else
		result = compareMantissae(left, right);

	return result;	
}

static int compareMantissae(Rkfloat left, Rkfloat right) {
	int i = left->size - 1, j = right->size - 1;

	while (i >= left->padding && j >= right->padding) {
		if (left->mantissa[i] > right->mantissa[j])
			return 1;
		else if (left->mantissa[i] < right->mantissa[j])
			return -1;
		i--;
		j--;
	}
	if (i >= left->padding)		// We should never have a trailing zero in a mantissa
		return 1;				// Each additional digit sways the result.
	else if (j >= right->padding)
		return -1;

	return 0;
}

static int mantissasEqualN(Rkfloat left, Rkfloat right, int n) {
	int i = left->size - 1, j = right->size - 1, k = 0;
	int result = 1;

	// printf("left size: %d, right size: %d\n", left->size, right->size);
	// printf("comparing digit: ");
	while (k < n && i >= left->padding && j >= right->padding) {
		// printf("%d [%d %d] ", k, left->mantissa[i], right->mantissa[j]);
		if (left->mantissa[i] != right->mantissa[j])
			result = 0;
		i--;
		j--;
		k++;
	}

	if (k < n && (i >= left->padding || j >= right->padding))
		result = 0;

	// printRkFloatRaw(left);
	// printf("=\n");
	// printRkFloatRaw(right);
	// printf("result: %d", result);
	// printf("\n");
	return result;
}

static void delete(Rkfloat self) {
	free(self->mantissa);
	free(self);
}

static bool isZero(Rkfloat self) {
	if (getLength(self) == 1 && self->mantissa[self->padding] == 0)
		return true;
	else
		return false;
}

static void mantissaCopy(Rkfloat dest, Rkfloat source) {
	int i, sLen, sPad;

	sLen = getLength(source);
	sPad = source->padding;

	for (i = 0; i < sLen; i++) {
		dest->mantissa[i] = source->mantissa[i + sPad];
	}
}

void printRkFloat(Rkfloat number) {
	char* floatString;

	floatString = rkFloatToString(number);
	printf("%s\n", floatString);
	free(floatString);
}

void printRkFloatRaw(Rkfloat number) {
	char* mantissa = rkMantissaToString(number);
	printf("sign: %c\n", number->sign ? '-' : '+');
	printf("significand: %s\n", mantissa);
	printf("exponent: %d\n", number->exponent);
	printf("padding: %d\n", number->padding);
	free(mantissa);
}

static void rkAccumulate(Rkfloat result, Rkfloat addend) {
	/*	The current strategy is to set the exponent for the addend properly in upstream code.
		We should probably set the result to zero when we start. Then add the addend to result.
		We can repeatedly add to this result as long as it has enough space allocated for the mantissa,
		and the addend is equally or less precise than the result.
	*/

	int i, j, offset, resDig, carry = 0, addDig, curDig;

	if (result->size == 0) {
		result->size = 1;
		result->mantissa[0] = 0;
	}

	if (isZero(result)) {
		offset = 0;
		result->exponent = addend->exponent;
	} else {
		offset = getPlaceValue(addend, addend->padding) - getPlaceValue(result, result->padding);
		if (addend->exponent > result->exponent)
			result->exponent = addend->exponent;
	}

	// sometimes we start putting in digits past the current length of the result. In that case,
	// fill up the intervening space with zeros.
	for (i = result->size; i < result->padding + offset; i++) {
		result->mantissa[i] = 0;
		result->size++;
	}

	for (i = result->padding + offset, j = addend->padding; j < addend->size; i++, j++) {
		curDig = i < result->size ? result->mantissa[i] : 0;
		addDig = addend->mantissa[j];
		resDig = curDig + addDig + carry;
		carry = resDig / 10;
		resDig %= 10;
		result->mantissa[i] = resDig;

		if (i == result->size) {
			result->size++;
		}
	}

	while (carry) {
		curDig = i < result->size ? result->mantissa[i] : 0;
		resDig = curDig + carry;
		carry = resDig / 10;
		resDig %= 10;
		result->mantissa[i] = resDig;

		if (i == result->size) {
			result->size++;
			result->exponent++;
		}
		i++;
	}

	i = result->padding;
	while (result->mantissa[i] == 0 && i < result->size - 1) {
		result->padding++;
		i++;
	}

}

static Rkfloat rkMultiplySimple(Rkfloat left, Rkfloat right) {
	int i, j, k;
	int sDigit;
	int lDigit;
	int rDigit;
	int cDigit;
	Rkfloat result;
	Rkfloat shorter;
	Rkfloat longer;
	Rkfloat addend;

	result = newRkFloat(left->size + right->size);

	if (left->size > right->size) {
		shorter = right;
		longer = left;
	} else {
		shorter = left;
		longer = right;
	}

	result->mantissa[0] = 0;
	result->size = 1;

	addend = newRkFloat(longer->size + 1);

	for (i = shorter->padding; i < shorter->size; i++) {
		sDigit = shorter->mantissa[i];
		cDigit = 0;
		addend->size = 0;
		addend->padding = 0;
		addend->exponent = longer->exponent;

		for (j = longer->padding, k = 0; j < longer->size; j++, k++) {
			lDigit = longer->mantissa[j];
			rDigit = sDigit * lDigit + cDigit;
			cDigit = rDigit / 10;
			rDigit %= 10;
			addend->mantissa[k] = rDigit;
			addend->size++;
		}

		if (cDigit > 0) {
			addend->mantissa[k++] = cDigit;
			addend->size++;
			addend->exponent++;
		}

		addend->exponent += getPlaceValue(shorter, i);
		rkAccumulate(result, addend);
	}

	addend->delete(addend);
	
	return result;
}

static Rkfloat rkMultiplyKaratsuba(Rkfloat left, Rkfloat right) {
	// There's a bug in here somewhere that causes inconsistent results on 1 / 4. 
	// It doesn't manifest when we use rkMultiplySimple.
	// Todo: Fix the bug that causes corrupt data sometimes on "4 r 0" and failure on "9 r 0"
	// Apparently the bug is somewhere else. I pass bad data in here.
	// Looks like the bug is fixed. It was in RkAbsAdd, and had to do with an inadequte
	// copyMantissa function that at times went outside the bounds of allocated memory.

	Rkfloat result, low1, high1, low2, high2, z0, z1, z2, r1, r2, temp1, temp2;
	int m, expOffset, leftOffset, rightOffset;

	if (getLength(left) < 2 || getLength(right) < 2)
		return rkMultiplySimple(left, right);

	// printf("left\n");
	// printRkFloat(left);
	// printf("right\n");
	// printRkFloat(right);

	leftOffset = (left->exponent + 1 - getLength(left));
	rightOffset = (right->exponent + 1 - getLength(right));
	expOffset = leftOffset + rightOffset;
	left->exponent -= leftOffset;
	right->exponent -= rightOffset;

	// Change this to max and fix split so we can have a zero for the high number.
	m = min(getLength(left), getLength(right)) / 2;

	rkSplit(left, m, &low1, &high1);
	rkSplit(right, m, &low2, &high2);

	// printf("low1:\n");
	// printRkFloat(low1);
	// printf("low2:\n");
	// printRkFloat(low2);
	// printf("high1:\n");
	// printRkFloat(high1);
	// printf("high2:\n");
	// printRkFloat(high2);

	z0 = rkMultiplyKaratsuba(low1, low2);
	// Does our function delete left and right? If not, then we have a memory leak. 
	temp1 = rkAbsAdd(low1, high1);
	temp2 = rkAbsAdd(low2, high2);
	z1 = rkMultiplyKaratsuba(temp1, temp2);
	z2 = rkMultiplyKaratsuba(high1, high2);
	temp1->delete(temp1);
	temp2->delete(temp2);

	// printf("left\n");
	// printRkFloat(left);
	// printf("high1\n");
	// printRkFloat(high1);
	// printf("high2\n");
	// printRkFloat(high2);
	// printf("z2\n");
	// printRkFloat(z2);

	r1 = copyRkFloat(z2);
	r1->exponent += (2 * m);

	temp1 = rkAbsAdd(z0, z2);
	r2 = rkAbsSubtract(z1, temp1);
	r2->exponent += m;
	temp1->delete(temp1);

	// printf("r1\n");
	// printRkFloat(r1);
	// printf("r2\n");
	// printRkFloat(r2);
	// printf("z0\n");
	// printRkFloat(z0);

	temp1 = rkAbsAdd(r1, r2);
	result = rkAbsAdd(temp1, z0);
	temp1->delete(temp1);

	result->exponent += expOffset;
	left->exponent += leftOffset;
	right->exponent += rightOffset;

	// printf("left\n");
	// printRkFloat(left);
	// printf("right\n");
	// printRkFloat(right);
	// printf("result\n");
	// printRkFloat(result);

	z0->delete(z0);
	z1->delete(z1);
	z2->delete(z2);
	r1->delete(r1);
	r2->delete(r2);
	low1->delete(low1);
	low2->delete(low2);
	high1->delete(high1);
	high2->delete(high2);
	// temp1->delete(temp1);
	// temp2->delete(temp2);

	// Are we supposed to delete low1, high1, low2, high2? Only if they're separate objects,
	// not slices of exising objects.

	// temp2->delete(temp2);
	// temp3->delete(temp3);
	// temp4->delete(temp4);

	return result;
}

static void rkSplit(Rkfloat number, int m, Rkfloat* low, Rkfloat* high) {
	// The reciprocal bug might be in here somewhere.
	int lowLen, highLen, i, j;
	
	lowLen = m;
	highLen = getLength(number) - m;

	*low = newRkFloat(lowLen);
	*high = newRkFloat(highLen);

	// To make things more efficient, I should have a way to create slices
	// of existing numbers, rather than copying digits.
	// But then I'd have to make sure I don't delete those slices.

	for (i = 0, j = number->padding; i < lowLen; i++, j++)
		(*low)->mantissa[i] = number->mantissa[j];
	for (i = 0, j = number->padding + lowLen; i < highLen; i++, j++)
		(*high)->mantissa[i] = number->mantissa[j];

	(*low)->size = lowLen;
	(*high)->size = highLen;
	(*low)->exponent = getLength(*low) - 1;
	(*high)->exponent = getLength(*high) - 1;
	trimMantissa(*low);
	trimMantissa(*high);

	// printf("num:\n");
	// printRkFloat(number);
	// printf("low:\n");
	// printRkFloat(*low);
	// printf("high:\n");
	// printRkFloat(*high);
}

Rkfloat rkReciprocal(Rkfloat num, int precision) {
	// Precision is tricky. We could use 1/3 as a canonical example to calculate for testing purposes.
	// The number of digits returned from a calculation is often more than the precision we want.
	// Do we keep all the digits in the intermediate steps, or do we cut them off 1 decimal place
	// after precision? I think the easiest way to handle this is to make a compare function that 
	// stops comparing after a certain number of digits. But that's probably impractical for highly
	// precise reciprocals, because we essentially double the number of digits with each calculation.
	// So we need another parameter or a global variable to limit the precision of all operations.
	// Need to keep wikipedia page advice in mind when dealing with precision.
	// Precision here means number of digits.
	// It would probably be better to use the other formula, you know, the one involving 1
	// So that we don't have too many digits floating around.
	// BUGS: There's a bug when we take the reciprocal of 10.
	Rkfloat result, temp, two, prod, diff;

	precision += 2;

	two = newRkFloat(1);
	two->mantissa[0] = 2;
	two->size = 1;
	two->exponent = 0;

	result = newRkFloat(precision);
	result->mantissa[0] = 2;
	result->size = 1;
	result->exponent = -(num->exponent) - 1;

	temp = copyRkFloat(result);

	while(true) {
		// printf("num:\n");
		// printRkFloat(num);
		prod = rkMultiplyKaratsuba(num, temp);
		// prod = rkMultiplySimple(num, temp);
		diff = rkAbsSubtract(two, prod);
		prod->delete(prod);
		// printf("temp:\n");
		// printRkFloat(temp);
		result = rkMultiplyKaratsuba(temp, diff);
		// result = rkMultiplySimple(temp, diff);
		diff->delete(diff);
		// printf("result\n");
		// printRkFloat(result);
		// printRkFloat(temp);
		removeTrailingZeros(result);
		// if (precision < getLength(result))
			// result->padding = result->size - precision;
		roundToDigits(result, precision);
		if (mantissasEqualN(result, temp, precision - 1))
			break;
		temp->delete(temp);
		// printf("old:\n");
		// printRkFloat(result);
		temp = copyRkFloat(result);
		// printf("new:\n");
		// printRkFloat(temp);
		result->delete(result);
	}

	precision -= 2;

	roundToDigits(result, precision);

	// if (precision < getLength(result)) {
	// 	result->padding = result->size - precision;
	// }

	result->sign = num->sign;
	temp->delete(temp);
	two->delete(two);

	return result;
}

static Rkfloat rkDivideFast(Rkfloat left, Rkfloat right, int precision) {
	Rkfloat result, reciprocal;

	precision += 2;

	reciprocal = rkReciprocal(right, precision);
	result = rkMultiplyKaratsuba(reciprocal, left);
	reciprocal->delete(reciprocal);

	precision -= 2;

	// if (precision < getLength(result)) {
	// 	result->padding = result->size - precision;
	// }

	roundToDigits(result, precision);

	return result;
}

/*Rkfloat rkSqrtFast(Rkfloat number, int precision) {

}
*/
static int getPlaceValue(Rkfloat num, int index) {
	// Place value is the effective exponent on the digit at index.

	return (index - num->size + 1 + num->exponent);
}

static int getPrecision(Rkfloat num) {
	return (num->size - num->padding - num->exponent - 1);
}

static int max(int left, int right) {
	if (left > right)
		return left;
	else
		return right;
}

static int min(int left, int right) {
	if (left < right)
		return left;
	else
		return right;
}

static int getLength(Rkfloat self) {
	return self->size - self->padding;
}

static void removeLeadingZeros(Rkfloat number) {
	int i = number->size - 1;

	while (i > number->padding && number->mantissa[i] == 0) {
		number->size--;
		number->exponent--;
		i--;
	}
}

static void removeTrailingZeros(Rkfloat number) {
	int i = number->padding;

	while (i < number->size && number->mantissa[i] == 0) {
		number->padding++;
		i++;
	}
}

static void trimMantissa(Rkfloat number) {
	removeLeadingZeros(number);
	removeTrailingZeros(number);
}

static void roundToDigits(Rkfloat number, int digits) {
	// This doesn't seem to work correctly. It yields 2 digits to many in some cases. 
	int dropped;

	if (getLength(number) <= digits)
		return;


	dropped = number->size - 1 - digits;

	// printf("in roundDigits\n");
	// printf("dropndx: %d\n", dropped);
	// printf("padding: %d\n", number->padding);
	// printf("size: %d\n", number->size);
	// printf("dropDig: %d\n", number->mantissa[dropped]);

	if (number->mantissa[dropped] > 5) {
		roundUpDigits(number, digits);
	} else if (number->mantissa[dropped] == 5) {
		if (number->mantissa[dropped + 1] % 2 != 0)
			roundUpDigits(number, digits);
		else
			number->padding = dropped + 1;
	} else {
		number->padding = dropped + 1;
	}
}

static void roundUpDigits(Rkfloat number, int digits) {
	int last, dig, carry, res;

	if (getLength(number) <= digits)
		return;

	last = number->size - digits;
	number->padding = last;

	res = number->mantissa[last] + 1;
	carry = res / 10;
	dig = res % 10;
	number->mantissa[last++] = dig;

	// printf("in roundUpDigits\n");
	// printf("dig: %d\n", dig);
	// printf("carry: %d\n", carry);

	while (carry) {
		number->padding++;
		res = number->mantissa[last] + 1;
		carry = res / 10;
		dig = res % 10;
		number->mantissa[last++] = dig;
	}
}