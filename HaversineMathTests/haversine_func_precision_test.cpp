#include <haversine_math.hpp>

#include <os.hpp>
#include <logging.hpp>
#include <intrinsics.hpp>

struct func_test_t {
    f64 in;
    f64 out;
};

constexpr func_test_t c_cosine_gold_standard[] =
{
    {0.0000000000000000000000000000000000000, 1.0000000000000000000000000000000000000},
    {0.3490658503988659153847381536977228602, 0.9396926207859083840541092773247314699}, // pi / 9
    {0.5235987755982988730771072305465838140, 0.8660254037844386467637231707529361835}, // pi / 6
    {0.7853981633974483096156608458198757210, 0.7071067811865475244008443621048490393}, // pi / 4
    {1.0471975511965977461542144610931676281, 0.5000000000000000000000000000000000000}, // pi / 3
    {1.5707963267948966192313216916397514421, 0.0000000000000000000000000000000000000}, // pi / 2
    {2.0943951023931954923084289221863352562, -0.5000000000000000000000000000000000000}, // 2pi / 3
    {2.3561944901923449288469825374596271631, -0.7071067811865475244008443621048490393}, // 3pi / 4
    {2.6179938779914943643855361527329190701, -0.8660254037844386467637231707529361835}, // 5pi / 6
    {3.1415926535897932384626433832795028842, -1.0000000000000000000000000000000000000}, // pi
};

constexpr func_test_t c_arcsine_gold_standard[] =
{
    {-1.0000000000000000000000000000000000000, -1.5707963267948966192313216916397514421}, // -pi/2
    {-0.8660254037844386467637231707529361835, -1.0471975511965977461542144610931676281}, // -sqrt(3)/2 -> -pi/3
    {-0.7071067811865475244008443621048490393, -0.7853981633974483096156608458198757210}, // -sqrt(2)/2 -> -pi/4
    {-0.5000000000000000000000000000000000000, -0.5235987755982988730771072305465838140}, // -pi/6
    { 0.0000000000000000000000000000000000000,  0.0000000000000000000000000000000000000}, // 0
    { 0.5000000000000000000000000000000000000,  0.5235987755982988730771072305465838140}, // pi/6
    { 0.7071067811865475244008443621048490393,  0.7853981633974483096156608458198757210}, // sqrt(2)/2 -> pi/4
    { 0.8660254037844386467637231707529361835,  1.0471975511965977461542144610931676281}, // sqrt(3)/2 -> pi/3
    { 1.0000000000000000000000000000000000000,  1.5707963267948966192313216916397514421}, // pi/2
};

constexpr func_test_t c_sqrt_gold_standard[] =
{
    {0.0000000000000000000000000000000000000, 0.0000000000000000000000000000000000000}, // sqrt(0) = 0
    {1.0000000000000000000000000000000000000, 1.0000000000000000000000000000000000000}, // sqrt(1) = 1
    {2.0000000000000000000000000000000000000, 1.4142135623730950488016887242096980786}, // sqrt(2)
    {3.0000000000000000000000000000000000000, 1.7320508075688772935274463415058723669}, // sqrt(3)
    {4.0000000000000000000000000000000000000, 2.0000000000000000000000000000000000000}, // sqrt(4) = 2
    {5.0000000000000000000000000000000000000, 2.2360679774997896964091736687312762354}, // sqrt(5)
    {9.0000000000000000000000000000000000000, 3.0000000000000000000000000000000000000}, // sqrt(9) = 3
    {16.000000000000000000000000000000000000, 4.0000000000000000000000000000000000000}, // sqrt(16) = 4
    {25.000000000000000000000000000000000000, 5.0000000000000000000000000000000000000}, // sqrt(25) = 5
    {100.00000000000000000000000000000000000, 10.000000000000000000000000000000000000}, // sqrt(100) = 10
};

struct golden_test_t {
    f64 (*f)(f64);
    func_test_t const *tests;
    usize test_count;
    char const *name;
    f64 allowed_error;
};

struct reference_test_t {
    f64 (*f)(f64);
    f64 (*reference)(f64);
    f64 test_range_min, test_range_max;
    u32 test_count;
    char const *name, *rname;
    f64 allowed_error;
};

#define GTEST(f_, ts_, aerr_) golden_test_t{&f_, ts_, ARR_CNT(ts_), #f_, aerr_}  
#define RTEST(f_, rf_, trmin_, trmax_, tcnt_, aerr_) \
    reference_test_t{&f_, &rf_, (trmin_), (trmax_), (tcnt_), #f_, #rf_, aerr_}  

#if VERBOSE
#define LOGVERBOSE(...) LOGNORMAL(__VA_ARGS__)
#else
#define LOGVERBOSE(...)
#endif

int main(int argc, char **argv)
{
    init_os_process_state(g_os_proc_state);

    for (auto [f, ts, ts_cnt, nm, allowed_err] :
        {
            GTEST(cos, c_cosine_gold_standard, DBL_EPSILON),
            GTEST(asin, c_arcsine_gold_standard, DBL_EPSILON),
            GTEST(sqrt, c_sqrt_gold_standard, DBL_EPSILON)
        })
    {
        LOGNORMAL(
            "Golden test for %s. Allowed error=%.18lf", nm, allowed_err);
        for (usize i = 0; i < ts_cnt; ++i) {
            f64 const in = ts[i].in;
            f64 const res = (*f)(in);
            f64 const req = ts[i].out;
            f64 const err = abs(req - res);
            if (err > allowed_err) {
                LOGNORMAL(
                    "[FAIL] %s(%.18lf). "
                    "Required %.18lf, got %.18lf. Error=%.18lf",
                    nm, in, req, res, err);
            } else {
                LOGNORMAL(
                    "[OK]   %s(%.18lf) = %.18lf. Error=%.18lf",
                    nm, in, res, err);
            }
        }
    }

    LOGNORMAL("");

    for (auto [f, rf, tr_min, tr_max, tcnt, nm, rnm, allowed_err] :
        {
            // @TEST: just against themselves, our stuff will go here
            RTEST(asin, asin, 0.0, 1.0, 1024, 0.0),

            RTEST(sin_quad_approx, sin, -c_pi, c_pi, 1024, DBL_EPSILON),
            RTEST(cos_quad_approx, cos, -c_pi, c_pi, 1024, DBL_EPSILON),

            RTEST(sin_quad_approx, sin, -2.0 * c_pi, 2.0 * c_pi, 1024, DBL_EPSILON),
            RTEST(cos_quad_approx, cos, -2.0 * c_pi, 2.0 * c_pi, 1024, DBL_EPSILON),

            RTEST(i_sqrt, sqrt, 0.0, 1.0, 1024, DBL_EPSILON),
            RTEST(i_sqrt_dc, sqrt, 0.0, 1.0, 1024, DBL_EPSILON),
            RTEST(i_sqrt_approx, sqrt, 0.0, 1.0, 1024, DBL_EPSILON)
        })
    {
        LOGVERBOSE("Reference test for %s (against %s). Allowed error=%.18lf",
            nm, rnm, allowed_err);
        int errcnt = 0;
        float max_error = 0.0;
        for (usize i = 0; i < tcnt; ++i) {
            f64 const in = tr_min + (tr_max - tr_min) * f64(i) / (tcnt - 1);
            f64 const res = (*f)(in);
            f64 const req = (*rf)(in);
            f64 const err = abs(req - res);
            max_error = max(max_error, err);
            if (err > allowed_err) {
                LOGVERBOSE(
                    "[FAIL] %s(%.18lf). Reference %s=%.18lf, "
                    "got %.18lf. Error=%.18lf", nm, in, rnm, req, res, err);
                ++errcnt;
            } else {
                LOGVERBOSE(
                    "[OK]   %s(%.18lf). Reference %s=%.18lf, "
                    "got %.18lf. Error=%.18lf", nm, in, rnm, req, res, err);
            }
        }
        LOGNORMAL(
            "Test for %s (against %s) done, max error %.18lf, %d/%d "
            "under allowed error level of %.18lf",
            nm, rnm, max_error, tcnt - errcnt, tcnt, allowed_err);
    }
}

