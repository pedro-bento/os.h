// os.h by Pedro Bento
//
// Supported platforms
//  - Unix-like Operating Systems
//  - Windows
//
// Data types
//  - Bool
//  - Int8
//  - Int16
//  - Int32
//  - Int64
//  - Uint8
//  - Uint16
//  - Uint32
//  - Uint64
//  - Bytes
//
// Macros
//  - NDEBUG                                                    - when defined, assertions are disabled.
//  - STATIC_ASSERT
//  - TRACE_ERROR                                               - called with a human readable message when an error occurs.
//  - TRUE
//  - FALSE
//  - LARGE_PAGES                                               - when defined, allocations will use large pages.
//
// Functions
//  - Bool Alloc(Uint64 size, Bytes *bytes)                     - alloc size bytes of read-write memory.
//  - Bool Free(Bytes bytes)                                    - free bytes.
//  - Bool MapFile(const char *filePath, String *fileMap)       - map a file into readonly memory.
//  - Bool UnmapFile(String fileMap)                            - unmap a file from memory.

#ifndef OS_H
#define OS_H

#if defined(NDEBUG)
#define STATIC_ASSERT(condition)
#else
#define STATIC_ASSERT(condition) typedef char static_assertion_##__LINE__[(condition) ? 1 : -1]
#endif

#ifndef TRACE_ERROR
#define TRACE_ERROR(cstr)
#endif

typedef char Bool;
#define TRUE 1
#define FALSE 0

typedef char Int8;
typedef short Int16;
typedef int Int32;
typedef long long Int64;

STATIC_ASSERT(sizeof(Int8) == 1);
STATIC_ASSERT(sizeof(Int16) == 2);
STATIC_ASSERT(sizeof(Int32) == 4);
STATIC_ASSERT(sizeof(Int64) == 8);

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long long Uint64;

STATIC_ASSERT(sizeof(Uint8) == 1);
STATIC_ASSERT(sizeof(Uint16) == 2);
STATIC_ASSERT(sizeof(Uint32) == 4);
STATIC_ASSERT(sizeof(Uint64) == 8);

typedef struct {
    Uint8 *base;
    Uint64 size;
} Bytes;

Bool Alloc(Uint64 size, Bytes *bytes);
Bool Free(Bytes bytes);
Bool MapFile(const char *filePath, Bytes *fileMap);
Bool UnmapFile(Bytes fileMap);

#endif

#if defined(OS_IMPLEMENTATION)

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void TraceError() {
    TCHAR tstr[FORMAT_MESSAGE_MAX_WIDTH_MASK];

    DWORD size = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0,
        tstr,
        sizeof(tstr) / sizeof(TCHAR),
        NULL
    );
    if (size == 0) {
        TRACE_ERROR("Could not format a human readable error message");
        return;
    }

#ifdef UNICODE
    char cstr[FORMAT_MESSAGE_MAX_WIDTH_MASK];
    WideCharToMultiByte(CP_UTF8, 0, tstr, -1, cstr, sizeof(cstr), NULL, NULL);
#else
    char *cstr = tstr;
#endif

    TRACE_ERROR(cstr);
}

Bool Alloc(Uint64 size, Bytes *bytes) {
#if defined(LARGE_PAGES)
    Int64 largePages = MEM_LARGE_PAGES;
#else
    Int64 largePages = 0;
#endif

    LPVOID base = VirtualAlloc(
        NULL,
        (SIZE_T) size,
        MEM_RESERVE | MEM_COMMIT | largePages,
        PAGE_READWRITE
    );
    if (base == NULL) {
        TraceError();
        return FALSE;
    }

    bytes->size = size;
    bytes->base = (Uint8*) base;

    return TRUE;
}

Bool Free(Bytes bytes) {
    if (VirtualFree((LPVOID) bytes.base, 0, MEM_RELEASE) == 0) {
        TraceError();
        return FALSE;
    }

    return TRUE;
}

Bool MapFile(const char *filePath, Bytes *fileMap) {
#if defined(UNICODE)
    TCHAR tFilePath[MAX_PATH];
    if (MultiByteToWideChar(CP_ACP, 0, filePath, -1, tFilePath, MAX_PATH) == 0) {
        TraceError();
        return FALSE;
    }
#else
    TCHAR *tFilePath = (TCHAR*) filePath;
#endif

    HANDLE hFile = CreateFile(
        tFilePath,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        TraceError();
        return FALSE;
    }

    if (GetFileSize(hFile, NULL) == 0) {
        CloseHandle(hFile);
        fileMap->size = 0;
        fileMap->base = NULL;
        return TRUE;
    }

#if defined(LARGE_PAGES)
    Int64 largePages = SEC_LARGE_PAGES;
#else
    Int64 largePages = 0;
#endif

    HANDLE hMapping = CreateFileMapping(
        hFile,
        NULL,
        PAGE_READONLY | largePages,
        0,
        0,
        NULL
    );
    if (hMapping == NULL) {
        TraceError();
        CloseHandle(hFile);
        return FALSE;
    }

    LPVOID pMapView = MapViewOfFile(
        hMapping,
        FILE_MAP_READ,
        0,
        0,
        0
    );
    if (pMapView == NULL) {
        TraceError();
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    fileMap->size = (Uint64) GetFileSize(hFile, NULL);
    fileMap->base = (Uint8*) pMapView;

    CloseHandle(hMapping);
    CloseHandle(hFile);

    return TRUE;
}

Bool UnmapFile(Bytes fileMap) {
    if (fileMap.size == 0) {
        return TRUE;
    }

    LPVOID pMapView = (LPVOID) fileMap.base;
    if (!UnmapViewOfFile(pMapView)) {
        TraceError();
        return FALSE;
    }

    return TRUE;
}

#elif defined(__unix__)

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

Bool Alloc(Uint64 size, Bytes *bytes) {
#if defined(LARGE_PAGES)
    Int64 largePages = MAP_HUGETLB;
#else
    Int64 largePages = 0;
#endif

    void *base = mmap(
        NULL,
        (size_t) size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | largePages,
        -1,
        0
    );
    if (base == MAP_FAILED) {
        TRACE_ERROR(strerror(errno));
        return FALSE;
    }

    bytes->size = size;
    bytes->base = (Uint8*) base;

    return TRUE;
}

Bool Free(Bytes bytes) {
    if (munmap((void *) bytes.base, (size_t) bytes.size) != 0) {
        TRACE_ERROR(strerror(errno));
        return FALSE;
    }

    return TRUE;
}

Bool MapFile(const char *filePath, Bytes *fileMap) {
    int fd = open(
        filePath,
        O_RDONLY
    );
    if (fd == -1) {
        TRACE_ERROR(strerror(errno));
        return FALSE;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        TRACE_ERROR(strerror(errno));
        close(fd);
        return FALSE;
    }
    size_t size = st.st_size;
    if (size == 0) {
        close(fd);
        fileMap->size = 0;
        fileMap->base = NULL;
        return TRUE;
    }

#if defined(LARGE_PAGES)
    Int64 largePages = MAP_HUGETLB;
#else
    Int64 largePages = 0;
#endif

    void *data = mmap(
        NULL,
        size,
        PROT_READ,
        MAP_PRIVATE | largePages,
        fd,
        0
    );
    if (data == MAP_FAILED) {
        TRACE_ERROR(strerror(errno));
        close(fd);
        return FALSE;
    }

    fileMap->size = (Uint64) size;
    fileMap->base = (Uint8*) data;

    close(fd);

    return TRUE;
}

Bool UnmapFile(Bytes fileMap) {
    if (fileMap.base == NULL) {
        return TRUE;
    }

    if (munmap((void *) fileMap.base, (size_t) fileMap.size) == -1) {
        TRACE_ERROR(strerror(errno));
        return FALSE;
    }

    return TRUE;
}

#endif

#endif
