#include <stdio.h>
#include <string.h>

#define TRACE_ERROR(error) printf("[ERROR] %s\n", (error))
// #define LARGE_PAGES

#define OS_IMPLEMENTATION
#include "../os.h"

#define KB(N) ((N)*1024)

void main() {
    // Uint64 size = KB(256);
    Uint64 size = 256;

    Bytes bytes;
    if (!Alloc(size, &bytes)) {
        printf("[ERROR] Could not allocate `%llu` bytes of memory\n", size);
        return;
    }

    memset(bytes.base, 'E', bytes.size);
    printf("bytes[%llu]=%.*s\n", bytes.size, (int) bytes.size, bytes.base);

    if (!Free(bytes)) {
        printf("[ERROR] Could not free `%lld` bytes of memory\n", size);
        return;
    }
}
