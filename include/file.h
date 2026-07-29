#ifndef FILE_H
#define FILE_H

typedef struct {
    char *filepath;
    char *contents;
} FileInfo;

FileInfo* new_fileinfo(char* filepath);
char *read_file(char* filepath);
bool check_file_exists(char* filepath);

#endif
