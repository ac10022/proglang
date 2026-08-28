#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "file.h"
#include "base.h"
#include "arena.h"

/*
 * Open and read a file from filepath, output its contents into a buffer of MAX_BUFFER_LENGTH
 */
char *read_file(Arena* arena, char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ERR_GENERAL("Failed to read file.");
        return NULL;
    }

    char *buf = PALLOCS(arena, MAX_BUFFER_LENGTH);
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
FileInfo *new_fileinfo(CompilerContext *c_ctx, char *filepath) {
    char *contents = read_file(c_ctx->arena, filepath);
    if (contents == NULL) return NULL;

    FileInfo *info = PALLOCT(c_ctx->arena, FileInfo, 1);
    if (!info) return NULL;

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
