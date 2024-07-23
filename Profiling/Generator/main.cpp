#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <cassert>

#define LOGERR(fmt_, ...) fprintf(stderr, "[ERR] " fmt_ "\n", ##__VA_ARGS__)

#define STRERROR_BUF(bufname_, err_) \
    char bufname_[256];              \
    strerror_s(bufname_, 256, err_)

static bool streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

static float randflt(float min, float max)
{
    return ((float)rand() / RAND_MAX)*(max - min) + min;
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

static void output_point_pair(const point_pair_t &point, bool last)
{
    OUTPUT("    {\"x0\": %f, \"y0\": %f, \"x1\": %f, \"y1\": %f}%s\n",
           point.x0, point.y0, point.x1, point.y1, last ? "" : ",");
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

    for (int i = 2; i < argc; ++i) {
        if (streq(argv[i], "-o")) {
            ++i;
            if (i >= argc) {
                LOGERR("Invalid arg, specify file name after -o");
                return 1;
            }

            errno_t err_code;
            if ((err_code = fopen_s(&g_outf, argv[i], "w")) != 0) {
                STRERROR_BUF(err_text, err_code);
                LOGERR("Failed to open %s for write, error: %s", argv[i], err_text);
                return 1;
            }
        } else if (strncmp(argv[i], "-seed=", 6) == 0) {
            const char *p = argv[i] + 6;
            rand_seed = atoll(p);
            if (rand_seed == 0) {
                LOGERR("Invalid arg, specify non-zero seed in -seed=[val]");
                return 1;
            }
        }

        // @TODO: random technique choice
        // @TODO: checksum and checkresult dump to %fname%.check.bin
    }

    init_random(rand_seed);

    OUTPUT("{\n");
    OUTPUT("  \"points\": [\n");

    for (int64_t i = 0; i < point_count; ++i) {
        point_pair_t pair = generate_random_point_pair();
        output_point_pair(pair, i == point_count - 1);
    }

    OUTPUT("  ]\n");
    OUTPUT("}\n");

    return 0;
}
