#include "memory.hpp"
#include "defs.hpp"
#include "util.hpp"
#include <cstdio>

// @TODO: now I just load to the beginning of the memory, it would be better to change that
u32 load_file_to_memory(memory_access_t dest, const char *fn)
{
    FILE *f = fopen(fn, "rb");
    if (!f) {
        LOGERR("Failed to open file %s for load", fn);
        return 0;
    }

    // @NOTE: if the file is too large, it is just truncated
    u32 bytes_read = fread(dest.mem + dest.base, 1, dest.size, f);
    fclose(f);
    return bytes_read;
}
