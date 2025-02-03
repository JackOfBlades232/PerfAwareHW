#include <repetition.hpp>
#include <profiling.hpp>

#include <cstdio>

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

// @TODO: verify correctness

// @TODO: sort this out (construction vs reset, Casey is onto smth)
static uint64_t g_cpu_timer_freq = uint64_t(-1);

static void fread_rep_test(const char *fn)
{
    // @TODO: > 4g files, aligned alloc
    FILE *f = fopen(fn, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s for fread\n", fn);
        return;
    }

    fseek(f, 0, SEEK_END);
    long flen = ftell(f);

    file_t fmem = {new char[flen], size_t(flen)};
    fclose(f);

    RepetitionTester rt{"fread", (uint64_t)flen, g_cpu_timer_freq, 10.f, true};
    rt.Start();

    do {
        FILE *f = fopen(fn, "rb");
        fseek(f, 0, SEEK_SET);

        rt.BeginTimeBlock();
        size_t read = fread(fmem.data, fmem.len, 1, f);
        rt.EndTimeBlock();

        if (read != 1)
            rt.ReportError("Failed read");

        rt.ReportProcessedBytes(read * fmem.len);

        fclose(f);
    } while (rt.Tick());

    free_file(fmem);
}

// @TODO: _read, ReadFile, unix syscalls

int main(int argc, char **argv)
{
    g_cpu_timer_freq = measure_cpu_timer_freq(0.1l);

    if (argc < 2) {
        fprintf(stderr, "Invalid args, usage: prog.exe [file name]\n");
        return 1;
    }

    fread_rep_test(argv[1]);
    return 0;
}
