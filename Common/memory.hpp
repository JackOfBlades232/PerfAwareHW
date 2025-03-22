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
    // For some reason I can barely make it work on my low-ram pc( Is fragmentation that bad?
    if (g_os_proc_state.large_page_size == 0)
        return nullptr;
    const size_t bytes_for_pages = round_up(bytes, g_os_proc_state.large_page_size);
    return VirtualAlloc(nullptr, bytes_for_pages, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
}

inline void free_os_large_pages_memory(void *mem, size_t)
{
    free_os_pages_memory(mem, 0);
}

#else

#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/resource.h>

inline void *allocate_os_pages_memory(size_t bytes)
{
    const size_t bytes_for_pages =
        round_up(bytes, g_os_proc_state.regular_page_size);
    return mmap(nullptr, bytes_for_pages, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

inline void free_os_pages_memory(void *mem, size_t bytes)
{
    munmap(mem, round_up(bytes, g_os_proc_state.regular_page_size));
}

// @TODO: sort out if page size should be dynamically decided and if we should try for 1g
// This is for appication usage, for measurements should be enough
inline void *allocate_os_large_pages_memory(size_t bytes)
{
    void *ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB, 0, 0);
    return ptr == MAP_FAILED ? nullptr : ptr;
}

inline void free_os_large_pages_memory(void *mem, size_t bytes)
{
    constexpr size_t c_huge_page_alignment = 2 << 20;
    munmap(mem, round_up(bytes, c_huge_page_alignment));
}

#endif

inline void page_memory_in(void *mem, size_t bytes)
{
    char *m = (char *)mem;
    for (char *p = m; p < m + bytes; p += 8)
        *p = char(m - p);
}
