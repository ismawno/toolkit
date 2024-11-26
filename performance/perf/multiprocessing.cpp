#include "perf/settings.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/multiprocessing/for_each.hpp"
#include "tkit/profiling/clock.hpp"
#include <fstream>

namespace TKit
{
struct Number
{
    u32 Value;
    std::byte Padding[TKIT_CACHE_LINE_SIZE - sizeof(u32)];
};

template <typename MTX> void RecordThreadPoolSum(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads)
{
    std::ofstream file(g_Root + "/performance/results/thread_pool_sum.csv");
    file << "threads,sum (ns),result\n";

    ThreadPool<MTX> threadPool(p_Maxthreads);
    DynamicArray<Ref<Task<u32>>> tasks(p_Maxthreads);
    DynamicArray<u32> values(p_Settings.SumCount);

    for (u32 i = 0; i < p_Settings.SumCount; i++)
        values[i] = i;

    usize nthreads = 1;
    while (nthreads <= p_Maxthreads)
    {
        ForEach(threadPool, values.begin(), values.end(), tasks.begin(), nthreads,
                [](auto p_It1, auto p_It2, const usize) {
                    u32 sum = 0;
                    for (auto it = p_It1; it != p_It2; ++it)
                        sum += *it;
                    return sum;
                });

        Clock clock;
        u32 sum = 0;
        for (usize i = 0; i < nthreads; i++)
            sum += tasks[i]->WaitForResult();

        const Timespan mtTime = clock.GetElapsed();
        file << nthreads << ',' << mtTime.AsNanoseconds() << ',' << sum << '\n';
        nthreads *= 2;
    }
}

void RecordParallelSum(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads)
{
    std::ofstream file(g_Root + "/performance/results/parallel_sum.csv");
    file << "threads,sum (ns),result\n";

    DynamicArray<std::thread> threads(p_Maxthreads);
    DynamicArray<u32> sums(p_Maxthreads);
    DynamicArray<Ref<Task<u32>>> tasks(p_Maxthreads);
    DynamicArray<u32> values(p_Settings.SumCount);

    for (u32 i = 0; i < p_Settings.SumCount; i++)
        values[i] = i;

    usize nthreads = 1;
    while (nthreads <= p_Maxthreads)
    {

        for (usize i = 0; i < nthreads; i++)
        {
            const usize start = i * p_Settings.SumCount / nthreads;
            const usize end = (i + 1) * p_Settings.SumCount / nthreads;
            threads[i] = std::thread([i, start, end, &sums, &values]() {
                u32 sum = 0;
                for (usize j = start; j < end; ++j)
                    sum += values[j];
                sums[i] = sum;
            });
        }
        Clock clock;
        u32 sum = 0;
        for (usize i = 0; i < nthreads; i++)
        {
            threads[i].join();
            sum += sums[i];
        }

        const Timespan mtTime = clock.GetElapsed();
        file << nthreads << ',' << mtTime.AsNanoseconds() << ',' << sum << '\n';
        nthreads *= 2;
    }
}

template void RecordThreadPoolSum<std::mutex>(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads);
template void RecordThreadPoolSum<SpinLock>(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads);
} // namespace TKit