#ifndef FILE_H
#define FILE_H

typedef struct {
    char *filepath;
    char *contents;
} FileInfo;

FileInfo* F_NewFileInfo(char* filepath);

char *F_ReadFile(char* filepath);
#endif
