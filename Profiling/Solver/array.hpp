#pragma once

#include <cassert>
#include <cstdint>

// @TODO: pull out to some utils, and complete features when needed
// Idea: HpArray, StArray, DynArray, TmpArray, etc

#define FOR(arr_) for (decltype(arr_[0]) it = arr_.Begin(); it != arr_.End(); ++it)

// @TODO: pull out
template <typename T>
static T max(T a, T b)
{
    return a > b ? a : b;
}

// @TODO: make normal w/ inplace news, and test

template <typename T>
class DynArray {
    T *m_data = nullptr;
    uint32_t m_len = 0, m_cap = 0;

    static constexpr uint32_t c_min_cap = 8;

public:
    DynArray() = default;
    DynArray(const DynArray &other) {
        if (&other == this)
            return;

        CopyFrom(other);
    }
    DynArray& operator=(const DynArray &other) {
        if (&other == this)
            return *this;

        CopyFrom(other);
        return *this;
    }

    ~DynArray() {
        if (m_data)
            delete[] m_data;
    }

    void Add(const T &elem) {
        if (m_len == m_cap)
            Grow(max(2 * m_cap, c_min_cap));

        m_data[m_len++] = elem;
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

private:
    void Grow(uint32_t new_cap) {
        if (new_cap <= m_cap)
            return;

        T *new_data = new T[new_cap];
        if (m_data) {
            for (uint32_t i = 0; i < m_len; ++i)
                new_data[i] = m_data[i];

            delete[] m_data;
        }

        m_data = new_data;
        m_cap = new_cap;
    }

    void CopyFrom(const DynArray &other) {
        m_len = other.m_len;
        m_cap = m_len;

        T *old_data = m_data;
        if (other.m_data) {
            m_data = new T[m_len];
            for (uint32_t i = 0; i < m_len; ++i)
                m_data[i] = other.m_data[i];
        } else
            m_data = nullptr;

        if (old_data)
            delete[] old_data;
    }
};
