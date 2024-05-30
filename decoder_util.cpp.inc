inline uint16_t u16_swap_bytes(uint16_t n)
{
    return (n >> 8) | ((n & 0xFF) << 8);
}

template <class T>
inline bool fetch_mem(FILE *f, T *out)
{
    return fread(out, sizeof(*out), 1, f) == 1;
}

template <class T>
bool fetch_integer_with_endian_swap(FILE *f, T *out)
{
    static_assert(sizeof(*out) <= 2, "Endian swap fetch only for 8 and 16 bit integers");

    if (fread(out, sizeof(*out), 1, f) != 1)
        return false;
    if (sizeof(*out) > 1 && g_machine_is_big_endian)
        *out = u16_swap_bytes(*out);
    return true;
}

#define TRY_ELSE_RETURN(_try, _retval) \
    do {                               \
        if (!(_try))                   \
            return _retval;            \
    } while (0)

#define NOT_IMPL() fprintf(stderr, "Not implemented\n")

#define ARR_SIZE(_arr) (sizeof(_arr) / sizeof((_arr)[0]))
