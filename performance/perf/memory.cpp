#include "perf/memory.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/memory/block_allocator.hpp"
#include "tkit/memory/stack_allocator.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include <fstream>
#include <thread>

namespace TKit
{
struct ExampleData
{
    f64 Values[16];
    void SetValues() noexcept
    {
        for (usize i = 0; i < 16; ++i)
            Values[i] = static_cast<f64>(i);
    }
};

void RecordMallocFree(const AllocationSettings &p_Settings) noexcept
{
    std::ofstream file(g_Root + "/performance/results/malloc_free.csv");
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};

    file << "passes,malloc (ns),free (ns)\n";
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
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

void RecordBlockAllocator(const AllocationSettings &p_Settings) noexcept
{
    std::ofstream file(g_Root + "/performance/results/block_allocator.csv");
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    file << "passes,block_alloc (ns),block_dealloc (ns)\n";

    BlockAllocator allocator = BlockAllocator::CreateFromType<ExampleData>(p_Settings.MaxPasses);
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
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

void RecordStackAllocator(const AllocationSettings &p_Settings) noexcept
{
    const char *path = "/performance/results/stack_allocator.csv";
    std::ofstream file(g_Root + path);
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    file << "passes,stack_alloc (ns),stack_dealloc (ns)\n";

    StackAllocator allocator{p_Settings.MaxPasses * sizeof(ExampleData)};
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
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

void RecordArenaAllocator(const AllocationSettings &p_Settings) noexcept
{
    const char *path = "/performance/results/arena_allocator.csv";
    std::ofstream file(g_Root + path);
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    file << "passes,arena_alloc (ns)\n";

    ArenaAllocator allocator{p_Settings.MaxPasses * sizeof(ExampleData)};
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        for (usize i = 0; i < passes; ++i)
            allocated[i] = allocator.Create<ExampleData>();
        const Timespan allocTime = clock.GetElapsed();

        allocator.Reset();

        file << passes << ',' << allocTime.AsNanoseconds() << '\n';
    }
}
} // namespace TKit