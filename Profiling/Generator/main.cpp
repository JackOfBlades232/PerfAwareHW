#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <cassert>
#include <cmath>

#define LOGERR(fmt_, ...) fprintf(stderr, "[ERR] " fmt_ "\n", ##__VA_ARGS__)

#define STRERROR_BUF(bufname_, err_) \
    char bufname_[256];              \
    strerror_s(bufname_, 256, err_)

#define MIN(a_, b_) ((a_) < (b_) ? (a_) : (b_))
#define MAX(a_, b_) ((a_) > (b_) ? (a_) : (b_))

static bool streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

static float randflt(float min, float max)
{
    return ((float)rand() / RAND_MAX)*(max - min) + min;
}

static float saturate(float x)
{
    return MAX(0.f, MIN(1.f, x));
}

static FILE *g_outf = stdout;
#define OUTPUT(fmt_, ...) fprintf(g_outf, fmt_, ##__VA_ARGS__)

enum random_technique_t {
    e_rt_uniform
};

static random_technique_t rand_technique = e_rt_uniform;

static void init_random(unsigned seed)
{
    srand(seed);
}

struct point_pair_t {
    float x0, y0, x1, y1;
};

static point_pair_t generate_random_point_pair()
{
    switch (rand_technique) {
    case e_rt_uniform:
        return point_pair_t {
            randflt(-180.f, 180.f), randflt(-90.f, 90.f),
            randflt(-180.f, 180.f), randflt(-90.f, 90.f)
        };

    default:
        assert(0);
    }

    return point_pair_t{};
}

static void output_point_pair(const point_pair_t &pair, bool last)
{
    OUTPUT("    {\"x0\": %f, \"y0\": %f, \"x1\": %f, \"y1\": %f}%s\n",
           pair.x0, pair.y0, pair.x1, pair.y1, last ? "" : ",");
}

static float reference_haversine_dist(const point_pair_t &pair)
{
    constexpr float c_pi = 3.14159265359f;

    auto deg2rad = [](float deg) { return deg * c_pi / 180.f; };
    auto rad2deg = [](float rad) { return rad * 180.f / c_pi; };

    auto haversine = [](float rad) { return (1.f - cosf(rad)) * 0.5f; };

    float dx = deg2rad(fabsf(pair.x0 - pair.x1));
    float dy = deg2rad(fabsf(pair.y0 - pair.y1));

    float hav_of_diff =
        haversine(dx) + cosf(pair.x0)*cosf(pair.x1)*haversine(dy);

    float rad_of_diff = acosf(saturate(1.f - 2.f*hav_of_diff));
    return rad2deg(rad_of_diff);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGERR("Invalid args, usage: exe [point count] [args]");
        return 1;
    }

    int64_t point_count = atoll(argv[1]);
    if (point_count <= 0) {
        LOGERR("Invalid point count, must be an integer > 0");
        return 1;
    }

    unsigned rand_seed = time(nullptr);

    FILE *checksum_f = nullptr;

    for (int i = 2; i < argc; ++i) {
        if (streq(argv[i], "-o")) {
            ++i;
            if (i >= argc) {
                LOGERR("Invalid arg, specify file name after -o");
                return 1;
            }

            auto open_file_or_err = [](const char *fname, const char *mode) -> FILE * {
                errno_t err_code;
                FILE *f;
                if ((err_code = fopen_s(&f, fname, mode)) != 0) {
                    STRERROR_BUF(err_text, err_code);
                    LOGERR("Failed to open %s for write, error: %s",
                           fname,
                           err_text);
                    exit(1);
                }

                return f;
            };

            char checksum_fname[256];
            snprintf(checksum_fname, sizeof(checksum_fname), "%s.check.bin", argv[i]);

            g_outf     = open_file_or_err(argv[i], "w");
            checksum_f = open_file_or_err(checksum_fname, "wb");
        } else if (strncmp(argv[i], "-seed=", 6) == 0) {
            const char *p = argv[i] + 6;
            rand_seed = atoll(p);
            if (rand_seed == 0) {
                LOGERR("Invalid arg, specify non-zero seed in -seed=[val]");
                return 1;
            }
        }

        // @TODO: random technique choice
    }

    init_random(rand_seed);

    OUTPUT("{\n");
    OUTPUT("  \"points\": [\n");

    float sum = 0.f;

    for (int64_t i = 0; i < point_count; ++i) {
        point_pair_t pair = generate_random_point_pair();
        output_point_pair(pair, i == point_count - 1);

        float dist = reference_haversine_dist(pair);
        sum += dist;

        if (checksum_f)
            fwrite(&dist, sizeof(dist), 1, checksum_f);
        else
            OUTPUT("    // dist=%f\n", dist);
    }

    float avg = sum / point_count;
    if (checksum_f)
        fwrite(&avg, sizeof(avg), 1, checksum_f);
    else
        OUTPUT("  // avg=%f\n", avg);

    OUTPUT("  ]\n");
    OUTPUT("}\n");

    if (checksum_f)
        fclose(checksum_f);
    if (g_outf)
        fclose(g_outf);

    return 0;
}
