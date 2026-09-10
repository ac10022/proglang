#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "file.h"
#include "base.h"
#include "arena.h"

/*
 * Open and read a file from filepath, output its contents into a buffer of MAX_BUFFER_LENGTH
 */
char *read_file(CompilerContext* c_ctx, char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ERR_HALT_CTX(c_ctx->cl_ctx, "Failed to read file '%s'.", filepath);
        return NULL;
    }

    char *buf = PALLOCS(c_ctx->arena, MAX_BUFFER_LENGTH);
    if (!buf) {
        ERR_HALT_CTX(c_ctx->cl_ctx, "Memory allocation failed.");
        fclose(file);
        return NULL;
    }

    size_t true_len = fread(buf, sizeof(char), MAX_BUFFER_LENGTH - 1, file);

    if (ferror(file) != 0) {
        ERR_HALT_CTX(c_ctx->cl_ctx, "Error reading file.");
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
    char *contents = read_file(c_ctx, filepath);
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

/*
 * Helper function to replace the extension of a file.
 */
char* replace_ext(Arena* arena, char* filepath, char* suffix) {
    if (!arena) return NULL;
    if (!filepath) return NULL;
    if (!suffix) return NULL;

    size_t len = strlen(filepath);
    size_t suffix_len = strlen(suffix);
    if (len == 0) return NULL;

    // find the final path component
    size_t start = 0;
    for (size_t i = 0; i < len; i++) {
#ifdef _WIN32
        if (filepath[i] == '/' || filepath[i] == '\\') start = i + 1;
#else
        if (filepath[i] == '/') start = i + 1;
#endif
    }

    // case where the / is the end of the string
    if (start == len) return NULL;

    // find the . part of the extension
    size_t end_len = len;
    for (size_t i = len; i > start + 1; i--) {
        if (filepath[i-1] == '.') {
            end_len = i - 1;
            break;
        }
    }

    char* outpath = PALLOCS(arena, end_len + suffix_len + 1);
    // output filepath stripped of ending
    memcpy(outpath, filepath, end_len);
    // append suffix
    memcpy(outpath + end_len, suffix, suffix_len);

    // null terminate
    outpath[end_len + suffix_len] = '\0';
    
    return outpath;
}
