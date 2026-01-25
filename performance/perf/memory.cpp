#include "perf/memory.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/memory/block_allocator.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/container/dynamic_array.hpp"
#include <fstream>

namespace TKit::Perf
{
struct ExampleData
{
    f64 Values[16];
    void SetValues()
    {
        for (usize i = 0; i < 16; ++i)
            Values[i] = static_cast<f64>(i);
    }
};

void RecordMallocFree(const AllocationSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/malloc_free.csv");
    DynamicArray<ExampleData *> allocated{settings.MaxPasses};

    file << "passes,malloc (ns),free (ns)\n";
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = new ExampleData;
        const Timespan allocTime = clock.Restart();

        for (usize i = 0; i < passes; ++i)
            delete allocated[i];
        const Timespan deallocTime = clock.GetElapsed();

        file << passes << ',' << allocTime.AsNanoseconds() << ',' << deallocTime.AsNanoseconds() << '\n';
    }
}

void RecordBlockAllocator(const AllocationSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/block_allocator.csv");
    DynamicArray<ExampleData *> allocated{settings.MaxPasses};
    file << "passes,block_alloc (ns),block_dealloc (ns)\n";

    BlockAllocator allocator = BlockAllocator::CreateFromType<ExampleData>(settings.MaxPasses);
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = allocator.Create<ExampleData>();
        const Timespan allocTime = clock.Restart();

        for (usize i = 0; i < passes; ++i)
            allocator.Destroy(allocated[i]);
        const Timespan deallocTime = clock.GetElapsed();

        file << passes << ',' << allocTime.AsNanoseconds() << ',' << deallocTime.AsNanoseconds() << '\n';
    }
}

void RecordStackAllocator(const AllocationSettings &settings)
{
    const char *path = "/performance/results/stack_allocator.csv";
    std::ofstream file(g_Root + path);
    DynamicArray<ExampleData *> allocated{settings.MaxPasses};
    file << "passes,stack_alloc (ns),stack_dealloc (ns)\n";

    StackAllocator allocator{static_cast<usize>(settings.MaxPasses * sizeof(ExampleData))};
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = allocator.Create<ExampleData>();
        const Timespan allocTime = clock.Restart();

        for (usize i = passes - 1; i < passes; --i)
            allocator.Destroy(allocated[i]);
        const Timespan deallocTime = clock.GetElapsed();

        file << passes << ',' << allocTime.AsNanoseconds() << ',' << deallocTime.AsNanoseconds() << '\n';
    }
}

void RecordArenaAllocator(const AllocationSettings &settings)
{
    const char *path = "/performance/results/arena_allocator.csv";
    std::ofstream file(g_Root + path);
    DynamicArray<ExampleData *> allocated{settings.MaxPasses};
    file << "passes,arena_alloc (ns)\n";

    ArenaAllocator allocator{static_cast<usize>(settings.MaxPasses * sizeof(ExampleData))};
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = allocator.Create<ExampleData>();
        const Timespan allocTime = clock.GetElapsed();

        allocator.Reset();

        file << passes << ',' << allocTime.AsNanoseconds() << '\n';
    }
}
} // namespace TKit::Perf
