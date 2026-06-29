#include <stdio.h>
#include <stdlib.h>
#include "../include/file.h"
#include "../include/base.h"

char *F_ReadFile(char* filepath) {
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

FileInfo *F_NewFileInfo(char *filepath) {
    char *contents = F_ReadFile(filepath);
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
