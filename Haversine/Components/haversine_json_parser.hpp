#pragma once

#include <buffer.hpp>
#include <defs.hpp>
#include <string.hpp>
#include <profiling.hpp>

enum token_type_t {
    e_tt_eof,        // eof
    e_tt_string,     // string literal
    e_tt_numeric,    // numeric literal
    e_tt_true,       // literal true
    e_tt_false,      // literal false
    e_tt_null,       // literal null
    e_tt_lbrace,     // {
    e_tt_rbrace,     // }
    e_tt_lsqbracket, // [
    e_tt_rsqbracket, // ]
    e_tt_comma,      // ,
    e_tt_colon,      // :

    e_tt_error = -1
};

struct input_file_t {
    buffer_t source;
    u64 pos;

    bool IsEof() const { return pos >= source.len; }
};

struct token_t {
    token_type_t type;
    union {
        string_t str;
        f64 number;
    };

    bool IsFinal() const { return type == e_tt_eof || type == e_tt_error; }
};

inline string_t malloc_string(u64 len)
{
    return {(char *)malloc(len), len};
}

inline void free_string(string_t &s)
{
    if (is_valid(s)) {
        free(s.s);
        s = {};
    }
}

inline void cleanup(token_t &t)
{
    if (t.type == e_tt_string)
        free_string(t.str);
    t = {};
}

inline bool is_eof(int c)
{
    return c == EOF;
}

inline bool is_whitespace(int c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline bool is_digit(int c)
{
    return c >= '0' && c <= '9';
}

inline bool parse_double(string_t repr, f64 &out)
{
    if (!is_valid(repr))
        return false;
    out = 0.0;
    char const *p = repr.s, *start = repr.s, *end = repr.s + repr.len;
    bool minus = false;
    if (p < end && *p == '-') {
        minus = true;
        ++p;
    }
    while (p < end && is_digit(*p)) {
        out = out * 10.0 + f64(*p - '0');
        ++p;
    }
    if (p == start || (minus && p == start + 1))
        return false;
    else if (p == end)
        return true;

    if (p < end && *p == '.') {
        ++p;
        if (p == end || !is_digit(*p))
            return false;
        f64 measure = 0.1;
        while (p < end && is_digit(*p)) {
            out += measure * f64(*p - '0');
            measure *= 0.1;
            ++p;
        }
    }
    if (p < end && (*p == 'e' || *p == 'E')) {
        ++p;
        bool eminus = false;
        if (p < end && *p == '+') {
            ++p;
        } else if (p < end && *p == '-') {
            eminus = true;
            ++p;
        }
        if (p == end || !is_digit(*p))
            return false;
        i32 exp = 0;
        while (p < end && is_digit(*p))
            exp = exp * 10 + i32(*p - '0');
        if (eminus)
            exp = -exp;
        out = pow(out, f64(exp));
    }
    if (minus)
        out = -out;
    return p == end;
}

inline token_t get_next_token(input_file_t &input)
{
    PROFILED_FUNCTION;

    token_t tok = {e_tt_error};
    string_t identifier_view = {};
    bool is_in_string = false;
    bool token_had_string = false; // for "" tokens
    int c;

    for (;;) {
        c = input.IsEof() ? EOF : (char)input.source.data[input.pos++];
    
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
                // If there was an identifier, return sep and yield
                if (is_valid(identifier_view) || token_had_string) {
                    --input.pos;
                    if (token_had_string) {
                        tok.type = e_tt_string;
                        tok.str = malloc_string(identifier_view.len + 1);
                        memcpy(tok.str.s, identifier_view.s, --tok.str.len);
                        tok.str.s[tok.str.len] = '\0';
                    } else if (strncmp(
                        identifier_view.s, "null",
                        identifier_view.len) == 0)
                    {
                        tok.type = e_tt_null;
                    } else if (strncmp(
                        identifier_view.s, "true",
                        identifier_view.len) == 0)
                    {
                        tok.type = e_tt_true;
                    } else if (strncmp(
                        identifier_view.s, "false",
                        identifier_view.len) == 0)
                    {
                        tok.type = e_tt_false;
                    } else {
                        tok.type = e_tt_numeric;
                        if (!parse_double(identifier_view, tok.number))
                            tok.type = e_tt_error;
                    }

                    goto yield;
                }

                // Else, parse separator

                // Try token-type separators
                switch (c) {
                case '{':
                    tok.type = e_tt_lbrace;
                    goto yield;
                case '}':
                    tok.type = e_tt_rbrace;
                    goto yield;
                case '[':
                    tok.type = e_tt_lsqbracket;
                    goto yield;
                case ']':
                    tok.type = e_tt_rsqbracket;
                    goto yield;
                case ',':
                    tok.type = e_tt_comma;
                    goto yield;
                case ':':
                    tok.type = e_tt_colon;
                    goto yield;

                default:
                    break;
                }

                // Otherwise, eol/eof
                if (is_eof(c)) {
                    tok.type = e_tt_eof;
                    goto yield;
                }

                // If none of the above, just whitespace
                continue;
            }
        }

        // If no separator found or screened/in string, 'add char to id'
        assert(c >= SCHAR_MIN && c <= SCHAR_MAX);

        if (is_valid(identifier_view)) {
            ++identifier_view.len;
        } else {
            identifier_view.s = (char *)&input.source.data[input.pos - 1];
            identifier_view.len = 1;
        }
    }

yield:
    return tok;
}

struct json_ent_t;

struct json_field_t {
    string_t name;
    json_ent_t *ent;
};

enum json_ent_type_t {
    e_jt_object,
    e_jt_array,
    e_jt_string,
    e_jt_number,
    e_jt_bool,
    e_jt_null
};

struct json_object_t {
    json_field_t *fields;
    u64 field_cnt, field_cap;
};

struct json_array_t {
    json_ent_t **elements;
    u64 element_cnt, element_cap;
};

using json_string_t = string_t;
using json_number_t = f64;
using json_bool_t = b32;

struct json_null_t {};

struct json_ent_t {
    json_ent_type_t type;
    union {
        json_object_t obj;
        json_array_t arr;
        json_string_t str;
        json_number_t num;
        json_bool_t bl;
        json_null_t null;
    };
};

inline json_ent_t *allocate_json_entity(json_ent_type_t type)
{
    json_ent_t *e = (json_ent_t *)malloc(sizeof(json_ent_t));
    memset(e, 0, sizeof(*e));
    e->type = type;
    return e;
}

inline void push_json_field(json_ent_t &obj, json_field_t f)
{
    assert(obj.type == e_jt_object);
    json_object_t &o = obj.obj;
    if (!o.fields) {
        o.field_cap = 2;
        o.fields = (json_field_t *)malloc(o.field_cap * sizeof(json_field_t));
    } else if (o.field_cnt >= o.field_cap) {
        json_field_t *tmp = o.fields;
        o.field_cap = 2 * o.field_cnt;
        o.fields = (json_field_t *)malloc(o.field_cap * sizeof(json_field_t));
        memcpy(o.fields, tmp, o.field_cnt * sizeof(json_field_t)); 
    }
    o.fields[o.field_cnt++] = f;
}

inline void push_json_element(json_ent_t &arr, json_ent_t *e)
{
    assert(arr.type == e_jt_array);
    assert(e);
    json_array_t &a = arr.arr;
    if (!a.elements) {
        a.element_cap = 2;
        a.elements = (json_ent_t **)malloc(a.element_cap * sizeof(void *));
    } else if (a.element_cnt >= a.element_cap) {
        json_ent_t **tmp = a.elements;
        a.element_cap = 2 * a.element_cnt;
        a.elements = (json_ent_t **)malloc(a.element_cap * sizeof(void *));
        memcpy(a.elements, tmp, a.element_cnt * sizeof(void *)); 
    }
    a.elements[a.element_cnt++] = e;
}

inline json_ent_t *json_object_query(json_ent_t const &obj, char const *name)
{
    assert(obj.type == e_jt_object);
    json_object_t const &o = obj.obj;
    for (u32 i = 0; i < o.field_cnt; ++i) {
        if (streq(name, o.fields[i].name.s))
            return o.fields[i].ent;
    }
    return nullptr;
}

inline void deallocate(json_ent_t *&e);

inline void cleanup(json_field_t &f) {
    free_string(f.name);
    deallocate(f.ent);
    f = {};
}

inline void deallocate(json_ent_t *&e) {
    if (!e)
        return;
    switch (e->type) {
    case e_jt_object:
        for (u32 i = 0; i < e->obj.field_cnt; ++i)
            cleanup(e->obj.fields[i]);
        free(e->obj.fields);
        break;
    case e_jt_array:
        for (u32 i = 0; i < e->arr.element_cnt; ++i)
            deallocate(e->arr.elements[i]);
        free(e->arr.elements);
        break;
    case e_jt_string:
        free_string(e->str);
    default:
        break;
    }
    free(e);
    e = nullptr;
}

#define PARSE_ERR(obj_, fmt_, ...)                                      \
    do {                                                                \
        LOGERR("While parsing object=%p : " fmt_, obj_, ##__VA_ARGS__); \
        deallocate(obj_);                                               \
        return nullptr;                                                 \
    } while (0)

#define GET_TOK(inf_) get_next_token(inf_)

inline json_ent_t *parse_json_entity(input_file_t &inf);
inline json_ent_t *parse_json_entity(input_file_t &inf, token_t &first_token);

inline json_ent_t *parse_json_object(input_file_t &inf)
{
    json_ent_t *obj = allocate_json_entity(e_jt_object);

    for (;;) {
        token_t tok = GET_TOK(inf);
        DEFER([&] { cleanup(tok); });

        if (tok.IsFinal())
            PARSE_ERR(obj, "Unexpected EOF/error");

        if (tok.type == e_tt_rbrace)
            break;

        if (tok.type != e_tt_string)
            PARSE_ERR(obj, "Invalid token, should be a string field name");

        json_field_t field = {tok.str, nullptr};
        tok = GET_TOK(inf);

        if (tok.type != e_tt_colon) {
            cleanup(field);
            PARSE_ERR(obj, "Invalid token, should be a colon");
        }

        field.ent = parse_json_entity(inf);
        if (!field.ent) {
            cleanup(field);
            PARSE_ERR(obj, "Invalid token, should be a json entity");
        }

        push_json_field(*obj, field);

        tok = GET_TOK(inf);
        if (tok.type == e_tt_comma)
            continue;
        else if (tok.type == e_tt_rbrace)
            break;

        PARSE_ERR(obj, "Invalid token");
    }

    return obj;
}

inline json_ent_t *parse_json_array(input_file_t &inf)
{
    json_ent_t *arr = allocate_json_entity(e_jt_array);

    for (;;) {
        token_t tok = GET_TOK(inf);
        DEFER([&] { cleanup(tok); });

        if (tok.IsFinal())
            PARSE_ERR(arr, "Unexpected EOF/error");

        if (tok.type == e_tt_rsqbracket)
            break;

        json_ent_t *elem = parse_json_entity(inf, tok);
        if (!elem)
            PARSE_ERR(arr, "Invalid token, should be a json entity");

        push_json_element(*arr, elem);

        tok = GET_TOK(inf);
        if (tok.type == e_tt_comma)
            continue;
        else if (tok.type == e_tt_rsqbracket)
            break;

        PARSE_ERR(arr, "Invalid token");
    }

    return arr;
}

inline json_ent_t *parse_json_entity(input_file_t &inf, token_t &first_token)
{
    PROFILED_FUNCTION;

    DEFER([&] { cleanup(first_token); });

    switch (first_token.type) {
    case e_tt_lbrace:
        return parse_json_object(inf);
    case e_tt_lsqbracket:
        return parse_json_array(inf);
    case e_tt_null:
        return allocate_json_entity(e_jt_null);
    case e_tt_true: {
        json_ent_t *e = allocate_json_entity(e_jt_bool);
        e->bl = true;
        return e;
    }
    case e_tt_false: {
        json_ent_t *e = allocate_json_entity(e_jt_bool);
        e->bl = false;
        return e;
    }
    case e_tt_numeric: {
        json_ent_t *e = allocate_json_entity(e_jt_number);
        e->num = first_token.number;
        return e;
    }
    case e_tt_string: {
        json_ent_t *e = allocate_json_entity(e_jt_string);
        e->str = xchg(first_token.str, string_t{});
        return e;
    }
    default:
        return nullptr;
    }
}

inline json_ent_t *parse_json_entity(input_file_t &inf)
{
    token_t tok = GET_TOK(inf);
    return parse_json_entity(inf, tok);
}

inline json_ent_t *parse_json_input(input_file_t &inf)
{
    PROFILED_FUNCTION_PF;

    json_ent_t *root = nullptr;
    token_t first_token = GET_TOK(inf);
    DEFER([&] { cleanup(first_token); });

    if (first_token.type == e_tt_eof) {
        LOGERR("Provide a non-emty json!");
        return nullptr;
    }

    if (!first_token.IsFinal()) {
        if (first_token.type != e_tt_lbrace) {
            LOGERR("Invalid json format: must be a root object");
            return nullptr;
        }

        root = parse_json_object(inf);
    }

    if (root == nullptr)
        LOGERR("Json parsing error");

    return root;
}

#define OUTPUT(fmt_, ...) printf(fmt_, ##__VA_ARGS__)

inline void print_json(json_ent_t *ent, int depth, bool indent, bool put_comma)
{
    PROFILED_FUNCTION;

    auto output_indent = [](int depth) {
        for (int i = 0; i < depth; ++i)
            OUTPUT("    ");
    };
    if (indent)
        output_indent(depth);
    switch (ent->type) {
    case e_jt_null:
        OUTPUT("null");
        break;
    case e_jt_bool:
        OUTPUT("%s", ent->bl ? "true" : "false");
        break;
    case e_jt_number:
        OUTPUT("%lf", ent->num);
        break;
    case e_jt_string:
        OUTPUT("%.*s", int(ent->str.len), ent->str.s);
        break;
    case e_jt_array:
        OUTPUT("[\n");
        for (uint32_t i = 0; i < ent->arr.element_cnt; ++i)
            print_json(ent->arr.elements[i], depth + 1, true, true);
        output_indent(depth);
        OUTPUT("]");
        break;
    case e_jt_object:
        OUTPUT("{\n");
        for (uint32_t i = 0; i < ent->obj.field_cnt; ++i) {
            json_field_t const &f = ent->obj.fields[i];
            output_indent(depth + 1);
            OUTPUT("\"%.*s\": ", int(f.name.len), f.name.s);
            print_json(f.ent, depth + 1, false, true);
        }
        output_indent(depth);
        OUTPUT("}");
    }
    if (put_comma)
        OUTPUT(",\n");
    else
        OUTPUT("\n");
}

inline int tokenize_and_print(input_file_t &inf)
{
    PROFILED_FUNCTION;

    token_t tok;
    while (!(tok = GET_TOK(inf)).IsFinal()) {
        switch (tok.type) {
            case e_tt_string:
                OUTPUT("str(%.*s)", int(tok.str.len), tok.str.s);
                break;
            case e_tt_numeric:
                OUTPUT("num(%lf)", tok.number);
                break;
            case e_tt_lbrace:
                OUTPUT("({)");
                break;
            case e_tt_rbrace:
                OUTPUT("(})");
                break;
            case e_tt_lsqbracket:
                OUTPUT("([)");
                break;
            case e_tt_rsqbracket:
                OUTPUT("(])");
                break;
            case e_tt_comma:
                OUTPUT("(,)");
                break;
            case e_tt_colon:
                OUTPUT("(:)");
                break;
        
            default:
                break;
        }

        cleanup(tok);

        OUTPUT("\n");
    }

    if (tok.type == e_tt_error) {
        LOGERR("Tokenization test failed!");
        return 2;
    }

    return 0;
}

inline int reprint_json(json_ent_t *root)
{
    PROFILED_FUNCTION;

    print_json(root, 0, true, false);
    return 0;
}
