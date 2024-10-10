#include "string.hpp"
#include "array.hpp"
#include "defer.hpp"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cmath>

// @TODO brush up

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
    tt_true,       // literal true
    tt_false,      // literal false
    tt_null,       // literal null
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
                    if (token_had_string) {
                        tok.type = tt_string;
                        tok.str = HpString(id.Begin(), id.Length());
                    } else if (strncmp(id.Begin(), "null", 4) == 0)
                        tok.type = tt_null;
                    else if (strncmp(id.Begin(), "true", 4) == 0)
                        tok.type = tt_true;
                    else if (strncmp(id.Begin(), "false", 5) == 0)
                        tok.type = tt_false;
                    else {
                        tok.type = tt_numeric;
                        HpString literal(id.Begin(), id.Length());
                        char *end = nullptr;
                        tok.number = strtold(literal.CStr(), &end);
                        if (end != literal.End() ||
                            ((tok.number == HUGE_VALL || tok.number == -HUGE_VALL) && errno == ERANGE) ||
                            (tok.number == 0.0L && errno == EINVAL))
                        {
                            tok.type = tt_error;
                        }
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

static FILE *g_inf = stdin;
#define GET_TOK() get_next_token(g_inf)

#define OUTPUT(fmt_, ...) printf(fmt_, ##__VA_ARGS__)

static void output_indent(int depth)
{
    for (int i = 0; i < depth; ++i)
        OUTPUT("  ");
}

#define OUTPUT_IND(depth_, fmt_, ...) \
    do {                              \
        output_indent(depth_);        \
        OUTPUT(fmt_, ##__VA_ARGS__);  \
    } while (0)

struct IJsonEntity {
    virtual ~IJsonEntity() {}
    virtual void PrettyPrint(int depth) const = 0;
};

struct json_field_t {
    HpString name;
    IJsonEntity *val;
};

class JsonObject : public IJsonEntity {
    DynArray<json_field_t> m_fields;

public:
    JsonObject() = default;
    ~JsonObject() override {
        FOR (m_fields)
            delete it->val;
    }

    void AddField(const json_field_t &field) { m_fields.Add(field); }

    void PrettyPrint(int depth) const override {
        OUTPUT_IND(depth, "{\n");
        FOR (m_fields) {
            OUTPUT_IND(depth + 1, "\"%s\":\n", it->name.CStr());
            it->val->PrettyPrint(depth + 1);
        }
        OUTPUT_IND(depth, "},\n");
    }
};

class JsonArray : public IJsonEntity {
    DynArray<IJsonEntity *> m_elements;

public:
    JsonArray() = default;
    ~JsonArray() override {
        FOR (m_elements)
            delete *it;
    }

    void Add(IJsonEntity *elem) { m_elements.Add(elem); }

    void PrettyPrint(int depth) const override {
        OUTPUT_IND(depth, "[\n");
        FOR (m_elements)
            (*it)->PrettyPrint(depth + 1);
        OUTPUT_IND(depth, "],\n");
    }
};

class JsonString : public IJsonEntity {
    HpString m_content;

public:
    JsonString(const HpString &str) : m_content(str) {}

    void PrettyPrint(int depth) const override {
        OUTPUT_IND(depth, "\"%s\",\n", m_content.CStr());
    }
};

class JsonNumber : public IJsonEntity {
    long double m_val;

public:
    JsonNumber(long double val) : m_val(val) {}
    operator long double() const { return m_val; }

    void PrettyPrint(int depth) const override { OUTPUT_IND(depth, "%Lf,\n", m_val); }
};

class JsonBool : public IJsonEntity {
    bool m_val;

public:
    JsonBool(bool val) : m_val(val) {}
    operator bool() const { return m_val; }

    void PrettyPrint(int depth) const override {
        OUTPUT_IND(depth, "%s,\n", m_val ? "true" : "false");
    }
};

class JsonNull : public IJsonEntity {
public:
    void PrettyPrint(int depth) const override { OUTPUT_IND(depth, "null,\n"); }
};

#define PARSE_ERR(obj_, fmt_, ...)                                      \
    do {                                                                \
        delete obj_;                                                    \
        LOGERR("While parsing object=%p : " fmt_, obj_, ##__VA_ARGS__); \
        return nullptr;                                                 \
    } while (0)

static IJsonEntity *parse_json_entity(const token_t *first_token = nullptr);

// @TODO: logerr messages & logic
// @TODO: refac

static JsonObject *parse_json_object()
{
    JsonObject *obj = new JsonObject;

    for (;;) {
        token_t tok = GET_TOK();
        if (tok.IsFinal())
            PARSE_ERR(obj, "Unexpected EOF/error");

        if (tok.type == tt_rbrace)
            break;

        if (tok.type != tt_string)
            PARSE_ERR(obj, "Invalid token");

        json_field_t field = {tok.str, nullptr};

        tok = GET_TOK();
        if (tok.type != tt_colon)
            PARSE_ERR(obj, "Invalid token");

        field.val = parse_json_entity();
        if (!field.val)
            PARSE_ERR(obj, "Invalid token");

        obj->AddField(field);

        tok = GET_TOK();
        if (tok.type == tt_comma)
            continue;
        else if (tok.type == tt_rbrace)
            break;

        PARSE_ERR(obj, "Invalid token");
    }

    return obj;
}

static JsonArray *parse_json_array()
{
    JsonArray *arr = new JsonArray;

    for (;;) {
        token_t tok = GET_TOK();
        if (tok.IsFinal())
            PARSE_ERR(arr, "Unexpected EOF/error");

        if (tok.type == tt_rsqbracket)
            break;

        IJsonEntity *elem = parse_json_entity(&tok);
        if (!elem)
            PARSE_ERR(arr, "Invalid token");

        arr->Add(elem);

        tok = GET_TOK();
        if (tok.type == tt_comma)
            continue;
        else if (tok.type == tt_rsqbracket)
            break;

        PARSE_ERR(arr, "Invalid token");
    }

    return arr;
}

static IJsonEntity *parse_json_entity(const token_t *first_token)
{
    token_t tok = first_token ? *first_token : GET_TOK();
    switch (tok.type) {
    case tt_lbrace:
        return parse_json_object();
    case tt_lsqbracket:
        return parse_json_array();
    case tt_null:
        return new JsonNull;
    case tt_true:
        return new JsonBool(true);
    case tt_false:
        return new JsonBool(false);
    case tt_numeric:
        return new JsonNumber(tok.number);
    case tt_string:
        return new JsonString(tok.str);

    default:
        return nullptr;
    }
}

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

    /*
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
    */

    printf("TEST: Parsing\n");

    JsonObject *root = nullptr;
    token_t first_token = GET_TOK();
    if (first_token.type == tt_eof) {
        LOGERR("Provide a non-emty json!");
        return 2;
    }

    if (!first_token.IsFinal()) {
        if (first_token.type != tt_lbrace) {
            LOGERR("Invalid json format: must be a root object");
            return 2;
        }

        root = parse_json_object();
    }

    if (root == nullptr) {
        LOGERR("Json parsing error");
        return 2;
    }

    root->PrettyPrint(0);
    delete root;

    OUTPUT("TODO: Calc haversine!\n");
    OUTPUT("TODO: Make it not dogshit slow!\n");
    OUTPUT("TODO: Brush up and save the utils!\n");
    return 0;
}
