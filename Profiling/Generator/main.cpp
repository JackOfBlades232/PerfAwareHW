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
    e_rt_uniform,
    e_rt_clusters
};

struct point_pair_t {
    float x0, y0, x1, y1;
};

struct cluster_t {
    float cx, cy;
    float max_delta;
};

static constexpr unsigned c_max_clusters = 1024;

static random_technique_t g_rand_technique = e_rt_clusters;

static int g_cluster_count = 6;
static cluster_t clusters[c_max_clusters] = {};

static void init_random(unsigned seed)
{
    srand(seed);

    if (g_rand_technique == e_rt_clusters) {
        for (int i = 0; i < g_cluster_count; ++i) {
            clusters[i] = cluster_t{
                randflt(-180.f, 180.f), randflt(-90.f, 90.f), // center
                randflt(5.f, 10.f)                            // max coord delta from center
            };
        }
    }
}

static point_pair_t generate_random_point_pair()
{
    switch (g_rand_technique) {
    case e_rt_uniform:
        return point_pair_t{
            randflt(-180.f, 180.f), randflt(-90.f, 90.f),
            randflt(-180.f, 180.f), randflt(-90.f, 90.f)
        };

    case e_rt_clusters: {
        // Not "fair", but IDGAF
        // Choose first uniformly
        int cid1 = (int)floor(randflt(0.f, g_cluster_count + 0.999f));
        // Choose next uniformly from those remaining
        int cid2 = (cid1 + (int)floor(randflt(0.f, g_cluster_count - 0.001f))) % g_cluster_count;

        const cluster_t &cl1 = clusters[cid1];
        const cluster_t &cl2 = clusters[cid2];

        return point_pair_t{
            cl1.cx + randflt(-cl1.max_delta, cl1.max_delta),
            cl1.cy + randflt(-cl1.max_delta, cl1.max_delta),
            cl2.cx + randflt(-cl2.max_delta, cl2.max_delta),
            cl2.cy + randflt(-cl2.max_delta, cl2.max_delta),
        };
    } break;
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
    float y0r = deg2rad(pair.y0);
    float y1r = deg2rad(pair.y1);

    float hav_of_diff =
        haversine(dy) + cosf(y0r)*cosf(y1r)*haversine(dx);

    float rad_of_diff = acosf(1.f - 2.f*hav_of_diff);
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

    bool specified_cluster_count = false;
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
            rand_seed = atoi(p);
            if (rand_seed <= 0) {
                LOGERR("Invalid arg, specify positive seed in -seed=[val]");
                return 1;
            }
        } else if (strncmp(argv[i], "-random-technique=", 18) == 0) {
            // @TODO: random technique choice
            const char *technique = argv[i] + 18;
            if (streq(technique, "uniform"))
                g_rand_technique = e_rt_uniform;
            else if (streq(technique, "clusters"))
                g_rand_technique = e_rt_clusters;
            else {
                LOGERR("Invalid arg, specify one of [uniform|clusters] in -random-technique=[val]");
                return 1;
            }
        } else if (strncmp(argv[i], "-cluster-count=", 15) == 0) {
            const char *p = argv[i] + 15;
            g_cluster_count = atoi(p);
            if (g_cluster_count <= 0) {
                LOGERR("Invalid arg, specify positive cluster_count in -cluster-count=[val]");
                return 1;
            } else if (g_cluster_count > c_max_clusters) {
                LOGERR("Invalid arg, -cluster-count must be <= %u", c_max_clusters);
                return 1;
            }

            specified_cluster_count = true;
        }
    }

    if (g_rand_technique == e_rt_uniform && specified_cluster_count) {
        LOGERR("Invalid arg, -cluster-count=[val] is invalid when -random-technique=uniform");
        return 1;
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
