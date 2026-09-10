#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

bool is_file(const char *source) {
    FILE *fptr = fopen(source, "r");
    if(!fptr) {
        printf("File '%s' not found!.\n", source);
        return false;
    }

    struct stat path_stat;
    if(stat(source, &path_stat) != 0) return false;
    if(!S_ISREG(path_stat.st_mode)) {
        printf("No such file!.\n");
        return false;
    }

    const char *dot = strrchr(source, '.');
    if(!dot || strcmp(dot, ".s8") != 0) {
        printf("Extension must be .s8\n");
        return false;
    }

    return true;
}

char *read(const char *source) {
    if(!is_file(source)) {
        return NULL;
    } else {
        FILE *fptr = fopen(source, "r");
        if(!fptr) return NULL;

        fseek(fptr, 0, SEEK_END);
        long length = ftell(fptr);
        rewind(fptr);

        char *buffer = (char*)malloc(length + 1);
        if(!buffer) {
            fclose(fptr);
            return NULL;
        }

        size_t reader = fread(buffer, 1, length, fptr);
        buffer[reader] = '\0';

        fclose(fptr);
        return buffer;
    }
}

typedef struct CommandLine {
    const char *key;
    const char *des;
} CommandLine;

const CommandLine cmd[] = {
    {"help", "show help message"}, {"version", "show newest version"}
};

const size_t cmd_count = sizeof(cmd) / sizeof(cmd[0]);

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("usage: seight [file_or_options]\n");
        return 1;
    }

    char *c = argv[1];

    if (strcmp(c, "help") == 0) {
        printf("Options:\n");
        for (size_t i = 0; i < cmd_count; i++) {
            printf("    %s - %s\n", cmd[i].key, cmd[i].des);
        }
    }

    return 0;
}