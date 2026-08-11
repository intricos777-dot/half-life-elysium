#pragma once
#include <cstddef>
#include <cstdint>

namespace te {

class MemoryAllocator {
public:
    static void* allocate(size_t size, size_t alignment = 16);
    static void deallocate(void* ptr);

    static size_t get_total_allocated();
    static void dump_stats();
};

} // namespace te
