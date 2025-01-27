#include "string.hpp"
#include "array.hpp"
#include "util.hpp"
#include "defer.hpp"

#include "profiling.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cmath>
#include <climits>
#include <cstring>

#define LOGDBG(fmt_, ...) fprintf(stderr, "[DBG] " fmt_ "\n", ##__VA_ARGS__)
#define LOGERR(fmt_, ...) fprintf(stderr, "[ERR] " fmt_ "\n", ##__VA_ARGS__)

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
    long double number;
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
    PROFILED_FUNCTION;

    token_t tok = {tt_error};
    DynArray<char> id{};
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
                        tok.str = HpString{id.Begin(), id.Length()};
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
        id.Append(c);
    }

yield:
    return tok;
}

static FILE *g_inf = stdin;
#define GET_TOK() get_next_token(g_inf)

#define OUTPUT(fmt_, ...) printf(fmt_, ##__VA_ARGS__)

struct json_field_t;

struct IJsonEntity {
    virtual ~IJsonEntity() {}

    virtual bool IsObject() const = 0;
    virtual bool IsArray() const = 0;
    virtual bool IsString() const = 0;
    virtual bool IsNumber() const = 0;
    virtual bool IsBool() const = 0;
    virtual bool IsNull() const = 0;

    virtual IJsonEntity *ObjectQuery(const char *key) const = 0;
    virtual const json_field_t &FieldAt(uint32_t id) const = 0;
    virtual IJsonEntity *ArrayAt(uint32_t id) const = 0;
    virtual const HpString &Str() const = 0;
    virtual long double Number() const = 0;
    virtual bool Bool() const = 0;
    virtual uint32_t ElemCnt() const = 0;
};

#define IS_STUB(name_) bool name_() const override { return false; }

struct json_field_t {
    HpString name;
    IJsonEntity *val;
};

class JsonObject : public IJsonEntity {
    DynArray<json_field_t> m_fields;

public:
    JsonObject() = default;
    ~JsonObject() override {
        FOR(m_fields) delete it->val;
    }

    void AddField(json_field_t &&field) { m_fields.Append(mv(field)); }

    bool IsObject() const override { return true; }
    IS_STUB(IsArray)
    IS_STUB(IsString)
    IS_STUB(IsNumber)
    IS_STUB(IsBool)
    IS_STUB(IsNull)

    IJsonEntity *ObjectQuery(const char *key) const override {
        FOR(m_fields) {
            if (streq(key, it->name.CStr()))
                return it->val;
        }
        return nullptr;
    }
    const json_field_t &FieldAt(uint32_t id) const override { return m_fields[id]; }
    uint32_t ElemCnt() const override { return m_fields.Length(); }
    IJsonEntity *ArrayAt(uint32_t id) const override { assert(0); return {}; }
    const HpString &Str() const override { assert(0); return {}; }
    long double Number() const override { assert(0); return {}; }
    bool Bool() const override { assert(0); return {}; }
};

class JsonArray : public IJsonEntity {
    DynArray<IJsonEntity *> m_elements;

public:
    JsonArray() = default;
    ~JsonArray() override {
        FOR(m_elements) delete *it;
    }

    void Add(IJsonEntity *elem) { m_elements.Append(elem); }

    bool IsArray() const override { return true; }
    IS_STUB(IsObject)
    IS_STUB(IsString)
    IS_STUB(IsNumber)
    IS_STUB(IsBool)
    IS_STUB(IsNull)

    IJsonEntity *ArrayAt(uint32_t id) const override { return m_elements[id]; }
    uint32_t ElemCnt() const override { return m_elements.Length(); }
    IJsonEntity *ObjectQuery(const char *key) const override { assert(0); return {}; }
    const json_field_t &FieldAt(uint32_t id) const override { assert(0); return {}; }
    const HpString &Str() const override { assert(0); return {}; }
    long double Number() const override { assert(0); return {}; }
    bool Bool() const override { assert(0); return {}; }
};

class JsonString : public IJsonEntity {
    HpString m_content;

public:
    JsonString(HpString &&str) : m_content{mv(str)} {}

    bool IsString() const override { return true; }
    IS_STUB(IsObject)
    IS_STUB(IsArray)
    IS_STUB(IsNumber)
    IS_STUB(IsBool)
    IS_STUB(IsNull)

    const HpString &Str() const override { return m_content; }
    IJsonEntity *ObjectQuery(const char *key) const override { assert(0); return {}; }
    const json_field_t &FieldAt(uint32_t id) const override { assert(0); return {}; }
    IJsonEntity *ArrayAt(uint32_t id) const override { assert(0); return {}; }
    long double Number() const override { assert(0); return {}; }
    bool Bool() const override { assert(0); return {}; }
    uint32_t ElemCnt() const override { assert(0); return {}; }
};

class JsonNumber : public IJsonEntity {
    long double m_val;

public:
    JsonNumber(long double val) : m_val(val) {}

    bool IsNumber() const override { return true; }
    IS_STUB(IsObject)
    IS_STUB(IsArray)
    IS_STUB(IsString)
    IS_STUB(IsBool)
    IS_STUB(IsNull)

    long double Number() const override { return m_val; }
    IJsonEntity *ObjectQuery(const char *key) const override { assert(0); return {}; }
    const json_field_t &FieldAt(uint32_t id) const override { assert(0); return {}; }
    IJsonEntity *ArrayAt(uint32_t id) const override { assert(0); return {}; }
    const HpString &Str() const override { assert(0); return {}; }
    bool Bool() const override { assert(0); return {}; }
    uint32_t ElemCnt() const override { assert(0); return {}; }
};

class JsonBool : public IJsonEntity {
    bool m_val;

public:
    JsonBool(bool val) : m_val(val) {}

    bool IsBool() const override { return true; }
    IS_STUB(IsObject)
    IS_STUB(IsArray)
    IS_STUB(IsString)
    IS_STUB(IsNumber)
    IS_STUB(IsNull)

    bool Bool() const override { return m_val; }
    IJsonEntity *ObjectQuery(const char *key) const override { assert(0); return {}; }
    const json_field_t &FieldAt(uint32_t id) const override { assert(0); return {}; }
    IJsonEntity *ArrayAt(uint32_t id) const override { assert(0); return {}; }
    const HpString &Str() const override { assert(0); return {}; }
    long double Number() const override { assert(0); return {}; }
    uint32_t ElemCnt() const override { assert(0); return {}; }
};

class JsonNull : public IJsonEntity {
public:
    bool IsNull() const override { return true; }
    IS_STUB(IsObject)
    IS_STUB(IsArray)
    IS_STUB(IsString)
    IS_STUB(IsNumber)
    IS_STUB(IsBool)

    IJsonEntity *ObjectQuery(const char *key) const override { assert(0); return {}; }
    const json_field_t &FieldAt(uint32_t id) const override { assert(0); return {}; }
    IJsonEntity *ArrayAt(uint32_t id) const override { assert(0); return {}; }
    const HpString &Str() const override { assert(0); return {}; }
    long double Number() const override { assert(0); return {}; }
    bool Bool() const override { assert(0); return {}; }
    uint32_t ElemCnt() const override { assert(0); return {}; }
};

// @TODO: more info on parse errors

#define PARSE_ERR(obj_, fmt_, ...)                                      \
    do {                                                                \
        delete obj_;                                                    \
        LOGERR("While parsing object=%p : " fmt_, obj_, ##__VA_ARGS__); \
        return nullptr;                                                 \
    } while (0)

static IJsonEntity *parse_json_entity();
static IJsonEntity *parse_json_entity(const token_t &first_token);

static JsonObject *parse_json_object()
{
    PROFILED_FUNCTION;

    JsonObject *obj = new JsonObject{};

    for (;;) {
        token_t tok = GET_TOK();
        if (tok.IsFinal())
            PARSE_ERR(obj, "Unexpected EOF/error");

        if (tok.type == tt_rbrace)
            break;

        if (tok.type != tt_string)
            PARSE_ERR(obj, "Invalid token, should be a string field name");

        json_field_t field = {mv(tok.str), nullptr};

        tok = GET_TOK();
        if (tok.type != tt_colon)
            PARSE_ERR(obj, "Invalid token, should be a colon");

        field.val = parse_json_entity();
        if (!field.val)
            PARSE_ERR(obj, "Invalid token, should be a json entity");

        obj->AddField(mv(field));

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
    PROFILED_FUNCTION;

    JsonArray *arr = new JsonArray{};

    for (;;) {
        token_t tok = GET_TOK();
        if (tok.IsFinal())
            PARSE_ERR(arr, "Unexpected EOF/error");

        if (tok.type == tt_rsqbracket)
            break;

        IJsonEntity *elem = parse_json_entity(tok);
        if (!elem)
            PARSE_ERR(arr, "Invalid token, should be a json entity");

        arr->Add(elem);

        token_t ntok = GET_TOK();
        if (ntok.type == tt_comma)
            continue;
        else if (ntok.type == tt_rsqbracket)
            break;

        PARSE_ERR(arr, "Invalid token");
    }

    return arr;
}

static IJsonEntity *parse_json_entity(const token_t &first_token)
{
    PROFILED_FUNCTION;

    switch (first_token.type) {
    case tt_lbrace:
        return parse_json_object();
    case tt_lsqbracket:
        return parse_json_array();
    case tt_null:
        return new JsonNull;
    case tt_true:
        return new JsonBool{true};
    case tt_false:
        return new JsonBool{false};
    case tt_numeric:
        return new JsonNumber{first_token.number};
    case tt_string:
        return new JsonString{HpString::Copy(first_token.str)};

    default:
        return nullptr;
    }
}

static IJsonEntity *parse_json_entity()
{
    token_t tok = GET_TOK();
    return parse_json_entity(tok);
}

static JsonObject *parse_json_input()
{
    PROFILED_FUNCTION;

    JsonObject *root = nullptr;
    token_t first_token = GET_TOK();
    if (first_token.type == tt_eof) {
        LOGERR("Provide a non-emty json!");
        return nullptr;
    }

    if (!first_token.IsFinal()) {
        if (first_token.type != tt_lbrace) {
            LOGERR("Invalid json format: must be a root object");
            return nullptr;
        }

        root = parse_json_object();
    }

    if (root == nullptr)
        LOGERR("Json parsing error");

    return root;
}

static void print_json(IJsonEntity *ent, int depth, bool indent, bool put_comma)
{
    PROFILED_FUNCTION;

    auto output_indent = [](int depth) {
        for (int i = 0; i < depth; ++i)
            OUTPUT("    ");
    };
    if (indent)
        output_indent(depth);
    if (ent->IsNull())
        OUTPUT("null");
    else if (ent->IsBool())
        OUTPUT("%s", ent->Bool() ? "true" : "false");
    else if (ent->IsNumber())
        OUTPUT("%Lf", ent->Number());
    else if (ent->IsString())
        OUTPUT("%.*s", ent->Str().Length(), ent->Str().CStr());
    else if (ent->IsArray()) {
        OUTPUT("[\n");
        for (uint32_t i = 0; i < ent->ElemCnt(); ++i)
            print_json(ent->ArrayAt(i), depth + 1, true, true);
        output_indent(depth);
        OUTPUT("]");
    } else if (ent->IsObject()) {
        OUTPUT("{\n");
        for (uint32_t i = 0; i < ent->ElemCnt(); ++i) {
            const json_field_t &f = ent->FieldAt(i);
            output_indent(depth + 1);
            OUTPUT("\"%.*s\": ", f.name.Length(), f.name.CStr());
            print_json(f.val, depth + 1, false, true);
        }
        output_indent(depth);
        OUTPUT("}");
    }
    if (put_comma)
        OUTPUT(",\n");
    else
        OUTPUT("\n");
}

static int tokenize_and_print_main()
{
    PROFILED_FUNCTION;

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

    return 0;
}

static int reprint_json_main(JsonObject *root)
{
    PROFILED_FUNCTION;

    print_json(root, 0, true, false);
    return 0;
}

static float haversine_dist(float x0, float y0, float x1, float y1)
{
    PROFILED_FUNCTION;

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
    init_profiler();
    DEFER([] { finish_profiling_and_dump_stats(printf); });

    PROFILED_BLOCK("Program");

    bool only_tokenize = false;
    bool only_reprint_json = false;
    const char *json_fname = nullptr;
    FILE *checksum_f = nullptr;

    {
        PROFILED_BLOCK("Argument parsing");

        for (int i = 1; i < argc; ++i) {
            if (streq(argv[i], "-f")) {
                ++i;
                if (i >= argc) {
                    LOGERR("Invalid arg, specify file name after -f");
                    return 1;
                }

                json_fname = argv[i];

                g_inf = fopen(json_fname, "r");
                if (!g_inf) {
                    LOGERR("Failed to open %s to read, error: %s",
                           json_fname, strerror(errno));
                    return 1;
                }
            } else if (streq(argv[i], "-tokenize")) {
                if (only_reprint_json) {
                    LOGERR("Invalid usage: -tokenize and -print are incompatible");
                    return 1;
                }
                only_tokenize = true;
            } else if (streq(argv[i], "-reprint")) {
                if (only_tokenize) {
                    LOGERR("Invalid usage: -tokenize and -print are incompatible");
                    return 1;
                }
                only_reprint_json = true;
            } else {
                LOGERR("Invalid arg: %s", argv[i]);
                return 1;
            }
        }
    }

    if (only_tokenize)
        return tokenize_and_print_main();

    JsonObject *root = parse_json_input();
    if (!root)
        return 2;

    DEFER([root] { delete root; });

    if (only_reprint_json)
        return reprint_json_main(root);

    {
        PROFILED_BLOCK("Misc preparation");

        if (root->ElemCnt() != 1 ||
            !streq(root->FieldAt(0).name.CStr(), "points") ||
            !root->FieldAt(0).val->IsArray())
        {
            LOGERR("Invalid format: correct is { \"points\": [ ... ] }");
            return 2;
        }

        if (json_fname) {
            char checksum_fname[256];
            snprintf(checksum_fname, sizeof(checksum_fname), "%s.check.bin", json_fname);
            checksum_f = fopen(checksum_fname, "rb");
        }
    }

    float sum;
    float avg;
    uint32_t point_cnt;

    {
        PROFILED_BLOCK("Haversine claculation");

        DynArray<float> distances{};

        IJsonEntity *points = root->FieldAt(0).val;
        point_cnt = points->ElemCnt();

        for (uint32_t i = 0; i < point_cnt; ++i) {
            IJsonEntity *elem = points->ArrayAt(i);
            auto print_point_format_error = [] {
                LOGERR("Invalid point format: correct is "
                       "{ \"x0\": .f, \"y0\": .f, \"x1\": .f, \"y1\": .f }");
            };
            auto read_float = [elem](const char *name, float &out) {
                if (IJsonEntity *d = elem->ObjectQuery(name)) {
                    if (d->IsNumber()) {
                        out = (float)d->Number();
                        return true;
                    }
                }
                return false;
            };

            if (!elem->IsObject() || elem->ElemCnt() != 4) {
                print_point_format_error();
                return 2;
            }
            float x0, y0, x1, y1;
            if (!read_float("x0", x0) ||
                !read_float("y0", y0) ||
                !read_float("x1", x1) ||
                !read_float("y1", y1))
            {
                print_point_format_error();
                return 2;
            }

            const float dist = haversine_dist(x0, y0, x1, y1);

            if (checksum_f) {
                float valid;
                fread(&valid, sizeof(valid), 1, checksum_f);
                if (dist != valid) {
                    LOGERR("Validation failed: dist #%u mismatch, claculated %f, required %f",
                           i, dist, valid);
                    return 2;
                }
            }

            distances.Append(dist);
        }

        sum = 0.f;
        FOR(distances) sum += *it;

        avg = sum / point_cnt;
    }

    {
        PROFILED_BLOCK("Output, validation and shutdown");

        OUTPUT("Point count: %u\n", point_cnt);
        OUTPUT("Avg dist: %f\n\n", avg);

        if (checksum_f) {
            float valid;
            fread(&valid, sizeof(valid), 1, checksum_f);
            if (avg != valid) {
                LOGERR("Validation failed: avg dist mismatch, claculated %f, required %f",
                       avg, valid);
                return 2;
            } else
                OUTPUT("Validation: %f\n", valid);
        } else
            OUTPUT("Validation file not found\n");

        if (checksum_f)
            fclose(checksum_f);
        if (g_inf)
            fclose(g_inf);

        OUTPUT("\nTODO: Make it not dogshit slow\n");
        OUTPUT("TODO: Sort out and save the utils\n\n");
    }

    return 0;
}

static_assert(__COUNTER__ < c_profiler_slots_count, "Too many profiler slots declared");
