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

#define STOP_COMPILATION() \
    do { \
        fprintf(stderr, "Stopping compilation ...\n"); \
        exit(1); \
    } while (0) 

#define ERR_GENERAL(fmt, ...) \
    do { \
        fprintf(stderr, "ERROR:\t" fmt "\n", ##__VA_ARGS__); \
        STOP_COMPILATION(); \
    } while (0)

#define WARN_GENERAL(fmt, ...) fprintf(stderr, "WARNING:\t" fmt "\n", ##__VA_ARGS__)

#define STR_STARTS_WITH(str, start) (strncmp(str, start, strlen(start)) == 0)

#define CHR_IS_NEWLINE(chr) ((chr) == '\n' || (chr) == '\r')

#define TODO(msg) assert(0 && (msg))

#endif

