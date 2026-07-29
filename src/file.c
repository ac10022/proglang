#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../include/file.h"
#include "../include/base.h"

/*
 * Open and read a file from filepath, output its contents into a buffer of MAX_BUFFER_LENGTH
 */
char *read_file(char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ERR_GENERAL("Failed to read file.");
        return NULL;
    }

    char *buf = malloc(MAX_BUFFER_LENGTH);
    if (!buf) {
        ERR_GENERAL("Memory allocation failed.");
        fclose(file);
        return NULL;
    }

    size_t true_len = fread(buf, sizeof(char), MAX_BUFFER_LENGTH - 1, file);

    if (ferror(file) != 0) {
        ERR_GENERAL("Error reading file.");
        free(buf);
        fclose(file);
        return NULL;
    }

    buf[true_len++] = '\0';

    fclose(file);
    return buf;
}

/*
 * Open and read a file from filepath, and return its data in a dynamically allocated FileInfo object.
 */
FileInfo *new_fileinfo(char *filepath) {
    char *contents = read_file(filepath);
    if (contents == NULL) return NULL;

    FileInfo *info = calloc(1, sizeof(FileInfo));
    if (!info) {
        free(contents);
        return NULL;
    }

    info->contents = contents;
    info->filepath = filepath;

    return info;
}

/*
 * Helper function to check if a file exists and we have access to it.
 */
bool check_file_exists(char* filepath) {
    if (!filepath) return false;
    
    FILE* file = fopen(filepath, "r");
    if (file) {
        fclose(file); return true;
    }
    return false;
}
