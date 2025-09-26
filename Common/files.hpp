#pragma once

#include "os.hpp"
#include "util.hpp"
#include "profiling.hpp"

enum os_file_mapping_flags_bits_t {
    e_osfmf_largepage = 1,
};

typedef uint32_t os_file_mapping_flags_t;

#if _WIN32

#include <windows.h>

struct os_mapped_file_t {
    char *data;
    size_t len;
    HANDLE file_hnd;
    HANDLE mapping_hnd;
};

inline os_mapped_file_t os_read_map_file(
    const char *fn, os_file_mapping_flags_t flags = 0)
{
    os_mapped_file_t file = {};

    file.file_hnd = CreateFileA(
        fn, GENERIC_READ, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.file_hnd == INVALID_HANDLE_VALUE)
        return {};

    DWORD len_lo = 0, len_hi = 0;
    len_lo = GetFileSize(file_hnd, &len_hi);
    file.real_len = (size_t(len_hi) << 32) | size_t(len_lo);

    DWORD mapping_flags = PAGE_READONLY;
    if ((flags & e_osfmf_largepage) && g_os_proc_state.large_page_size)
        mapping_flags |= SEC_LARGE_PAGES;

    file.mapping_hnd = CreateFileMappingA(
        file_hnd, nullptr, mapping_flags, 0, 0, nullptr);
    if (file.mapping_hnd == INVALID_HANDLE_VALUE) {
        CloseHandle(file.file_hnd);
        return {};
    }

    file.data = (char *)MapViewOfFile(
        mapping_hnd, FILE_MAP_READ, 0, 0, file.len);
    if (!file.data) {
        CloseHandle(file.mapping_hnd);
        CloseHandle(file.file_hnd);
        return {};
    }

    return file;
}

inline void os_unmap_file(os_mapped_file_t &file)
{
    if (file.data) {
        UnmapViewOfFile(file.data);
        CloseHandle(file.mapping_hnd);
        CloseHandle(file.file_hnd);
        file = {};
    }
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>
#include <fcntl.h>

struct os_mapped_file_t {
    char *data;
    size_t len;
    size_t mapped_len;
    int fd;
};

inline os_mapped_file_t os_read_map_file(
    const char *fn, os_file_mapping_flags_t flags = 0)
{
    if (flags & e_osfmf_largepage)
        return {}; // Real files are not supported by hugepages

    os_mapped_file_t file = {};
    file.fd = open(fn, O_RDONLY, 0);
    if (file.fd < 0)
        return {};

    int mmap_flags = MAP_PRIVATE;
    size_t pagesize = size_t(getpagesize());

    file.len = size_t(lseek(file.fd, 0, SEEK_END));
    file.mapped_len = round_up(file.len, pagesize);

    file.data = (char *)mmap(
        nullptr, file.mapped_len, PROT_READ, mmap_flags, file.fd, 0);
    if (file.data == MAP_FAILED) {
        close(file.fd);
        return {};
    }

    return file;
}

inline void os_unmap_file(os_mapped_file_t &file)
{
    if (file.data) {
        munmap(file.data, file.mapped_len);
        close(file.fd);
        file = {};
    }
}

#endif
