#pragma once

#include "os.hpp"
#include "util.hpp"

#if _WIN32

#include <windows.h>

inline void *allocate_os_pages_memory(size_t bytes)
{
    return VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

inline void free_os_pages_memory(void *mem, size_t)
{
    VirtualFree(mem, 0, MEM_RELEASE);
}

inline void *allocate_os_large_pages_memory(size_t bytes)
{
    // @TODO: have not been able to make it work yet, verify
    if (g_os_proc_state.large_page_sz == 0)
        return nullptr;
    const size_t bytes_for_pages = round_up(bytes, g_os_proc_state.large_page_sz);
    return VirtualAlloc(nullptr, bytes_for_pages, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
}

inline void free_os_large_pages_memory(void *mem, size_t)
{
    free_os_pages_memory(mem, 0);
}

#else

#include <sys/mman.h>
#include <unistd.h>

inline void *allocate_os_pages_memory(size_t bytes)
{
    const size_t bytes_for_pages = round_up(bytes, getpagesize());
    return mmap(nullptr, bytes_for_pages, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

inline void free_os_pages_memory(void *mem, size_t bytes)
{
    munmap(mem, bytes);
}

inline void *allocate_os_large_pages_memory(size_t bytes)
{
    // @TODO
    return nullptr;
}

inline void free_os_large_pages_memory(void *mem, size_t)
{
    // @TODO
}

#endif
