#ifndef FILE_H
#define FILE_H

#include "base.h"

typedef struct {
    char *filepath;
    char *contents;
} FileInfo;

FileInfo *new_fileinfo(CompilerContext *c_ctx, char *filepath);
bool check_file_exists(char* filepath);

#endif
