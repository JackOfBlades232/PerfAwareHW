#include <profiling.hpp>
#include <memory.hpp>
#include <os.hpp>
#include <defs.hpp>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: <program> [page count]\n");
        return 1;
    }

    usize const page_count = atol(argv[1]);

    init_os_process_state(g_os_proc_state);

    usize const mem_bytes = page_count * g_os_proc_state.regular_page_size;

    for (usize cnt = 1; cnt <= page_count; ++cnt) {
        char *mem = (char *)allocate_os_pages_memory(mem_bytes);

        usize begin_faults = READ_PAGE_FAULT_COUNTER();
        for (usize i = 0; i < cnt * g_os_proc_state.regular_page_size; ++i)
            mem[i] = char(i);
        usize fault_cnt = READ_PAGE_FAULT_COUNTER() - begin_faults;
        printf("%zu,%zu,%zu,%lld\n", page_count, cnt, fault_cnt, i64(fault_cnt - cnt));
        
        free_os_pages_memory(mem, mem_bytes);
    }
    return 0;
}
