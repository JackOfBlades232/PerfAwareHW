#include <os.hpp>
#include <memory.hpp>
#include <buffer.hpp>
#include <files.hpp>
#include <profiling.hpp>
#include <repetition.hpp>
#include <defs.hpp>
#include <defer.hpp>
#include <intrinsics.hpp>
#include <threads.hpp>

#define RT_STOP_TIME 10.f

static usize get_flen(char const *fn)
{
    FILE *fh = fopen(fn, "rb");
    if (!fh)
        return 0;
    DEFER([fh] { fclose(fh); });
    fseek(fh, 0, SEEK_END);
    usize const len = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    return len;
}

static buffer_t allocate(usize bytes, bool large_pages)
{
    buffer_t b = large_pages ? allocate_lp(bytes) : allocate(bytes);
    assert(is_valid(b));
    return b;
}

static char const *make_test_name(
    char const *base, bool large_pages, bool do_work, u64 chunk_size)
{
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s, %llukb chunk%s%s",
        base, chunk_size >> 10,
        large_pages ? " + large pages" : "",
        do_work ? " + work loop" : "");
    return buf;
}

static i64 sum_buffer(buffer_t const &buf)
{
#if AVX512
    __m512i s1 = _mm512_setzero_si512();
    __m512i s2 = _mm512_setzero_si512();
    __m512i s3 = _mm512_setzero_si512();
    __m512i s4 = _mm512_setzero_si512();

    for (u8 *ptr = buf.data, *end = buf.data + buf.len;
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
#else
    __m256i s1 = _mm256_setzero_si256();
    __m256i s2 = _mm256_setzero_si256();
    __m256i s3 = _mm256_setzero_si256();
    __m256i s4 = _mm256_setzero_si256();

    for (__m256i const
            *ptr = (__m256i const *)buf.data,
            *end = (__m256i const *)(buf.data + buf.len);
        ptr < end;
        ptr += 4) 
    {
        __m256i const l1 = _mm256_load_si256((__m256i const *)ptr);
        __m256i const l2 = _mm256_load_si256((__m256i const *)ptr + 1);
        __m256i const l3 = _mm256_load_si256((__m256i const *)ptr + 2);
        __m256i const l4 = _mm256_load_si256((__m256i const *)ptr + 3);

        s1 = _mm256_add_epi64(s1, l1);
        s2 = _mm256_add_epi64(s2, l2);
        s3 = _mm256_add_epi64(s3, l3);
        s4 = _mm256_add_epi64(s4, l4);
    }

    __m256i const s12 = _mm256_add_epi64(s1, s2);
    __m256i const s34 = _mm256_add_epi64(s3, s4);

    __m256i const h = _mm256_add_epi64(s12, s34);
#endif

    __m128i const q = _mm_add_epi64(
        _mm256_castsi256_si128(h),
        _mm256_extracti128_si256(h, 1));

    return _mm_cvtsi128_si64(_mm_add_epi64(q, _mm_unpackhi_epi64(q, q)));
}

static void run_test(
    auto &&test, usize file_size, u64 req_workres,
    u64 cpu_timer_freq, char const *name,
    RepetitionTester &rt, repetition_test_results_t &results)
{
    rt.ReStart(results, file_size);
    do {
        rt.BeginTimeBlock();

        test(file_size, req_workres);
        
        rt.EndTimeBlock();
        rt.ReportProcessedBytes(file_size);
    } while (rt.Tick());

    print_reptest_results(results, file_size, cpu_timer_freq, name, true);
    printf(",%lf", best_gbps(results, file_size, cpu_timer_freq));
}

static auto allocate_and_touch_tester(
    u64 chunk_size, bool large_pages, bool do_work)
{
    static volatile u64 work_sink = 0;
    return [=](usize file_size, u64) {
        work_sink = 0;
        buffer_t workbuf = allocate(chunk_size, large_pages);
        for (usize i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            memset(workbuf.data, 1, chunk_size);
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        deallocate(workbuf);

        return work_sink;
    };
}

static auto allocate_and_movsb_tester(
    u64 chunk_size, buffer_t const &scratch, bool large_pages, bool do_work)
{
    static i64 volatile work_sink = 0;
    return [=](usize file_size, u64) {
        work_sink = 0;
        buffer_t workbuf = allocate(chunk_size, large_pages);
        u8 const *src = scratch.data;
        for (usize i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            i_movsb((uchar *)workbuf.data, (uchar const *)src, chunk_size);
            src += chunk_size;
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        deallocate(workbuf);

        return work_sink;
    };
}

static auto allocate_and_copy_tester(
    u64 chunk_size, buffer_t const &scratch, bool large_pages, bool do_work)
{
    static i64 volatile work_sink = 0;
    return [=](usize file_size, u64) {
        work_sink = 0;
        buffer_t workbuf = allocate(chunk_size, large_pages);
        u8 const *src = scratch.data;
        for (usize i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            memcpy(workbuf.data, src, chunk_size);
            src += chunk_size;
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        deallocate(workbuf);

        return work_sink;
    };
}

static auto os_read_tester(
    u64 chunk_size, char const *fn,
    bool large_pages, bool do_work, bool validate_work = true)
{
    static i64 volatile work_sink = 0;
    return [=](usize file_size, u64 req_workres) {
        work_sink = 0;
        buffer_t workbuf = allocate(chunk_size, large_pages);
        os_file_t f = os_read_open_file(fn);
        for (usize i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            os_file_read(f, workbuf.data, workbuf.len);
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        os_close_file(f);
        deallocate(workbuf);

        if (do_work && validate_work)
            assert(work_sink == req_workres);
        return work_sink;
    };
}

static auto fread_tester(
    u64 chunk_size, char const *fn,
    bool large_pages, bool do_work, bool validate_work = true)
{
    static i64 volatile work_sink = 0;
    return [=](usize file_size, u64 req_workres) {
        work_sink = 0;
        buffer_t workbuf = allocate(chunk_size, large_pages);
        FILE *f = fopen(fn, "rb");
        for (usize i = 0, iters = file_size / chunk_size; i < iters; ++i) {
            fread(workbuf.data, workbuf.len, 1, f);
            if (do_work)
                work_sink += sum_buffer(workbuf);
        }
        fclose(f);
        deallocate(workbuf);

        if (do_work && validate_work)
            assert(work_sink == req_workres);
        return work_sink;
    };
}

enum buffer_state_t {
    e_bs_written,
    e_bs_read,
};

struct threaded_work_buffer_t {
    buffer_t buf = {};
    buffer_state_t volatile state = e_bs_written;
};

struct threaded_work_block_t {
    threaded_work_buffer_t buffers[2];
    i64 volatile *output;
    u64 iters;
};

// @NOTE: here I trust modern compilers to treat _mm_mfence as a memory
// clobber, which as far as I am aware they generally do. Not safe at all))

static THREAD_ENTRY(work_loop, wb)
{
    auto *work = (threaded_work_block_t *)wb;

    for (u64 i = 0; i < work->iters; ++i) {
        threaded_work_buffer_t &buf = work->buffers[i & 1];

        while (buf.state != e_bs_read)
            _mm_pause();

        _mm_mfence();

        *work->output += sum_buffer(buf.buf);

        _mm_mfence();

        buf.state = e_bs_written;

        i_full_compiler_barrier();
    }

    return 0;
}

static auto os_read_threaded_work_tester(
    u64 chunk_size, char const *fn,
    bool large_pages, bool validate_work = true)
{
    static i64 volatile work_sink = 0;
    return [=](usize file_size, u64 req_workres) {
        work_sink = 0;
        u64 const buf_size = chunk_size / 2;
        u64 const iters = file_size / buf_size;
        threaded_work_block_t wb = {};
        wb.buffers[0].buf = allocate(buf_size, large_pages);
        wb.buffers[1].buf = allocate(buf_size, large_pages);
        wb.output = &work_sink;
        wb.iters = iters;

        os_thread_t worker = os_spawn_thread(&work_loop, &wb);
        assert(is_valid(worker));

        os_file_t f = os_read_open_file(fn);
        for (u64 i = 0; i < iters; ++i) {
            threaded_work_buffer_t &buf = wb.buffers[i & 1];

            while (buf.state != e_bs_written)
                _mm_pause();

            _mm_mfence();

            os_file_read(f, buf.buf.data, buf.buf.len);

            _mm_mfence();

            buf.state = e_bs_read;

            i_full_compiler_barrier();
        }

        os_join_thread(worker);
        os_close_file(f);

        deallocate(wb.buffers[0].buf);
        deallocate(wb.buffers[1].buf);

        if (validate_work)
            assert(work_sink == req_workres);
        return work_sink;
    };
}

static auto map_tester(
    u64 chunk_size, char const *fn, bool validate_work = true)
{
    static i64 volatile work_sink = 0;
    return [=](usize file_size, u64 req_workres) {
        work_sink = 0;
        os_mapped_file_t f = os_read_map_file(fn, e_osfmf_no_init_map);
        for (usize off = 0; off < file_size; off += chunk_size) {
            os_read_map_section(f, off, chunk_size);
            work_sink += sum_buffer(
                buffer_t{(u8 *)f.data, chunk_size, false});
            os_unmap_section(f);
        }
        os_unmap_file(f);

        if (validate_work)
            assert(work_sink == req_workres);
        return work_sink;
    };
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Invalid args, usage: prog [fname].\n");
        return 1;
    }

    init_os_process_state(g_os_proc_state);

    u64 const cpu_timer_freq = measure_cpu_timer_freq(0.1l);
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

    constexpr u64 c_chunk_sizes[] = 
    {
        kb(16), kb(64), kb(256), mb(1), mb(2), mb(4), mb(8), mb(16),
        mb(64), mb(128), mb(256), mb(512), gb(1),
    };

    constexpr char const *c_regular_test_names[] = 
    {
        "Allocate and touch", "Allocate and movsb", "Allocate and copy",
        "Os read", "Fread"
    };
    constexpr char const *c_work_only_test_names[] = 
    {
        "Os read + thread", "Map"
    };

    char const *fn = argv[1];
    usize const flen = get_flen(fn);

    u64 const req_divisor = c_chunk_sizes[ARR_CNT(c_chunk_sizes) - 1];
    if (flen == 0) {
        fprintf(stderr, "Can't open '%s'.\n", fn);
        return 2;
    } else if (flen % req_divisor != 0) {
        fprintf(stderr,
            "File len must be divisible by %llu bytes.\n", req_divisor);
        return 1;
    }

    u64 const req_workres =
        os_read_tester(flen, fn, false, true, false)(flen, 0);

    buffer_t scratch = allocate(flen);

    RepetitionTester rt{flen, cpu_timer_freq, RT_STOP_TIME, true};
    repetition_test_results_t results{};

    printf("Chunk size");
    for (char const *name : c_regular_test_names)
        printf(",%s", name);
    if (has_large_pages) {
        for (char const *name : c_regular_test_names)
            printf(",%s + large pages", name);
    }
    for (char const *name : c_regular_test_names)
        printf(",%s + work", name);
    for (char const *name : c_work_only_test_names)
        printf(",%s", name);
    if (has_large_pages) {
        for (char const *name : c_regular_test_names)
            printf(",%s + work + large pages", name);
    }
    if (has_large_pages) {
        for (char const *name : c_work_only_test_names)
            printf(",%s + large pages", name);
    }
    printf("\n");

    for (u64 chunk_size : c_chunk_sizes) {
        printf("%llu", chunk_size);
        for (bool do_work : {false, true})
            for (bool large_pages : {false, true})  {
                if (large_pages && !has_large_pages)
                    continue;
                run_test(
                    allocate_and_touch_tester(
                        chunk_size, large_pages, do_work),
                    flen, req_workres, cpu_timer_freq,
                    make_test_name(c_regular_test_names[0],
                        large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    allocate_and_movsb_tester(
                        chunk_size, scratch, large_pages, do_work),
                    flen, req_workres, cpu_timer_freq,
                    make_test_name(c_regular_test_names[1],
                        large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    allocate_and_copy_tester(
                        chunk_size, scratch, large_pages, do_work),
                    flen, req_workres, cpu_timer_freq,
                    make_test_name(c_regular_test_names[2],
                        large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    os_read_tester(
                        chunk_size, fn, large_pages, do_work),
                    flen, req_workres, cpu_timer_freq,
                    make_test_name(c_regular_test_names[3],
                        large_pages, do_work, chunk_size),
                    rt, results);
                run_test(
                    fread_tester(
                        chunk_size, fn, large_pages, do_work),
                    flen, req_workres, cpu_timer_freq,
                    make_test_name(c_regular_test_names[4],
                        large_pages, do_work, chunk_size),
                    rt, results);
                if (do_work) {
                    run_test(
                        os_read_threaded_work_tester(
                            chunk_size, fn, large_pages),
                        flen, req_workres, cpu_timer_freq,
                        make_test_name(c_work_only_test_names[0],
                            large_pages, true, chunk_size),
                        rt, results);

                    // No large pages cuz linux, 64k chunk for alloc gran
                    if (!large_pages && chunk_size % kb(64) == 0) {
                        run_test(
                            map_tester(chunk_size, fn),
                            flen, req_workres, cpu_timer_freq,
                            make_test_name(c_work_only_test_names[1],
                                large_pages, true, chunk_size),
                            rt, results);
                    } else {
                        printf(",0");
                    }
                }
            }
        printf("\n");
    }
}
