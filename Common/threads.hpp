#pragma once

#include "os.hpp"

#if _WIN32

struct os_thread_t {
    HANDLE hnd = INVALID_HANDLE_VALUE;
    DWORD id = DWORD(-1);
};

inline bool is_valid(os_thread_t const &h)
{
    return h.hnd != INVALID_HANDLE_VALUE;
}

#define THREAD_ENTRY(fname_, dname_) DWORD WINAPI fname_(LPVOID dname_)

using thread_entry_func_t = DWORD (WINAPI *)(LPVOID);
using thread_payload_t = LPVOID;

inline os_thread_t os_spawn_thread(
    thread_entry_func_t entry, thread_payload_t payload = nullptr)
{
    os_thread_t h = {};
    h.hnd = CreateThread(nullptr, 0, entry, payload, 0, &h.id);
    if (!is_valid(h))
        return {};
    return h;
}

inline void os_join_thread(os_thread_t &h)
{
    assert(is_valid(h));
    auto res = WaitForMultipleObjects(1, &h.hnd, TRUE, INFINITE);
    assert(res == WAIT_OBJECT_0); 
    CloseHandle(h.hnd);
    h = {};
}

#else

#include <pthread.h>

struct os_thread_t {
    pthread_t hnd;
    bool has_value = false;
};

inline bool is_valid(os_thread_t const &h)
{
    return h.has_value;
}

#define THREAD_ENTRY(fname_, dname_) void *fname_(void *dname_)

using thread_entry_func_t = void *(*)(void *);
using thread_payload_t = void *;

inline os_thread_t os_spawn_thread(
    thread_entry_func_t entry, thread_payload_t payload = nullptr)
{
    os_thread_t h = {};
    h.has_value = pthread_create(&h.hnd, nullptr, entry, payload) == 0;
    return h;
}

inline void os_join_thread(os_thread_t &h)
{
    assert(is_valid(h));
    int res = pthread_join(h.hnd, nullptr);
    assert(res == 0);
    h = {};
}

#endif
