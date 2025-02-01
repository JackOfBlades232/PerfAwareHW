#pragma once

#include "util.hpp"

#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <cassert>
#include <cstdint>

// @TODO: pull out to some utils, and complete features when needed
// Idea: HpString, StString, DynString, TmpString, etc

// @TODO: work out semantics, test

class HpString {
    char *m_data = nullptr;
    uint32_t m_len = 0;

public:
    HpString() = default;
    HpString(const char *ptr, uint32_t len) : m_len{len} {
        assert(ptr);

        m_data = new char[m_len + 1];
        memcpy(m_data, ptr, m_len);
        m_data[m_len] = '\0';
    }
    explicit HpString(const char *ptr) : HpString{ptr, (uint32_t)strlen(ptr)} {}

    HpString(const HpString &other) = delete;
    HpString& operator=(const HpString &other) = delete;

    HpString(HpString &&other)
        : m_data{xchg(other.m_data, nullptr)}
        , m_len{xchg(other.m_len, 0)}
    {}
    HpString& operator=(HpString &&other) {
        HpString tmp{mv(other)};
        swp(m_data, tmp.m_data);
        swp(m_len, tmp.m_len);
        return *this;
    }

    ~HpString() {
        if (m_data)
            delete[] m_data;
    }

    uint32_t Length() const { return m_len; }
    const char *Begin() const { return m_data; }
    const char *End() const { return m_data + m_len; }

    const char *CStr() const { return Begin(); }

    bool Empty() const { return Length() == 0; }

    void Clear() {
        if (m_data)
            delete[] m_data;
        m_data = nullptr;
        m_len = 0;
    }

    static HpString Copy(const HpString &other) {
        HpString str;
        str.m_len = other.m_len;

        char *old_data = str.m_data;
        if (other.m_data) {
            str.m_data = new char[str.m_len + 1];
            memcpy(str.m_data, other.m_data, str.m_len);
            str.m_data[str.m_len] = '\0';
        } else
            str.m_data = nullptr;

        if (old_data)
            delete[] old_data;

        return str;
    }
};
