#include <haversine_calculation.hpp>

#include <defs.hpp>
#include <logging.hpp>

template <usize t_n>
static char const *argpref(char const *arg, char const (&val)[t_n])
{
    if (strncmp(arg, val, t_n - 1) != 0)
        return nullptr;
    return arg + (t_n - 1);
}

static f64 randflt(f64 min, f64 max)
{
    return clamp((f64(rand()) / RAND_MAX) * (max - min) + min, min, max);
}

static FILE *g_outf = stdout;
#define OUTPUT(fmt_, ...) fprintf(g_outf, fmt_, ##__VA_ARGS__)

enum random_technique_t {
    e_rt_uniform,
    e_rt_clusters
};

struct cluster_t {
    f64 cx, cy;
    f64 max_delta;
};

static constexpr uint c_max_clusters = 1024;

static random_technique_t g_rand_technique = e_rt_clusters;

static uint g_cluster_count = 6;
static cluster_t clusters[c_max_clusters] = {};

static void init_random(uint seed)
{
    srand(seed);

    if (g_rand_technique == e_rt_clusters) {
        for (uint i = 0; i < g_cluster_count; ++i) {
            clusters[i] = cluster_t{
                randflt(-180.0, 180.0), randflt(-90.0, 90.0), // center
                randflt(5.0, 10.0)                            // max coord delta from center
            };
        }
    }
}

static point_pair_t generate_random_point_pair()
{
    switch (g_rand_technique) {
    case e_rt_uniform:
        return point_pair_t{
            randflt(-180.0, 180.0), randflt(-90.0, 90.0),
            randflt(-180.0, 180.0), randflt(-90.0, 90.0)
        };

    case e_rt_clusters: {
        // Not "fair", but IDGAF
        // Choose first uniformly
        int cid1 = int(floor(randflt(0.0, g_cluster_count + 0.999))) % g_cluster_count;
        // Choose next uniformly from those remaining
        int cid2 = (cid1 + int(floor(randflt(0.0, g_cluster_count - 0.001)))) % g_cluster_count;

        cluster_t const &cl1 = clusters[cid1];
        cluster_t const &cl2 = clusters[cid2];

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

static void output_point_pair(point_pair_t const &pair, bool last)
{
    OUTPUT(
        "    "
        "{\"x0\": %.18lf, \"y0\": %.18lf, \"x1\": %.18lf, \"y1\": %.18lf}%s\n",
        pair.x0, pair.y0, pair.x1, pair.y1, last ? "" : ",");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGERR("Invalid args, usage: exe [point count] [args]");
        return 1;
    }

    i64 point_count = atoll(argv[1]);
    if (point_count <= 0) {
        LOGERR("Invalid point count, must be an integer > 0");
        return 1;
    }

    uint rand_seed = time(nullptr);

    FILE *checksum_f = nullptr;

    bool specified_cluster_count = false;
    for (int i = 2; i < argc; ++i) {
        if (streq(argv[i], "-o")) {
            ++i;
            if (i >= argc) {
                LOGERR("Invalid arg, specify file name after -o");
                return 1;
            }

            auto open_file_or_err = [](char const *fname, char const *mode) -> FILE * {
                FILE *f = fopen(fname, mode);
                if (!f) {
                    LOGERR("Failed to open %s for write, error: %s",
                           fname, strerror(errno));
                    exit(1);
                }

                return f;
            };

            char checksum_fname[256];
            snprintf(checksum_fname, sizeof(checksum_fname), "%s.check.bin", argv[i]);

            g_outf     = open_file_or_err(argv[i], "w+");
            checksum_f = open_file_or_err(checksum_fname, "wb+");
        } else if (char const *p = argpref(argv[i], "-seed=")) {
            rand_seed = atoi(p);
            if (rand_seed <= 0) {
                LOGERR("Invalid arg, specify positive seed in -seed=[val]");
                return 1;
            }
        } else if (char const *technique = argpref(argv[i], "-random-technique=")) {
            if (streq(technique, "uniform"))
                g_rand_technique = e_rt_uniform;
            else if (streq(technique, "clusters"))
                g_rand_technique = e_rt_clusters;
            else {
                LOGERR("Invalid arg, specify one of [uniform|clusters] in -random-technique=[val]");
                return 1;
            }
        } else if (char const *p = argpref(argv[i], "-cluster-count=")) {
            g_cluster_count = atoi(p);
            if (g_cluster_count <= 0) {
                LOGERR("Invalid arg, specify positive cluster_count in -cluster-count=[val]");
                return 1;
            } else if (g_cluster_count > c_max_clusters) {
                LOGERR("Invalid arg, -cluster-count must be <= %u", c_max_clusters);
                return 1;
            }

            specified_cluster_count = true;
        } else {
            LOGERR("Invalid arg: %s", argv[i]);
            return 1;
        }
    }

    if (g_rand_technique == e_rt_uniform && specified_cluster_count) {
        LOGERR("Invalid arg, -cluster-count=[val] is invalid when -random-technique=uniform");
        return 1;
    }

    init_random(rand_seed);

    OUTPUT("{\n");
    OUTPUT("  \"points\": [\n");

    f64 sum = 0.0;

    for (i64 i = 0; i < point_count; ++i) {
        point_pair_t pair = generate_random_point_pair();
        output_point_pair(pair, i == point_count - 1);

        f64 dist = haversine_dist_reference(pair);
        sum += dist;

        if (checksum_f)
            fwrite(&dist, sizeof(dist), 1, checksum_f);
        else
            OUTPUT("    // dist=%.18lf\n", dist);
    }

    f64 avg = sum / point_count;
    if (checksum_f)
        fwrite(&sum, sizeof(sum), 1, checksum_f);
    else
        OUTPUT("  // avg=%.18lf\n", avg);

    OUTPUT("  ]\n");
    OUTPUT("}\n");

    if (checksum_f)
        fclose(checksum_f);
    if (g_outf)
        fclose(g_outf);

    return 0;
}
