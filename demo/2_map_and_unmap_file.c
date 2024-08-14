#include <stdio.h>

#define TRACE_ERROR(error) printf("[ERROR] %s\n", (error))
// #define LARGE_PAGES

#define OS_IMPLEMENTATION
#include "../os.h"

void MapAndUnmapFile(const char *filePath) {
    Bytes fileMap;
    if (!MapFile(filePath, &fileMap)) {
        printf("[ERROR] Could not map file `%s` into memory\n", filePath);
        return;
    }

    printf("%s:\n%s", filePath, (char*) fileMap.base);

    if (!UnmapFile(fileMap)) {
        printf("[ERROR] Could not unmap file `%s` from memory\n", filePath);
        return;
    }
}

void main() {
    MapAndUnmapFile("./demo/lorem_ipsum");
    MapAndUnmapFile("./demo/empty_file");
}
