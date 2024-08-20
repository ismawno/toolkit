#include "perf/memory.hpp"
#include "kit/profiling/clock.hpp"
#include "kit/memory/block_allocator.hpp"
#include "kit/memory/stack_allocator.hpp"
#include <fstream>
#include <thread>

KIT_NAMESPACE_BEGIN

void RecordMallocFree(const AllocationSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/malloc_free_st.csv");
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};

    file << "passes,malloc_st (ms),free_st (ms)\n";
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = new ExampleData;
        const Timespan allocTime = clock.Restart();

        for (usize i = 0; i < passes; ++i)
            delete allocated[i];
        const Timespan deallocTime = clock.Elapsed();

        file << passes << ',' << allocTime.AsMilliseconds() << ',' << deallocTime.AsMilliseconds() << '\n';
    }
}

void RecordBlockAllocator(const AllocationSettings &p_Settings)
{
    const char *path = "/performance/results/block_allocator.csv";
    std::ofstream file(g_Root + path);
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    file << "passes,block_alloc (ms),block_dealloc (ms)\n";

    BlockAllocator<ExampleData> allocator{p_Settings.MaxPasses / 2};
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = allocator.CreateUnsafe();
        const Timespan allocTime = clock.Restart();

        for (usize i = 0; i < passes; ++i)
            allocator.DestroyUnsafe(allocated[i]);
        const Timespan deallocTime = clock.Elapsed();

        file << passes << ',' << allocTime.AsMilliseconds() << ',' << deallocTime.AsMilliseconds() << '\n';
    }
}

void RecordStackAllocator(const AllocationSettings &p_Settings)
{
    const char *path = "/performance/results/stack_allocator.csv";
    std::ofstream file(g_Root + path);
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    file << "passes,stack_alloc (ms),stack_dealloc (ms)\n";

    StackAllocator allocator{p_Settings.MaxPasses * sizeof(ExampleData)};
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = allocator.Create<ExampleData>();
        const Timespan allocTime = clock.Restart();

        for (usize i = passes - 1; i < passes; --i)
            allocator.Destroy(allocated[i]);
        const Timespan deallocTime = clock.Elapsed();

        file << passes << ',' << allocTime.AsMilliseconds() << ',' << deallocTime.AsMilliseconds() << '\n';
    }
}

KIT_NAMESPACE_END