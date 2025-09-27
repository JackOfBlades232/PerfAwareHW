#define RT_PRINT(fmt_, ...) fprintf(stderr, fmt_, ##__VA_ARGS__)
#define RT_PRINTLN(fmt_, ...) fprintf(stderr, fmt_ "\n", ##__VA_ARGS__)
#define RT_CLEAR(count_)                         \
    do {                                         \
        for (size_t i_ = 0; i_ < (count_); ++i_) \
            fprintf(stderr, "\b");               \
    } while (0)

#include <os.hpp>
#include <memory.hpp>
#include <files.hpp>
#include <profiling.hpp>
#include <repetition.hpp>
#include <util.hpp>
#include <defer.hpp>
#include <intrinsics.hpp>

#define RT_STOP_TIME 0.2f

struct buffer_t {
    uint8_t *data;
    size_t len;
    bool is_large_pages;
};

static size_t get_flen(char const *fn)
{
    FILE *fh = fopen(fn, "rb");
    if (!fh)
        return 0;
    DEFER([fh] { fclose(fh); });
    fseek(fh, 0, SEEK_END);
    size_t const len = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    return len;
}

static buffer_t allocate(size_t bytes, bool large_pages = false)
{
    void *data = (large_pages ?
        &allocate_os_large_pages_memory :
        &allocate_os_pages_memory)(bytes);
    assert(data);
    return {(uint8_t *)data, bytes, large_pages};
}

static void deallocate(buffer_t &buf)
{
    if (buf.data) {
        (buf.is_large_pages ?
            &free_os_large_pages_memory :
            &free_os_pages_memory)(xchg(buf.data, nullptr), xchg(buf.len, 0));
    }
}

static char const *make_test_name(
    char const *base, bool large_pages, bool do_work, uint64_t chunk_size)
{
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s, %llukb chunk%s%s",
        base, chunk_size >> 10,
        large_pages ? " + large pages" : "",
        do_work ? " + work loop" : "");
    return buf;
}

// @TODO: threading for work

static int64_t sum_buffer(buffer_t const &buf)
{
    __m512i s1 = _mm512_setzero_si512();
    __m512i s2 = _mm512_setzero_si512();
    __m512i s3 = _mm512_setzero_si512();
    __m512i s4 = _mm512_setzero_si512();

    uint8_t *ptr = buf.data;
    for (uint8_t *ptr = buf.data, *end = buf.data + buf.len;
        ptr < end; ptr += 256) 
    {
        __m512i const l1 = _mm512_load_epi64(ptr);
        __m512i const l2 = _mm512_load_epi64(ptr + 64);
        __m512i const l3 = _mm512_load_epi64(ptr + 128);
        __m512i const l4 = _mm512_load_epi64(ptr + 192);

        s1 = _mm512_add_epi64(s1, l1);
        s2 = _mm512_add_epi64(s2, l2);
        s3 = _mm512_add_epi64(s3, l3);
        s4 = _mm512_add_epi64(s4, l4);
    }

    __m512i const s12 = _mm512_add_epi64(s1, s2);
    __m512i const s34 = _mm512_add_epi64(s3, s4);
    __m512i const s1234 = _mm512_add_epi64(s12, s34);

    __m256i const h = _mm256_add_epi64(
        _mm512_castsi512_si256(s1234),
        _mm512_extracti64x4_epi64(s1234, 1));
    __m128i const q = _mm_add_epi64(
        _mm256_castsi256_si128(h),
        _mm256_extracti128_si256(h, 1));

    return _mm_cvtsi128_si64(_mm_add_epi64(q, _mm_unpackhi_epi64(q, q)));
}

static void run_test(
    auto &&test, size_t file_size, uint64_t cpu_timer_freq, char const *name,
    RepetitionTester &rt, repetition_test_results_t &results)
{
    rt.ReStart(results, file_size);
    do {
        rt.BeginTimeBlock();

        test(file_size);
        
        rt.EndTimeBlock();
        rt.ReportProcessedBytes(file_size);
    } while (rt.Tick());

    print_reptest_results(results, file_size, cpu_timer_freq, name, true);
    printf(",%Lf", best_gbps(results, file_size, cpu_timer_freq));
}

static auto allocate_and_touch_tester(
    uint64_t chunk_size, bool large_pages, bool do_work)
{
    static volatile uint64_t work_sink = 0;
    return [=](size_t file_size) {
        buffer_t workbuf = allocate(chunk_size, large_pages);
        for (size_t i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            memset(workbuf.data, 1, chunk_size);
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        deallocate(workbuf);
    };
}

static auto allocate_and_movsb_tester(
    uint64_t chunk_size, buffer_t const &scratch,
    bool large_pages, bool do_work)
{
    static volatile int64_t work_sink = 0;
    return [=](size_t file_size) {
        buffer_t workbuf = allocate(chunk_size, large_pages);
        uint8_t const *src = scratch.data;
        for (size_t i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            i_movsb(
                (unsigned char *)workbuf.data,
                (unsigned const char *)src,
                chunk_size);
            src += chunk_size;
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        deallocate(workbuf);
    };
}

static auto allocate_and_copy_tester(
    uint64_t chunk_size, buffer_t const &scratch,
    bool large_pages, bool do_work)
{
    static volatile int64_t work_sink = 0;
    return [=](size_t file_size) {
        buffer_t workbuf = allocate(chunk_size, large_pages);
        uint8_t const *src = scratch.data;
        for (size_t i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            memcpy(workbuf.data, src, chunk_size);
            src += chunk_size;
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        deallocate(workbuf);
    };
}

static auto os_read_tester(
    uint64_t chunk_size, char const *fn, bool large_pages, bool do_work)
{
    static volatile int64_t work_sink = 0;
    return [=](size_t file_size) {
        buffer_t workbuf = allocate(chunk_size, large_pages);
        os_file_t f = os_read_open_file(fn);
        for (size_t i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            os_file_read(f, workbuf.data, workbuf.len);
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        os_close_file(f);
        deallocate(workbuf);
    };
}

static auto fread_tester(
    uint64_t chunk_size, char const *fn, bool large_pages, bool do_work)
{
    static volatile int64_t work_sink = 0;
    return [=](size_t file_size) {
        buffer_t workbuf = allocate(chunk_size, large_pages);
        FILE *f = fopen(fn, "rb");
        for (size_t i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            fread(workbuf.data, workbuf.len, 1, f);
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        fclose(f);
        deallocate(workbuf);
    };
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Invalid args, usage: prog [fname].\n");
        return 1;
    }

    init_os_process_state(g_os_proc_state);

    uint64_t const cpu_timer_freq = measure_cpu_timer_freq(0.1l);
    bool has_large_pages = try_enable_large_pages(g_os_proc_state);

    if (has_large_pages) {
        void *test_allocation = allocate_os_large_pages_memory(2 << 20);
        if (!test_allocation) {
            has_large_pages = false;
            fprintf(stderr,
                "Large page allocation test failed with error %lld, "
                "will not be used\n\n", get_last_os_error());
        } else {
            free_os_large_pages_memory(test_allocation, 2 << 20);
            fprintf(stderr, "Enabled large pages\n\n");
        }
    } else {
        fprintf(stderr,
            "Failed to enable large pages, they will not be tested\n\n");
    }

    constexpr uint64_t c_chunk_sizes[] = 
    {
        kb(16), kb(64), kb(256), mb(1), mb(2), mb(4), mb(8), mb(16),
        mb(64), mb(128), mb(256), mb(512), gb(1),
    };

    constexpr char const *c_test_names[] = 
    {
        "Allocate and touch", "Allocate and movsb", "Allocate and copy",
        "Os read", "Fread",
    };

    char const *fn = argv[1];
    size_t const flen = get_flen(fn);

    uint64_t const req_divisor = c_chunk_sizes[ARR_CNT(c_chunk_sizes) - 1];
    if (flen == 0) {
        fprintf(stderr, "Can't open '%s'.\n", fn);
        return 2;
    } else if (flen % req_divisor != 0) {
        fprintf(stderr,
            "File len must be divisible by %llu bytes.\n", req_divisor);
        return 1;
    }

    buffer_t scratch = allocate(flen);

    RepetitionTester rt{flen, cpu_timer_freq, RT_STOP_TIME, true};
    repetition_test_results_t results{};

    printf("Chunk size");
    for (char const *name : c_test_names)
        printf(",%s", name);
    if (has_large_pages) {
        for (char const *name : c_test_names)
            printf(",%s + large pages", name);
    }
    for (char const *name : c_test_names)
        printf(",%s + work", name);
    if (has_large_pages) {
        for (char const *name : c_test_names)
            printf(",%s + work + large pages", name);
    }
    printf("\n");

    for (uint64_t chunk_size : c_chunk_sizes) {
        printf("%llu", chunk_size);
        for (bool do_work : {false, true})
            for (bool large_pages : {false, true})  {
                if (large_pages && !has_large_pages)
                    continue;
                run_test(
                    allocate_and_touch_tester(chunk_size, large_pages, do_work),
                    flen, cpu_timer_freq,
                    make_test_name(c_test_names[0], large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    allocate_and_movsb_tester(chunk_size, scratch, large_pages, do_work),
                    flen, cpu_timer_freq,
                    make_test_name(c_test_names[1], large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    allocate_and_copy_tester(chunk_size, scratch, large_pages, do_work),
                    flen, cpu_timer_freq,
                    make_test_name(c_test_names[2], large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    os_read_tester(chunk_size, fn, large_pages, do_work),
                    flen, cpu_timer_freq,
                    make_test_name(c_test_names[3], large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    fread_tester(chunk_size, fn, large_pages, do_work),
                    flen, cpu_timer_freq,
                    make_test_name(c_test_names[4], large_pages, do_work, chunk_size),
                    rt, results);
            }
        printf("\n");
    }
}
