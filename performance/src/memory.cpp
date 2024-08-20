#include "perf/memory.hpp"
#include "kit/profiling/clock.hpp"
#include "kit/memory/block_allocator.hpp"
#include "kit/memory/stack_allocator.hpp"
#include <fstream>
#include <thread>

KIT_NAMESPACE_BEGIN

struct ExampleData
{
    f64 Values[16];
    void SetValues()
    {
        for (usize i = 0; i < 16; ++i)
            Values[i] = static_cast<f64>(i);
    }
};

void RecordMallocFreeST(const AllocationSettings &p_Settings)
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

void RecordMallocFreeMT(const AllocationSettings &p_Settings, const usize p_MaxThreads)
{
    std::ofstream file(g_Root + "/performance/results/malloc_free_mt.csv");
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    DynamicArray<std::thread> threads{p_MaxThreads};

    const auto allocate = [&allocated](const usize p_Start, const usize p_End) {
        for (usize i = p_Start; i < p_End; ++i)
            allocated[i] = new ExampleData;
    };
    const auto deallocate = [&allocated](const usize p_Start, const usize p_End) {
        for (usize i = p_Start; i < p_End; ++i)
            delete allocated[i];
    };

    file << "threads,passes,malloc_mt (ms),free_mt (ms)\n";

    usize nthreads = 1;
    while (nthreads <= p_MaxThreads)
    {
        for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
        {
            for (usize th = 0; th < nthreads; ++th)
            {
                const usize start = th * passes / nthreads;
                const usize end = (th + 1) * passes / nthreads;
                threads[th] = std::thread(allocate, start, end);
            }

            // Including thread creation in measurement seems a bit unfair
            Clock clock;
            for (usize th = 0; th < nthreads; ++th)
                threads[th].join();
            const Timespan allocTime = clock.Elapsed();

            for (usize th = 0; th < nthreads; ++th)
            {
                const usize start = th * passes / nthreads;
                const usize end = (th + 1) * passes / nthreads;
                threads[th] = std::thread(deallocate, start, end);
            }

            clock.Restart();
            for (usize th = 0; th < nthreads; ++th)
                threads[th].join();
            const Timespan deallocTime = clock.Elapsed();

            file << nthreads << ',' << passes << ',' << allocTime.AsMilliseconds() << ','
                 << deallocTime.AsMilliseconds() << '\n';
        }
        nthreads *= 2;
    }
}

void RecordBlockAllocatorST(const AllocationSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/block_allocator_st.csv");
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    file << "passes,block_alloc_st (ms),block_dealloc_st (ms)\n";

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

void RecordBlockAllocatorMT(const AllocationSettings &p_Settings, const usize p_MaxThreads)
{
    std::ofstream file(g_Root + "/performance/results/block_allocator_mt.csv");
    DynamicArray<ExampleData *> allocated{p_Settings.MaxPasses};
    DynamicArray<std::thread> threads{p_MaxThreads};

    BlockAllocator<ExampleData> allocator{p_Settings.MaxPasses / 2};
    const auto allocate = [&allocated, &allocator](const usize p_Start, const usize p_End) {
        for (usize i = p_Start; i < p_End; ++i)
            allocated[i] = allocator.Create();
    };
    const auto deallocate = [&allocated, &allocator](const usize p_Start, const usize p_End) {
        for (usize i = p_Start; i < p_End; ++i)
            allocator.Destroy(allocated[i]);
    };

    file << "threads,passes,block_alloc_mt (ms),block_dealloc_mt (ms)\n";

    usize nthreads = 1;
    while (nthreads <= p_MaxThreads)
    {
        for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
        {
            for (usize th = 0; th < nthreads; ++th)
            {
                const usize start = th * passes / nthreads;
                const usize end = (th + 1) * passes / nthreads;
                threads[th] = std::thread(allocate, start, end);
            }

            Clock clock;
            for (usize th = 0; th < nthreads; ++th)
                threads[th].join();
            const Timespan allocTime = clock.Restart();

            for (usize th = 0; th < nthreads; ++th)
            {
                const usize start = th * passes / nthreads;
                const usize end = (th + 1) * passes / nthreads;
                threads[th] = std::thread(deallocate, start, end);
            }

            clock.Restart();
            for (usize th = 0; th < nthreads; ++th)
                threads[th].join();
            const Timespan deallocTime = clock.Elapsed();

            file << nthreads << ',' << passes << ',' << allocTime.AsMilliseconds() << ','
                 << deallocTime.AsMilliseconds() << '\n';
        }
        nthreads *= 2;
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