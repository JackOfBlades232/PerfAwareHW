#include <haversine_calculation.hpp>

#include <os.hpp>
#include <logging.hpp>

struct func_test_t {
    f32 in;
    f32 out;
};

constexpr func_test_t c_cosine_gold_standard[] =
{
    {0.f, 1.f},
    {0.34906585039f, 0.93969262078f},  // pi / 9
    {0.52359877559f, 0.86602540378f},  // pi / 6
    {0.78539816339f, 0.70710678118f},  // pi / 4
    {1.0471975512f,  0.5f},            // pi / 3
    {1.57079632679f, 0.f},             // pi / 2
    {2.09439510239f, -0.5f},           // 2pi / 3
    {2.35619449019f, -0.70710678118f}, // 3pi / 4
    {2.61799387799f, -0.86602540378f}, // 5pi / 6
    {3.14159265359f, -1.f},            // pi
};

constexpr func_test_t c_arcsine_gold_standard[] =
{
    {-1.f,            -1.57079632679f}, // -pi/2
    {-0.86602540378f, -1.0471975512f},  // -sqrt(3)/2 -> -pi/3
    {-0.70710678118f, -0.78539816339f}, // -sqrt(2)/2 -> -pi/4
    {-0.5f,           -0.52359877559f}, // -pi/6
    { 0.f,            0.f},             // 0
    { 0.5f,           0.52359877559f},  // pi/6
    { 0.70710678118f, 0.78539816339f},  // sqrt(2)/2 -> pi/4
    { 0.86602540378f, 1.0471975512f},   // sqrt(3)/2 -> pi/3
    { 1.f,            1.57079632679f},  // pi/2
};

constexpr func_test_t c_sqrt_gold_standard[] =
{
    {0.f,   0.f},            // sqrt(0) = 0
    {1.f,   1.f},            // sqrt(1) = 1
    {2.f,   1.41421356237f}, // sqrt(2)
    {3.f,   1.73205080757f}, // sqrt(3)
    {4.f,   2.f},            // sqrt(4) = 2
    {5.f,   2.2360679775f},  // sqrt(5)
    {9.f,   3.f},            // sqrt(9) = 3
    {16.f,  4.f},            // sqrt(16) = 4
    {25.f,  5.f},            // sqrt(25) = 5
    {100.f, 10.f},           // sqrt(100) = 10
};

struct golden_test_t {
    f32 (*f)(f32);
    func_test_t const *tests;
    usize test_count;
    char const *name;
    f32 allowed_error;
};

struct reference_test_t {
    f32 (*f)(f32);
    f32 (*reference)(f32);
    f32 test_range_min, test_range_max;
    u32 test_count;
    char const *name, *rname;
    f32 allowed_error;
};

#define GTEST(f_, ts_, aerr_) golden_test_t{&f_, ts_, ARR_CNT(ts_), #f_, aerr_}  
#define RTEST(f_, rf_, trmin_, trmax_, tcnt_, aerr_) \
    reference_test_t{&f_, &rf_, (trmin_), (trmax_), (tcnt_), #f_, #rf_, aerr_}  

int main(int argc, char **argv)
{
    init_os_process_state(g_os_proc_state);

    for (auto [f, ts, ts_cnt, nm, allowed_err] :
        {
            GTEST(cosf, c_cosine_gold_standard, FLT_EPSILON),
            GTEST(asinf, c_arcsine_gold_standard, FLT_EPSILON),
            GTEST(sqrtf, c_sqrt_gold_standard, FLT_EPSILON)
        })
    {
        LOGNORMAL(
            "Golden test for %s. Allowed error=%.8f", nm, allowed_err);
        for (usize i = 0; i < ts_cnt; ++i) {
            f32 const in = ts[i].in;
            f32 const res = (*f)(in);
            f32 const req = ts[i].out;
            f32 const err = abs(req - res);
            if (err > allowed_err) {
                LOGNORMAL(
                    "[FAIL] %s(%.8f). Required %.8f, got %.8f. Error=%.8f",
                    nm, in, req, res, err);
            } else {
                LOGNORMAL(
                    "[OK]   %s(%.8f) = %.8f. Error=%.8f",
                    nm, in, res, err);
            }
        }
    }

    LOGNORMAL("");

    // @TEST: just against themselves, our stuff will go here
    for (auto [f, rf, tr_min, tr_max, tcnt, nm, rnm, allowed_err] :
        {
            RTEST(cosf, cosf, -c_pi, c_pi, 1024, 0.f),
            RTEST(asinf, asinf, -1.f, 1.f, 1024, 0.f),
            RTEST(sqrtf, sqrtf, 0.f, 1000.f, 1024, 0.f)
        })
    {
        LOGNORMAL("Reference test for %s (against %s). Allowed error=%.8f",
            nm, rnm, allowed_err);
        int errcnt = 0;
        for (usize i = 0; i < tcnt; ++i) {
            f32 const in = tr_min + (tr_max - tr_min) * f32(i) / (tcnt - 1);
            f32 const res = (*f)(in);
            f32 const req = (*rf)(in);
            f32 const err = abs(req - res);
            if (err > allowed_err) {
                LOGNORMAL(
                    "[FAIL] %s(%.8f). Reference %s(%.8f)=%.8f, "
                    "got %.8f. Error=%.8f", nm, in, rnm, in, req, res, err);
                ++errcnt;
            }
        }
        if (errcnt > 0) {
            LOGNORMAL(
                "[FAIL] test for %s (against %s) failed, %d/%d correct",
                nm, rnm, tcnt - errcnt, tcnt);
        } else {
            LOGNORMAL(
                "[OK]   test for %s (against %s) passed, %d/%d correct",
                nm, rnm, tcnt, tcnt);
        }
    }
}

