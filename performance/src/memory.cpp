#include "perf/memory.hpp"
#include "kit/profiling/clock.hpp"
#include "kit/memory/block_allocator.hpp"
#include <fstream>
#include <thread>

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

void RecordMallocFreeMultiThreaded(const AllocationSettings &p_Settings, const usz p_MaxThreads)
{
    std::ofstream file(g_Root + "/performance/results/malloc_free_mt.csv");

    DynamicArray<PaddedData> allocated{p_Settings.MaxPasses};
    DynamicArray<std::thread> threads{p_MaxThreads};

    const auto alloc = [&allocated](const usz passes, const usz tindex) {
        for (usz i = 0; i < passes; ++i)
        {
            PaddedData data;
            data.Data = new ExampleData;
            allocated[tindex * passes + i] = data;
        }
    };

    const auto dealloc = [&allocated](const usz passes, const usz tindex) {
        for (usz i = 0; i < passes; ++i)
            delete allocated[tindex * passes + i].Data;
    };

    // Would be nice to replace this with a thread pool
    file << "threads,passes,malloc_mt (ms),free_mt (ms)\n";
    for (usz th = 2; th <= p_MaxThreads; ++th)
        for (usz passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; ++passes)
        {
            for (usz i = 0; i < th; ++i)
                threads[i] = std::thread(alloc, passes / th, i);
            Clock clock;
            for (usz i = 0; i < th; ++i)
                threads[i].join();
            const Timespan allocTime = clock.Elapsed();

            for (usz i = 0; i < th; ++i)
                threads[i] = std::thread(dealloc, passes / th, i);
            clock.Restart();
            for (usz i = 0; i < th; ++i)
                threads[i].join();
            const Timespan deallocTime = clock.Elapsed();

            file << th << ',' << passes << ',' << allocTime.AsMilliseconds() << ',' << deallocTime.AsMilliseconds()
                 << '\n';
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
        file << "passes,ts_block_alloc_st (ms),ts_block_dealloc_st (ms)\n";
    else if constexpr (std::is_same_v<Allocator<ExampleData>, TUnsafeBlockAllocator<ExampleData>>)
        file << "passes,tu_block_alloc_st (ms),tu_block_dealloc_st (ms)\n";

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

template <template <typename> typename Allocator>
void RecordBlockAllocatorMultiThreaded(const AllocationSettings &p_Settings, usz p_MaxThreads)
{
    const char *path = nullptr;
    if constexpr (std::is_same_v<Allocator<ExampleData>, TSafeBlockAllocator<ExampleData>>)
        path = "/performance/results/ts_block_allocator_mt.csv";
    else if constexpr (std::is_same_v<Allocator<ExampleData>, TUnsafeBlockAllocator<ExampleData>>)
        path = "/performance/results/tu_block_allocator_mt.csv";

    std::ofstream file(g_Root + path);

    DynamicArray<PaddedData> allocated{p_Settings.MaxPasses};
    DynamicArray<std::thread> threads{p_MaxThreads};
    Allocator<ExampleData> allocator{p_Settings.MaxPasses / 2};

    const auto alloc = [&allocated, &allocator](const usz passes, const usz tindex) {
        for (usz i = 0; i < passes; ++i)
        {
            PaddedData data;
            data.Data = allocator.Construct();
            allocated[tindex * passes + i] = data;
        }
    };

    const auto dealloc = [&allocated, &allocator](const usz passes, const usz tindex) {
        for (usz i = 0; i < passes; ++i)
            allocator.Destroy(allocated[tindex * passes + i].Data);
    };

    // Would be nice to replace this with a thread pool
    if constexpr (std::is_same_v<Allocator<ExampleData>, TSafeBlockAllocator<ExampleData>>)
        file << "threads,passes,ts_block_alloc_mt (ms),ts_block_dealloc_mt (ms)\n";
    else if constexpr (std::is_same_v<Allocator<ExampleData>, TUnsafeBlockAllocator<ExampleData>>)
        file << "threads,passes,tu_block_alloc_mt (ms),tu_block_dealloc_mt (ms)\n";
    for (usz th = 2; th <= p_MaxThreads; ++th)
        for (usz passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; ++passes)
        {
            for (usz i = 0; i < th; ++i)
                threads[i] = std::thread(alloc, passes / th, i);
            Clock clock;
            for (usz i = 0; i < th; ++i)
                threads[i].join();
            const Timespan allocTime = clock.Elapsed();

            for (usz i = 0; i < th; ++i)
                threads[i] = std::thread(dealloc, passes / th, i);
            clock.Restart();
            for (usz i = 0; i < th; ++i)
                threads[i].join();
            const Timespan deallocTime = clock.Elapsed();

            file << th << ',' << passes << ',' << allocTime.AsMilliseconds() << ',' << deallocTime.AsMilliseconds()
                 << '\n';
        }
}

template void RecordBlockAllocatorSingleThreaded<TSafeBlockAllocator>(const AllocationSettings &p_Settings);
template void RecordBlockAllocatorSingleThreaded<TUnsafeBlockAllocator>(const AllocationSettings &p_Settings);

template void RecordBlockAllocatorMultiThreaded<TSafeBlockAllocator>(const AllocationSettings &p_Settings,
                                                                     usz p_MaxThreads);
template void RecordBlockAllocatorMultiThreaded<TUnsafeBlockAllocator>(const AllocationSettings &p_Settings,
                                                                       usz p_MaxThreads);

KIT_NAMESPACE_END