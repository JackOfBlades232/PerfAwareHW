#include <repetition.hpp>
#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <util.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>

#include <fcntl.h>

struct allocator_t {
    void *(*alloc)(size_t);
    void (*free)(void *, size_t);
};

// @TODO: pull out as generic data buffer?
struct file_t {
    char *data;
    size_t len;

    bool Loaded() const { return data != nullptr; }
};

// For tests
static void free_file_preserve_len(file_t &f, allocator_t allocator)
{
    if (f.Loaded()) {
        (*allocator.free)(f.data, f.len);
        f.data = nullptr;
    }
}

static volatile uint32_t g_mapped_file_read_sink = '\0'; // disabling optimization

template <bool t_realloc>
static void mem_write_rep_test(char const *, file_t &mem,
                               RepetitionTester &rt, allocator_t allocator)
{
    do {
        if (!mem.Loaded())
            mem.data = (char *)(*allocator.alloc)(mem.len);

        rt.BeginTimeBlock();
        for (char *p = mem.data; p != mem.data + mem.len; ++p)
            *p = char(p - mem.data);
        rt.EndTimeBlock();

        rt.ReportProcessedBytes(mem.len);

        if constexpr (t_realloc)
            free_file_preserve_len(mem, allocator);
    } while (rt.Tick());

    if (mem.Loaded())
        free_file_preserve_len(mem, allocator);
}

template <bool t_realloc>
static void mem_write_backwards_rep_test(char const *, file_t &mem,
                                         RepetitionTester &rt, allocator_t allocator)
{
    do {
        if (!mem.Loaded())
            mem.data = (char *)(*allocator.alloc)(mem.len);

        rt.BeginTimeBlock();
        for (char *p = mem.data + mem.len - 1; p >= mem.data; --p)
            *p = char(p - mem.data);
        rt.EndTimeBlock();

        rt.ReportProcessedBytes(mem.len);

        if constexpr (t_realloc)
            free_file_preserve_len(mem, allocator);
    } while (rt.Tick());

    if (mem.Loaded())
        free_file_preserve_len(mem, allocator);
}

template <bool t_realloc>
static void fread_rep_test(char const *fn, file_t &mem,
                           RepetitionTester &rt, allocator_t allocator)
{
    do {
        if (!mem.Loaded())
            mem.data = (char *)(*allocator.alloc)(mem.len);

        FILE *f = fopen(fn, "rb");
        assert(f);

        rt.BeginTimeBlock();
        size_t elems = fread(mem.data, mem.len, 1, f);
        rt.EndTimeBlock();

        if (elems != 1)
            rt.ReportError("Failed read");

        rt.ReportProcessedBytes(elems * mem.len);

        fclose(f);

        if constexpr (t_realloc)
            free_file_preserve_len(mem, allocator);
    } while (rt.Tick());

    if (mem.Loaded())
        free_file_preserve_len(mem, allocator);
}

#if _WIN32

#include <io.h>

template <bool t_realloc>
static void _read_rep_test(char const *fn, file_t &mem,
                           RepetitionTester &rt, allocator_t allocator)
{
    do {
        if (!mem.Loaded())
            mem.data = (char *)(*allocator.alloc)(mem.len);

        int fh;
        auto err = _sopen_s(&fh, fn, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0);
        assert(err == 0);

        rt.BeginTimeBlock();
        size_t bytes = _read(fh, mem.data, mem.len);
        rt.EndTimeBlock();

        if (bytes != mem.len)
            rt.ReportError("Failed read");

        rt.ReportProcessedBytes(bytes);

        _close(fh);

        if constexpr (t_realloc)
            free_file_preserve_len(mem, allocator);
    } while (rt.Tick());

    if (mem.Loaded())
        free_file_preserve_len(mem, allocator);
}

#include <winbase.h>
#include <fileapi.h>
#include <handleapi.h>

template <bool t_realloc>
static void ReadFile_rep_test(char const *fn, file_t &mem,
                              RepetitionTester &rt, allocator_t allocator)
{
    do {
        if (!mem.Loaded())
            mem.data = (char *)(*allocator.alloc)(mem.len);

        HANDLE hnd = CreateFileA(
            fn, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        assert(hnd != INVALID_HANDLE_VALUE);

        DWORD bytes;
        rt.BeginTimeBlock();
        BOOL res = ReadFile(hnd, mem.data, (DWORD)mem.len, &bytes, nullptr);
        rt.EndTimeBlock();

        if (!res || bytes != mem.len)
            rt.ReportError("Failed read");

        rt.ReportProcessedBytes(bytes);

        CloseHandle(hnd);

        if constexpr (t_realloc)
            free_file_preserve_len(mem, allocator);
    } while (rt.Tick());

    if (mem.Loaded())
        free_file_preserve_len(mem, allocator);
}

// @TODO: test no remapping too? Maybe someday
static void map_file_rep_test(char const *fn, file_t &mem,
                              RepetitionTester &rt, allocator_t)
{
    do {
        HANDLE file_hnd = CreateFileA(
            fn, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        assert(file_hnd != INVALID_HANDLE_VALUE);

        rt.BeginTimeBlock();

        HANDLE mapping_hnd = CreateFileMappingA(file_hnd, nullptr, PAGE_READONLY, 0, 0, nullptr);
        assert(mapping_hnd != INVALID_HANDLE_VALUE);

        mem.data = (char *)MapViewOfFile(mapping_hnd, FILE_MAP_READ, 0, 0, mem.len);
        if (mem.data) {
            for (char *p = mem.data; p != mem.data + mem.len; ++p)
                g_mapped_file_read_sink += uint32_t(*p);
        } else
            rt.ReportError("Failed file mapping");

        rt.EndTimeBlock();

        rt.ReportProcessedBytes(mem.len);

        UnmapViewOfFile(mem.data);
        CloseHandle(mapping_hnd);
        CloseHandle(file_hnd);
    } while (rt.Tick());
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

template <bool t_realloc>
static void read_rep_test(char const *fn, file_t &mem,
                          RepetitionTester &rt, allocator_t allocator)
{
    do {
        if (!mem.Loaded())
            mem.data = (char *)(*allocator.alloc)(mem.len);

        int fd = open(fn, O_RDONLY, 0);
        assert(fd >= 0);

        rt.BeginTimeBlock();
        size_t bytes = read(fd, mem.data, mem.len);
        rt.EndTimeBlock();

        if (bytes != mem.len)
            rt.ReportError("Failed read");

        rt.ReportProcessedBytes(bytes);

        close(fd);

        if constexpr (t_realloc)
            free_file_preserve_len(mem, allocator);
    } while (rt.Tick());

    if (mem.Loaded())
        free_file_preserve_len(mem, allocator);
}

static void map_file_rep_test(char const *fn, file_t &mem,
                              RepetitionTester &rt, allocator_t)
{
    do {
        int fd = open(fn, O_RDONLY, 0);
        assert(fd >= 0);

        rt.BeginTimeBlock();

        size_t bytes = round_up(mem.len, size_t(getpagesize()));
        mem.data = (char *)mmap(nullptr, bytes, PROT_READ, MAP_PRIVATE, fd, 0);

        if (mem.data != MAP_FAILED) {
            for (char *p = mem.data; p != mem.data + mem.len; ++p)
                g_mapped_file_read_sink += uint32_t(*p);
        } else
            rt.ReportError("Failed file mapping");

        rt.EndTimeBlock();

        rt.ReportProcessedBytes(mem.len);

        munmap(mem.data, bytes);
        close(fd);
    } while (rt.Tick());
}

#endif

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Invalid args, usage: prog.exe [file name]\n");
        return 1;
    }

    char const *fn = argv[1];

    init_os_process_state(g_os_proc_state);
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);
 
    allocator_t const mallocator = {
        .alloc = malloc,
        .free = +[](void *ptr, size_t) { free(ptr); }
    };
    allocator_t const os_page_allocator = {
        .alloc = allocate_os_pages_memory,
        .free = free_os_pages_memory
    };
    allocator_t const os_large_page_allocator = {
        .alloc = allocate_os_large_pages_memory,
        .free = free_os_large_pages_memory
    };

    file_t fmem = {};

    {
        FILE *f = fopen(fn, "rb");
        if (!f) {
            fprintf(stderr, "Failed to open %s to measure length\n", fn);
            return 2;
        }

        // @NOTE: Not bothering with >4gb sizes cause some funcs can't do them.
        fseek(f, 0, SEEK_END);
        fmem.len = (size_t)ftell(f);

        fclose(f);
    }

    bool has_large_pages = try_enable_large_pages(g_os_proc_state);

    if (has_large_pages) {
        void *test_allocation = allocate_os_large_pages_memory(fmem.len);
        if (!test_allocation) {
            has_large_pages = false;
            printf("Large page allocation test failed with error %lld, will not be used\n\n", get_last_os_error());
        } else {
            free_os_large_pages_memory(test_allocation, fmem.len);
            printf("Enabled large pages\n\n");
        }
    } else
        printf("Failed to enable large pages, they will not be tested\n\n");

    struct file_rep_test_t { 
        void (*func)(char const *, file_t &, RepetitionTester &, allocator_t);
        char const *name;
        RepetitionTester rt;
        allocator_t allocator;
        bool enabled = true;
    } tests[] =
    {
        {&mem_write_rep_test<false>, "mem write", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&mem_write_rep_test<true>, "mem write + malloc", {fmem.len, cpu_timer_freq, 10.f, true}, mallocator},
        {&mem_write_rep_test<true>, "mem write + os alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&mem_write_rep_test<true>, "mem write + os large alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_large_page_allocator, has_large_pages},

        {&mem_write_backwards_rep_test<false>, "mem write backwards", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&mem_write_backwards_rep_test<true>, "mem write backwards + malloc", {fmem.len, cpu_timer_freq, 10.f, true}, mallocator},
        {&mem_write_backwards_rep_test<true>, "mem write backwards + os alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&mem_write_backwards_rep_test<true>, "mem write backwards + os large alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_large_page_allocator, has_large_pages},

        {&fread_rep_test<false>, "fread", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&fread_rep_test<true>, "fread + malloc", {fmem.len, cpu_timer_freq, 10.f, true}, mallocator},
        {&fread_rep_test<true>, "fread + os alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&fread_rep_test<true>, "fread + os large alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_large_page_allocator, has_large_pages},

#if _WIN32
        {&_read_rep_test<false>, "_read", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&_read_rep_test<true>, "_read + malloc", {fmem.len, cpu_timer_freq, 10.f, true}, mallocator},
        {&_read_rep_test<true>, "_read + os alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&_read_rep_test<true>, "_read + os large alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_large_page_allocator, has_large_pages},

        {&ReadFile_rep_test<false>, "ReadFile", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&ReadFile_rep_test<true>, "ReadFile + malloc", {fmem.len, cpu_timer_freq, 10.f, true}, mallocator},
        {&ReadFile_rep_test<true>, "ReadFile + os alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&ReadFile_rep_test<true>, "ReadFile + os large alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_large_page_allocator, has_large_pages},
#else
        {&read_rep_test<false>, "read", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&read_rep_test<true>, "read + malloc", {fmem.len, cpu_timer_freq, 10.f, true}, mallocator},
        {&read_rep_test<true>, "read + os alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_page_allocator},
        {&read_rep_test<true>, "read + os large alloc", {fmem.len, cpu_timer_freq, 10.f, true}, os_large_page_allocator, has_large_pages},
#endif

        {&map_file_rep_test, "Map file (MapViewOfFile/mmap)", {fmem.len, cpu_timer_freq, 10.f, true}},
    };

    repetition_test_results_t results = {};

    for (;;) {
        for (size_t i = 0; i < ARR_CNT(tests); ++i) {
            file_rep_test_t &test = tests[i];
            if (!test.enabled)
                continue;
            test.rt.ReStart(results);
            (*test.func)(fn, fmem, test.rt, test.allocator);
            print_reptest_results(
                results, test.rt.GetTargetBytes(),
                cpu_timer_freq, test.name, false);
        }
    }

    return 0;
}
