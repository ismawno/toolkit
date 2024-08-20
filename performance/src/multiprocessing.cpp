#include "perf/settings.hpp"
#include "kit/multiprocessing/thread_pool.hpp"
#include "kit/multiprocessing/for_each.hpp"
#include "kit/profiling/clock.hpp"
#include <fstream>

KIT_NAMESPACE_BEGIN

struct Number
{
    u32 Value;
    std::byte Padding[KIT_CACHE_LINE_SIZE - sizeof(u32)];
};

template <typename MTX> void RecordThreadPoolSum(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads)
{
    std::ofstream file(g_Root + "/performance/results/thread_pool_sum.csv");
    file << "threads,sum (ms),result\n";
    Clock clock;

    // u32 sum = 0;
    // for (usize i = 0; i < p_Settings.SumCount; i++)
    //     sum += i;

    // const Timespan stTime = clock.Elapsed();
    // file << "1," << stTime.AsMilliseconds() << ',' << sum << '\n';

    ThreadPool<MTX> threadPool(p_Maxthreads);
    DynamicArray<Ref<Task<u32>>> tasks(p_Maxthreads);
    DynamicArray<u32> values(p_Settings.SumCount);

    for (usize i = 0; i < p_Settings.SumCount; i++)
        values[i] = i;

    usize nthreads = 1;
    while (nthreads <= p_Maxthreads)
    {
        clock.Restart();
        ForEach(threadPool, values.begin(), values.end(), tasks.begin(), nthreads,
                [](auto p_It1, auto p_It2, const usize) {
                    u32 sum = 0;
                    for (auto it = p_It1; it != p_It2; ++it)
                        sum += *it;
                    return sum;
                });
        u32 sum = 0;
        for (usize i = 0; i < nthreads; i++)
            sum += tasks[i]->WaitForResult();

        const Timespan mtTime = clock.Elapsed();
        file << nthreads << ',' << mtTime.AsMilliseconds() << ',' << sum << '\n';
        nthreads *= 2;
    }
}

template void RecordThreadPoolSum<std::mutex>(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads);
template void RecordThreadPoolSum<SpinMutex>(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads);

KIT_NAMESPACE_END