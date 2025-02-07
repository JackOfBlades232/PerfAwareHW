#pragma once

#if _WIN32

#include <windows.h>

inline void *allocate_os_pages_memory(size_t bytes)
{
    return VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

inline void free_os_pages_memory(void *mem, size_t)
{
    VirtualFree(mem, 0, MEM_RELEASE);
}

#else

#include <sys/mman.h>
#include <unistd.h>

inline void *allocate_os_pages_memory(size_t bytes)
{
    const size_t pgsz = getpagesize();
    const size_t pgcnt = (bytes - 1) / pgsz + 1;
    return mmap(nullptr, pgsz * pgcnt, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

inline void free_os_pages_memory(void *mem, size_t bytes)
{
    munmap(mem, bytes);
}

#endif
