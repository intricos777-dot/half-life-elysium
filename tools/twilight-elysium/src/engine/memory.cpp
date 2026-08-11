#include "memory.h"
#include <cstdlib>
#include <cstdio>
#include <atomic>

namespace te {

static std::atomic<size_t> g_total_allocated{0};

void* MemoryAllocator::allocate(size_t size, size_t alignment) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) return nullptr;
    g_total_allocated.fetch_add(size, std::memory_order_relaxed);
    return ptr;
}

void MemoryAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    free(ptr);
}

size_t MemoryAllocator::get_total_allocated() {
    return g_total_allocated.load(std::memory_order_relaxed);
}

void MemoryAllocator::dump_stats() {
    std::printf("[Memory] Total allocated: %zu bytes\n", get_total_allocated());
}

} // namespace te
