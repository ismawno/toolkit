#include "perf/memory/block_allocator.hpp"
#include "kit/memory/block_allocator.hpp"
#include "kit/profiling/clock.hpp"
#include <fstream>
#include <filesystem>

KIT_NAMESPACE_BEGIN

#ifdef KIT_BLOCK_ALLOCATOR_THREAD_SAFE
static const std::filesystem::path s_Directory =
    std::string(KIT_ROOT_PATH) + "/performance/results/block-allocator/thread-safe";
#else
static const std::filesystem::path s_Directory =
    std::string(KIT_ROOT_PATH) + "/performance/results/block-allocator/thread-unsafe";
#endif

struct ExampleData
{
    f64 Values[16];
    void SetValues()
    {
        for (usz i = 0; i < 16; ++i)
            Values[i] = static_cast<f64>(i);
    }
};

static void recordAllocation(const usz passes, std::ofstream &file,
                             DynamicArray<BlockAllocator<ExampleData>> &p_Allocators,
                             DynamicArray<DynamicArray<ExampleData *>> &p_BlockAllocated,
                             DynamicArray<DynamicArray<ExampleData *>> &p_MallocAllocated)
{
    // In this scenario, for loops are flipped (usually the outer loop is the one that iterates over the
    // containers), but I want to emphasize the ability for the block allocator to maintain reasonable contiguity.
    // By continually allocating from the new operator for the same container uninterrupted, the memory layout we
    // will get from the new operator will be contiguous. This, however, doesnt truly represent a real-world
    // scenario, as we would be allocating from different containers in a random order. That is why I want to
    // separate the allocation calls for the block allocator and the new operator.

    Clock clock;
    for (usz i = 0; i < passes; ++i)
        for (usz j = 0; j < p_BlockAllocated.size(); ++j)
            p_BlockAllocated[j][i] = p_Allocators[j].Construct();
    const Timespan blockTime = clock.Restart();

    for (usz i = 0; i < passes; ++i)
        for (usz j = 0; j < p_MallocAllocated.size(); ++j)
            p_MallocAllocated[j][i] = new ExampleData;
    const Timespan mallocTime = clock.Elapsed();

    file << passes * p_Allocators.size() << ',' << blockTime.AsMilliseconds() << ',' << mallocTime.AsMilliseconds()
         << '\n';
}

static void recordTraversal(const usz passes, std::ofstream &file,
                            DynamicArray<DynamicArray<ExampleData *>> &p_BlockAllocated,
                            DynamicArray<DynamicArray<ExampleData *>> &p_MallocAllocated)
{

    Clock clock;
    for (usz i = 0; i < p_BlockAllocated.size(); ++i)
        for (usz j = 0; j < passes; ++j)
            p_BlockAllocated[i][j]->SetValues();
    const Timespan blockTime = clock.Restart();

    for (usz i = 0; i < p_MallocAllocated.size(); ++i)
        for (usz j = 0; j < passes; ++j)
            p_MallocAllocated[i][j]->SetValues();
    const Timespan mallocTime = clock.Elapsed();

    file << passes * p_BlockAllocated.size() << ',' << blockTime.AsMilliseconds() << ',' << mallocTime.AsMilliseconds()
         << '\n';
}

static void recordDeallocation(const usz passes, std::ofstream &file,
                               DynamicArray<BlockAllocator<ExampleData>> &p_Allocators,
                               DynamicArray<DynamicArray<ExampleData *>> &p_BlockAllocated,
                               DynamicArray<DynamicArray<ExampleData *>> &p_MallocAllocated)
{
    Clock clock;
    for (usz i = 0; i < p_BlockAllocated.size(); ++i)
        for (usz j = 0; j < passes; ++j)
            p_Allocators[i].Destroy(p_BlockAllocated[i][j]);
    const Timespan blockTime = clock.Restart();

    for (usz i = 0; i < p_MallocAllocated.size(); ++i)
        for (usz j = 0; j < passes; ++j)
            delete p_MallocAllocated[i][j];
    const Timespan mallocTime = clock.Elapsed();

    file << passes * p_Allocators.size() << ',' << blockTime.AsMilliseconds() << ',' << mallocTime.AsMilliseconds()
         << '\n';
}

void RunBlockAllocatorBenchmarks(const BenchmarkSettings &p_Settings)
{
    DynamicArray<BlockAllocator<ExampleData>> allocators;
    DynamicArray<DynamicArray<ExampleData *>> blockAllocated;
    DynamicArray<DynamicArray<ExampleData *>> mallocAllocated;

    for (usz i = 0; i < p_Settings.Containers; ++i)
    {
        allocators.emplace_back(64 * 64);
        blockAllocated.emplace_back(p_Settings.MaxPasses);
        mallocAllocated.emplace_back(p_Settings.MaxPasses);
    }

    if (!std::filesystem::exists(s_Directory))
        std::filesystem::create_directories(s_Directory);

    std::ofstream allocations{s_Directory.string() + "/allocation.csv"};
    std::ofstream traversal{s_Directory.string() + "/traversal.csv"};
    std::ofstream deallocations{s_Directory.string() + "/deallocation.csv"};

    allocations << "allocations,block (ms),malloc (ms)\n";
    traversal << "traversals,block (ms),malloc (ms)\n";
    deallocations << "deallocations,block (ms),malloc (ms)\n";

    for (usz passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        recordAllocation(passes, allocations, allocators, blockAllocated, mallocAllocated);
        recordTraversal(passes, traversal, blockAllocated, mallocAllocated);
        recordDeallocation(passes, deallocations, allocators, blockAllocated, mallocAllocated);
    }
}

KIT_NAMESPACE_END