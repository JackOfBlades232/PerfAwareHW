#pragma once

#include <cstdint>

#if _WIN32

#include <windows.h>

#pragma comment(lib, "Advapi32.lib")

struct os_process_state_t {
    HANDLE process_hnd;
    uint64_t large_page_sz = 0; // 0 means disabled
};

inline void init_os_process_state(os_process_state_t &st)
{
    st.process_hnd = GetCurrentProcess();
}

inline bool try_enable_large_pages(os_process_state_t &st)
{
    HANDLE token_hnd;

    if (OpenProcessToken(st.process_hnd, TOKEN_ADJUST_PRIVILEGES, &token_hnd)) {
        TOKEN_PRIVILEGES privs = {};
        privs.PrivilegeCount = 1;
        privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (LookupPrivilegeValue(nullptr, SE_LOCK_MEMORY_NAME,
                                 &privs.Privileges[0].Luid)) 
        {
            AdjustTokenPrivileges(token_hnd, FALSE, &privs, 0, 0, 0);
            if (GetLastError() == ERROR_SUCCESS)
                st.large_page_sz = GetLargePageMinimum();
        }
        
        CloseHandle(token_hnd);
    }

    return st.large_page_sz > 0;
}

inline int64_t get_last_os_error()
{
    return int64_t(GetLastError());
}

#else

struct os_process_state_t {
    // @TODO
};

inline void init_os_process_state(os_process_state_t &st)
{
    // @TODO
}

inline bool try_enable_large_pages(os_process_state_t &st)
{
    // @TODO
    return false;
}

inline int64_t get_last_os_error()
{
    // @TODO
    return 0;
}

#endif

inline os_process_state_t g_os_proc_state{};
