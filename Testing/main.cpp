#include <repetition.hpp>
#include <profiling.hpp>

#include <cstdio>
#include <cassert>

#include <fcntl.h>

// @TODO: pull out as generic data buffer?
struct file_t {
    char *data;
    size_t len;

    bool Loaded() const { return data != nullptr; }
};

static void free_file(file_t &f)
{
    if (f.Loaded()) {
        delete f.data;
        f.data = nullptr;
        f.len = 0;
    }
}

static void fread_rep_test(const char *fn, file_t &mem, RepetitionTester &rt)
{
    do {
        FILE *f = fopen(fn, "rb");
        assert(f);

        rt.BeginTimeBlock();
        size_t elems = fread(mem.data, mem.len, 1, f);
        rt.EndTimeBlock();

        if (elems != 1)
            rt.ReportError("Failed read");

        rt.ReportProcessedBytes(elems * mem.len);

        fclose(f);
    } while (rt.Tick());
}

#if _WIN32

#include <io.h>

static void _read_rep_test(const char *fn, file_t &mem, RepetitionTester &rt)
{
    do {
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
    } while (rt.Tick());
}

#include <fileapi.h>
#include <handleapi.h>

static void ReadFile_rep_test(const char *fn, file_t &mem, RepetitionTester &rt)
{
    do {
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
    } while (rt.Tick());
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static void read_rep_test(const char *fn, file_t &mem, RepetitionTester &rt)
{
    do {
        int fd = open(fn, O_RDONLY, 0);
        assert(fd >= 0);

        rt.BeginTimeBlock();
        size_t bytes = read(fd, mem.data, mem.len);
        rt.EndTimeBlock();

        if (bytes != mem.len)
            rt.ReportError("Failed read");

        rt.ReportProcessedBytes(bytes);

        close(fd);
    } while (rt.Tick());
}

#endif

int main(int argc, char **argv)
{
    uint64_t cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    if (argc < 2) {
        fprintf(stderr, "Invalid args, usage: prog.exe [file name]\n");
        return 1;
    }

    const char *fn = argv[1];

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

        fmem.data = new char[fmem.len];
        fclose(f);
    }

    struct file_rep_test_t { 
        void (*func)(const char *, file_t &, RepetitionTester &);
        RepetitionTester rt;
    } tests[] =
    {
        {&fread_rep_test, {"fread", fmem.len, cpu_timer_freq, 10.f, true}},
#if _WIN32
        {&_read_rep_test, {"_read", fmem.len, cpu_timer_freq, 10.f, true}},
        {&ReadFile_rep_test, {"ReadFile", fmem.len, cpu_timer_freq, 10.f, true}},
#else
        {&read_rep_test, {"read", fmem.len, cpu_timer_freq, 10.f, true}},
#endif
    };

    for (size_t i = 0; i < ARR_CNT(tests); ++i) {
        file_rep_test_t &test = tests[i];
        test.rt.ReStart();
        (*test.func)(fn, fmem, test.rt);
    }

    free_file(fmem);
    return 0;
}
