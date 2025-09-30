#pragma once

#include <buffer.hpp>
#include <array.hpp>
#include <string.hpp>

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

struct input_file_t {
    buffer_t source;
    u64 pos;

    bool IsEof() const { return pos >= source.len; }
};

struct token_t {
    token_type_t type;
    f64 number;
    HeapString str;

    bool IsFinal() const { return type == tt_eof || type == tt_error; }
};
