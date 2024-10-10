#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <cassert>
#include <cstdint>

// @TODO: pull out to some utils, and complete features when needed
// Idea: HpString, StString, DynString, TmpString, etc

// @TODO: make normal and test

class HpString {
    char *m_data = nullptr;
    uint32_t m_len = 0;

public:
    HpString() = default;
    HpString(const char *ptr, uint32_t len) : m_len(len) {
        assert(ptr);

        m_data = new char[m_len + 1];
        memcpy(m_data, ptr, m_len);
        m_data[m_len] = '\0';
    }
    explicit HpString(const char *ptr) : HpString(ptr, strlen(ptr)) {}

    HpString(const HpString &other) {
        if (&other == this)
            return;

        CopyFrom(other);
    }
    HpString& operator=(const HpString &other) {
        if (&other == this)
            return *this;

        CopyFrom(other);
        return *this;
    }

    ~HpString() {
        if (m_data)
            delete[] m_data;
    }

    uint32_t Length() const { return m_len; }
    const char *Begin() const { return m_data; }
    const char *End() const { return m_data + m_len; }

    // @TODO: correct implicit cast?
    const char *CStr() const { return Begin(); }

private:
    void CopyFrom(const HpString &other) {
        m_len = other.m_len;

        char *old_data = m_data;
        if (other.m_data) {
            m_data = new char[m_len + 1];
            memcpy(m_data, other.m_data, m_len);
            m_data[m_len] = '\0';
        } else
            m_data = nullptr;

        if (old_data)
            delete[] old_data;
    }
};
