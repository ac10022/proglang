#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <stdio.h>

// use this header file for constants or macros across different parts of the project

/*
 * CONSTANTS
 */

#define MAX_BUFFER_LENGTH 4096

/*
 * MACROS
 */

#define ERR_GENERAL(msg) fprintf(stderr, "ERROR:\t%s", (msg))

#define STR_STARTS_WITH(str, start) (strncmp(str, start, strlen(start)) == 0)

#define CHR_IS_NEWLINE(chr) ((chr) == '\n' || (chr) == '\r')

#define TODO(msg) assert(0 && (msg))

#endif

