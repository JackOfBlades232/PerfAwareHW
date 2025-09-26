#pragma once

#include "os.hpp"
#include "util.hpp"
#include "profiling.hpp"

#if _WIN32

#include <windows.h>

struct mapped_file_t {
    char *data;
    size_t len;
    HANDLE file_hnd;
    HANDLE mapping_hnd;
};

inline mapped_file_t os_read_map_file(const char *fn)
{
    PROFILED_FUNCTION;

    mapped_file_t file = {};

    file.file_hnd = CreateFileA(
        fn, GENERIC_READ, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.file_hnd == INVALID_HANDLE_VALUE)
        return {};

    DWORD len_lo = 0, len_hi = 0;
    len_lo = GetFileSize(file_hnd, &len_hi);
    file.real_len = (size_t(len_hi) << 32) | size_t(len_lo);

    file.mapping_hnd = CreateFileMappingA(
        file_hnd, nullptr, PAGE_READONLY, 0, 0, nullptr);
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

inline void os_unmap_file(mapped_file_t &file)
{
    PROFILED_FUNCTION;

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
#include <unistd.h>
#include <fcntl.h>

struct mapped_file_t {
    char *data;
    size_t len;
    size_t mapped_len;
    int fd;
};

inline mapped_file_t os_read_map_file(const char *fn)
{
    PROFILED_FUNCTION;

    mapped_file_t file = {};
    file.fd = open(fn, O_RDONLY, 0);
    if (file.fd < 0)
        return {};

    file.len = size_t(lseek(file.fd, 0, SEEK_END));
    file.mapped_len = round_up(file.len, size_t(getpagesize()));

    file.data = (char *)mmap(
        nullptr, file.mapped_len, PROT_READ, MAP_PRIVATE, file.fd, 0);
    if (file.data == MAP_FAILED) {
        close(file.fd);
        return {};
    }

    return file;
}

inline void os_unmap_file(mapped_file_t &file)
{
    PROFILED_FUNCTION;

    if (file.data) {
        munmap(file.data, file.mapped_len);
        close(file.fd);
        file = {};
    }
}

#endif
