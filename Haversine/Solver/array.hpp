#pragma once

#include "util.hpp"

#include <cassert>
#include <cstdint>

// @TODO: work out semantics, test

template <typename T>
class DynArray {
    T *m_data = nullptr;
    uint32_t m_len = 0, m_cap = 0;

    static constexpr uint32_t c_min_cap = 8;

public:
    DynArray() = default;

    DynArray(const DynArray &other) = delete;
    DynArray& operator=(const DynArray &other) = delete;

    DynArray(DynArray &&other)
        : m_data{xchg(other.m_data, nullptr)}
        , m_len{xchg(other.m_len, 0)}
        , m_cap{xchg(other.m_cap, 0)}
    {}
    DynArray& operator=(DynArray &&other) {
        DynArray tmp{mv(other)};
        swp(m_data, tmp.m_data);
        swp(m_len, tmp.m_len);
        swp(m_cap, tmp.m_cap);
        return *this;
    }

    ~DynArray() {
        if (m_data)
            delete[] m_data;
    }

    void Append(T elem) {
        if (m_len == m_cap)
            Grow(max(2 * m_cap, c_min_cap));

        m_data[m_len++] = mv(elem);
    }

    void Clear() {
        if (m_data)
            delete[] m_data;
        m_data = nullptr;
        m_len = m_cap = 0;
    }

    uint32_t Length() const { return m_len; }
    uint32_t Empty() const { return m_len == 0; }

    T &At(uint32_t idx) {
        assert(idx < m_len);
        return m_data[idx];
    }

    const T &At(uint32_t idx) const {
        assert(idx < m_len);
        return m_data[idx];
    }

    void Reserve(uint32_t new_cap) { Grow(new_cap); }

    T *Begin() { return m_data; }
    T *End() { return m_data + m_len; }
    const T *Begin() const { return m_data; }
    const T *End() const { return m_data + m_len; }
    const T *CBegin() const { return Begin(); }
    const T *CEnd() const { return End(); }

    T &operator[](uint32_t idx) { return At(idx); }
    const T &operator[](uint32_t idx) const { return At(idx); }

    static DynArray Copy(const DynArray &other) {
        DynArray arr;
        arr.m_len = other.m_len;
        arr.m_cap = arr.m_len;

        T *old_data = arr.m_data;
        if (other.m_data) {
            arr.m_data = new T[arr.m_len];
            for (uint32_t i = 0; i < arr.m_len; ++i)
                arr.m_data[i] = mv(other.m_data[i]);
        } else
            arr.m_data = nullptr;

        if (old_data)
            delete[] old_data;

        return arr;
    }

private:
    void Grow(uint32_t new_cap) {
        if (new_cap <= m_cap)
            return;

        T *new_data = new T[new_cap];
        if (m_data) {
            for (uint32_t i = 0; i < m_len; ++i)
                new_data[i] = mv(m_data[i]);

            delete[] m_data;
        }

        m_data = new_data;
        m_cap = new_cap;
    }
};
