/*
 * kreikeys_io.c
 *
 *  Created on: Dec 17, 2012
 *      Author: rskreikebaum
 */

#include "kreikeys_io.h"

char* trimString(char* string) {
	int ndx = strlen(string) - 1;

	while (isspace(string[ndx]) && (ndx >= 0)) {		// Is it safe to have a null string? Yes.
		string[ndx] = '\0';
		ndx--;
	}

	return string;
}
