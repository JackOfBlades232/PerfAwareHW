#pragma once

#include "os.hpp"
#include "util.hpp"
#include "profiling.hpp"

enum os_file_mapping_flags_bits_t {
    e_osfmf_largepage = 1,
    e_osfmf_no_init_map = 2
};

typedef uint32_t os_file_mapping_flags_t;

#if _WIN32

#include <windows.h>

struct os_file_t {
    HANDLE hnd = INVALID_HANDLE_VALUE;
    size_t len;
};

inline bool is_valid(os_file_t const &f)
{
    return f.hnd != INVALID_HANDLE_VALUE;
}

inline os_file_t os_read_open_file(const char *fn)
{
    os_file_t f = {};

    f.hnd = CreateFileA(
        fn, GENERIC_READ, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (!is_valid(f))
        return {};

    DWORD len_lo = 0, len_hi = 0;
    len_lo = GetFileSize(f.hnd, &len_hi);
    f.len = (size_t(len_hi) << 32) | size_t(len_lo);

    return f;
}

inline void os_close_file(os_file_t &f)
{
    if (is_valid(f)) {
        CloseHandle(f.hnd);
        f = {};
    }
}

inline size_t os_file_read(os_file_t const &f, void *buf, size_t bytes)
{
    assert(is_valid(f));
    DWORD real_bytes;
    BOOL res = ReadFile(f.hnd, buf, (DWORD)bytes, &real_bytes, nullptr);
    return res ? size_t(real_bytes) : 0;
}

struct os_mapped_file_t {
    char *data;
    size_t len;
    HANDLE file_hnd;
    HANDLE mapping_hnd;
};

inline bool is_valid(os_mapped_file_t const &f)
{
    return
        f.file_hnd != INVALID_HANDLE_VALUE &&
        f.mapping_hnd != INVALID_HANDLE_VALUE;
}

inline bool is_mapped(os_mapped_file_t const &f)
{
    return f.data != nullptr;
}

inline void os_read_map_section(
    os_mapped_file_t &f, size_t off, size_t len)
{
    f.data = (char *)MapViewOfFile(
        f.mapping_hnd, FILE_MAP_READ,
        DWORD(off >> 32), DWORD(off & 0xFFFFFFFF), len);
}

inline void os_unmap_section(os_mapped_file_t &f)
{
    if (f.data) {
        UnmapViewOfFile(f.data);
        f.data = nullptr;
    }
}

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
    len_lo = GetFileSize(file.file_hnd, &len_hi);
    file.len = (size_t(len_hi) << 32) | size_t(len_lo);

    DWORD mapping_flags = PAGE_READONLY;
    if ((flags & e_osfmf_largepage) && g_os_proc_state.large_page_size)
        mapping_flags |= SEC_LARGE_PAGES;

    file.mapping_hnd = CreateFileMappingA(
        file.file_hnd, nullptr, mapping_flags, 0, 0, nullptr);
    if (file.mapping_hnd == INVALID_HANDLE_VALUE) {
        CloseHandle(file.file_hnd);
        return {};
    }

    if (flags & e_osfmf_no_init_map)
        return file;

    os_read_map_section(file, 0, file.len);
    if (!file.data) {
        CloseHandle(file.mapping_hnd);
        CloseHandle(file.file_hnd);
        return {};
    }

    return file;
}

inline void os_unmap_file(os_mapped_file_t &file)
{
    os_unmap_section(file);
    if (file.mapping_hnd != INVALID_HANDLE_VALUE)
        CloseHandle(file.mapping_hnd);
    if (file.file_hnd != INVALID_HANDLE_VALUE)
        CloseHandle(file.file_hnd);
    file = {};
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>
#include <fcntl.h>

struct os_file_t {
    int fd = -1;
    size_t len;
};

inline bool is_valid(os_file_t const &f)
{
    return f.fd >= 0;
}

inline os_file_t os_read_open_file(const char *fn)
{
    os_file_t f = {};

    f.fd = open(fn, O_RDONLY, 0);
    if (!is_valid(f))
        return {};

    f.len = lseek(f.fd, SEEK_END, 0);
    lseek(f.fd, SEEK_SET, 0);

    return f;
}

inline void os_close_file(os_file_t &f)
{
    if (is_valid(f)) {
        close(f.fd);
        f = {};
    }
}

inline size_t os_file_read(os_file_t const &f, void *buf, size_t bytes)
{
    assert(is_valid(f));
    return read(f.fd, buf, bytes);
}

struct os_mapped_file_t {
    char *data;
    size_t len;
    size_t mapped_len;
    int fd;
};

inline bool is_valid(os_mapped_file_t const &f)
{
    return f.fd >= 0;
}

inline bool is_mapped(os_mapped_file_t const &f)
{
    return f.data != nullptr;
}

inline void os_read_map_section(
    os_mapped_file_t &f, size_t off, size_t len)
{
    int mmap_flags = MAP_PRIVATE;
    size_t pagesize = size_t(getpagesize());

    f.mapped_len = round_up(len, pagesize);

    f.data = (char *)mmap(
        nullptr, f.mapped_len, PROT_READ, mmap_flags, f.fd, off_t(off));
    if (f.data == MAP_FAILED)
        f.data = nullptr;
}

inline void os_unmap_section(os_mapped_file_t &f)
{
    if (f.data) {
        munmap(f.data, f.mapped_len);
        f.data = nullptr;
    }
}

inline os_mapped_file_t os_read_map_file(
    const char *fn, os_file_mapping_flags_t flags = 0)
{
    if (flags & e_osfmf_largepage)
        return {}; // Real files are not supported by hugepages

    os_mapped_file_t file = {};
    file.fd = open(fn, O_RDONLY, 0);
    if (file.fd < 0)
        return {};

    file.len = size_t(lseek(file.fd, 0, SEEK_END));

    if (flags & e_osfmf_no_init_map)
        return file;

    os_read_map_section(file, 0, file.len);
    if (!file.data) {
        close(file.fd);
        return {};
    }

    return file;
}

inline void os_unmap_file(os_mapped_file_t &file)
{
    os_unmap_section(file);
    if (file.fd >= 0)
        close(file.fd);
    file = {};
}

#endif
