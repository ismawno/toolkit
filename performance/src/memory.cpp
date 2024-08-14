#include "perf/memory.hpp"
#include "kit/profiling/clock.hpp"
#include "kit/memory/block_allocator.hpp"
#include <fstream>

KIT_NAMESPACE_BEGIN

void RecordMallocFreeSingleThreaded(const AllocationSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/malloc_free_st.csv");
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};

    file << "passes,malloc_st (ms),free_st (ms)\n";
    for (usz passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        for (usz i = 0; i < passes; ++i)
            allocated[i] = new ExampleData;
        const Timespan allocTime = clock.Restart();

        for (usz i = 0; i < passes; ++i)
            delete allocated[i];
        const Timespan deallocTime = clock.Elapsed();

        file << passes << ',' << allocTime.AsMilliseconds() << ',' << deallocTime.AsMilliseconds() << '\n';
    }
}

template <template <typename> typename Allocator>
void RecordBlockAllocatorSingleThreaded(const AllocationSettings &p_Settings)
{
    const char *path = nullptr;
    if constexpr (std::is_same_v<Allocator<ExampleData>, TSafeBlockAllocator<ExampleData>>)
        path = "/performance/results/ts_block_allocator_st.csv";
    else if constexpr (std::is_same_v<Allocator<ExampleData>, TUnsafeBlockAllocator<ExampleData>>)
        path = "/performance/results/tu_block_allocator_st.csv";

    std::ofstream file(g_Root + path);
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    if constexpr (std::is_same_v<Allocator<ExampleData>, TSafeBlockAllocator<ExampleData>>)
        file << "passes,ts_block_alloc_st,ts_block_dealloc_st\n";
    else if constexpr (std::is_same_v<Allocator<ExampleData>, TUnsafeBlockAllocator<ExampleData>>)
        file << "passes,tu_block_alloc_st,tu_block_dealloc_st\n";

    Allocator<ExampleData> allocator{p_Settings.MaxPasses / 2};
    for (usz passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        for (usz i = 0; i < passes; ++i)
            allocated[i] = allocator.Construct();
        const Timespan allocTime = clock.Restart();

        for (usz i = 0; i < passes; ++i)
            allocator.Destroy(allocated[i]);
        const Timespan deallocTime = clock.Elapsed();

        file << passes << ',' << allocTime.AsMilliseconds() << ',' << deallocTime.AsMilliseconds() << '\n';
    }
}

template void RecordBlockAllocatorSingleThreaded<TSafeBlockAllocator>(const AllocationSettings &p_Settings);
template void RecordBlockAllocatorSingleThreaded<TUnsafeBlockAllocator>(const AllocationSettings &p_Settings);

KIT_NAMESPACE_END