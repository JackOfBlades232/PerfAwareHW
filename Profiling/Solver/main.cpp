#include "string.hpp"
#include "array.hpp"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cmath>

#define LOGERR(fmt_, ...) fprintf(stderr, "[ERR] " fmt_ "\n", ##__VA_ARGS__)

#define STRERROR_BUF(bufname_, err_) \
    char bufname_[256];              \
    strerror_s(bufname_, 256, err_)

static bool streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

enum token_type_t {
    tt_eof,        // eof
    tt_string,     // string literal
    tt_numeric,    // numeric literal
    tt_lbrace,     // {
    tt_rbrace,     // }
    tt_lsqbracket, // [
    tt_rsqbracket, // ]
    tt_comma,      // ,
    tt_colon,      // :

    tt_error = -1
};

struct token_t {
    token_type_t type;
    long double number; // @TODO: union for different types?
    HpString str;

    bool IsFinal() const { return type == tt_eof || type == tt_error; }
};

static int is_eof(int c)
{
    return c == EOF;
}

static int is_whitespace(int c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static token_t get_next_token(FILE *f)
{
    token_t tok = { tt_error };
    DynArray<char> id;
    bool is_in_string = false;
    bool token_had_string = false; // for "" tokens
    int c;

    // @FEAT: add screened characters and hex/oct/bin literals
    // @FEAT: place error text in str for error

    for (;;) {
        c = fgetc(f);
    
        if (is_in_string && is_eof(c))
            goto yield; // tok type is error

        if (c == '"') {
            is_in_string = !is_in_string;
            // Only the first assignment is needed
            token_had_string = true;
            continue;
        }

        if (!is_in_string) {
            // Separator found
            if (is_eof(c) || is_whitespace(c) || strchr("{}[],:", c)) {
                // If there was an identifier, return sep to stream and yield
                if (!id.Empty() || token_had_string) {
                    ungetc(c, f);
                    tok.type = token_had_string ? tt_string : tt_numeric;

                    if (tok.type == tt_numeric) {
                        HpString literal(id.Begin(), id.Length());
                        char *end = nullptr;
                        tok.number = strtold(literal.CStr(), &end);
                        if (end != literal.End() ||
                            ((tok.number == HUGE_VALL || tok.number == -HUGE_VALL) && errno == ERANGE) ||
                            (tok.number == 0.0L && errno == EINVAL))
                        {
                            tok.type = tt_error;
                        }
                    } else {
                        tok.str = HpString(id.Begin(), id.Length());
                    }

                    goto yield;
                }

                // Else, parse separator

                // Try token-type separators
                switch (c) {
                case '{':
                    tok.type = tt_lbrace;
                    goto yield;
                case '}':
                    tok.type = tt_rbrace;
                    goto yield;
                case '[':
                    tok.type = tt_lsqbracket;
                    goto yield;
                case ']':
                    tok.type = tt_rsqbracket;
                    goto yield;
                case ',':
                    tok.type = tt_comma;
                    goto yield;
                case ':':
                    tok.type = tt_colon;
                    goto yield;

                default:
                    break;
                }

                // Otherwise, eol/eof
                if (is_eof(c)) {
                    tok.type = tt_eof;
                    goto yield;
                }

                // If none of the above, just whitespace
                continue;
            }
        }

        // If no separator found or screened/in string, add char to id
        assert(c >= SCHAR_MIN && c <= SCHAR_MAX);
        id.Add(c);
    }

yield:
    return tok;
}

struct IJsonElement {
    // @TODO
};

struct json_field_t {
    HpString name;
    IJsonElement *val;
};

class JsonObject : IJsonElement {
    DynArray<json_field_t> fields;

    // @TODO
};

class JsonArray : IJsonElement {
    DynArray<IJsonElement *> m_elements;

    // @TODO
};

class JsonString : IJsonElement {
    HpString m_content;

    // @TODO
};

class JsonNumber : IJsonElement {
    long double m_val;

    // @TODO
};

class JsonBool : IJsonElement {
    bool m_val;

    // @TODO
};

class JsonNull : IJsonElement {
    // @TODO
};


// @TODO: verify
static float reference_haversine_dist(float x0, float y0, float x1, float y1)
{
    constexpr float c_pi = 3.14159265359f;

    auto deg2rad = [](float deg) { return deg * c_pi / 180.f; };
    auto rad2deg = [](float rad) { return rad * 180.f / c_pi; };

    auto haversine = [](float rad) { return (1.f - cosf(rad)) * 0.5f; };

    float dx = deg2rad(fabsf(x0 - x1));
    float dy = deg2rad(fabsf(y0 - y1));
    float y0r = deg2rad(y0);
    float y1r = deg2rad(y1);

    float hav_of_diff =
        haversine(dy) + cosf(y0r)*cosf(y1r)*haversine(dx);

    float rad_of_diff = acosf(1.f - 2.f*hav_of_diff);
    return rad2deg(rad_of_diff);
}

static FILE *g_inf = stdin;
#define GET_TOK() get_next_token(g_inf)

#define OUTPUT(fmt_, ...) printf(fmt_, ##__VA_ARGS__)

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (streq(argv[i], "-f")) {
            ++i;
            if (i >= argc) {
                LOGERR("Invalid arg, specify file name after -f");
                return 1;
            }

            errno_t err_code;
            if ((err_code = fopen_s(&g_inf, argv[i], "r")) != 0) {
                STRERROR_BUF(err_text, err_code);
                LOGERR("Failed to open %s to read, error: %s",
                       argv[i], err_text);
                exit(1);
            }
        }
    }

    printf("TEST: Tokenization\n");
    token_t tok;
    while (!(tok = GET_TOK()).IsFinal()) {
        switch (tok.type) {
            case tt_string:
                OUTPUT("str(%s)", tok.str.CStr());
                break;
            case tt_numeric:
                OUTPUT("num(%Lf)", tok.number);
                break;
            case tt_lbrace:
                OUTPUT("({)");
                break;
            case tt_rbrace:
                OUTPUT("(})");
                break;
            case tt_lsqbracket:
                OUTPUT("([)");
                break;
            case tt_rsqbracket:
                OUTPUT("(])");
                break;
            case tt_comma:
                OUTPUT("(,)");
                break;
            case tt_colon:
                OUTPUT("(:)");
                break;
        
            default:
                break;
        }

        OUTPUT("\n");
    }

    if (tok.type == tt_error) {
        LOGERR("Tokenization test failed!");
        return 2;
    }

    OUTPUT("TODO: parse input json!\n");
    OUTPUT("TODO: Calc haversine!\n");
    OUTPUT("TODO: Make it not dogshit slow!\n");
    OUTPUT("TODO: Brush up and save the utils!\n");
    return 0;
}
