#include "perf/settings.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/multiprocessing/for_each.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/utils/literals.hpp"
#include <fstream>

namespace TKit::Perf
{
static ArenaAllocator s_Alloc{1_kib};
struct Number
{
    u32 Value;
    std::byte Padding[TKIT_CACHE_LINE_SIZE - sizeof(u32)];
};

void RecordThreadPoolSum(const ThreadPoolSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/thread_pool_sum.csv");
    file << "threads,sum (ns),result\n";

    ThreadPool threadPool(&s_Alloc, settings.MaxThreads);
    StaticArray128<Task<u32>> tasks(settings.MaxThreads);
    DynamicArray<u32> values(settings.SumCount);

    for (u32 i = 0; i < settings.SumCount; ++i)
        values[i] = i;

    usize nthreads = 1;
    while (nthreads <= settings.MaxThreads)
    {
        NonBlockingForEach(threadPool, values.begin(), values.end(), tasks.begin(), nthreads, [](auto it1, auto it2) {
            u32 sum = 0;
            for (auto it = it1; it != it2; ++it)
                sum += *it;
            return sum;
        });

        Clock clock;
        u32 sum = 0;
        for (usize i = 0; i < nthreads; ++i)
        {
            sum += tasks[i].WaitForResult();
            tasks[i].Reset();
        }

        const Timespan mtTime = clock.GetElapsed();
        file << nthreads << ',' << mtTime.AsNanoseconds() << ',' << sum << '\n';
        nthreads *= 2;
    }
}

void RecordParallelSum(const ThreadPoolSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/parallel_sum.csv");
    file << "threads,sum (ns),result\n";

    DynamicArray<std::thread> threads(settings.MaxThreads);
    DynamicArray<u32> sums(settings.MaxThreads);
    DynamicArray<u32> values(settings.SumCount);

    for (u32 i = 0; i < settings.SumCount; ++i)
        values[i] = i;

    usize nthreads = 1;
    while (nthreads <= settings.MaxThreads)
    {

        for (usize i = 0; i < nthreads; ++i)
        {
            const usize start = i * settings.SumCount / nthreads;
            const usize end = (i + 1) * settings.SumCount / nthreads;
            threads[i] = std::thread([i, start, end, &sums, &values] {
                u32 sum = 0;
                for (usize j = start; j < end; ++j)
                    sum += values[j];
                sums[i] = sum;
            });
        }
        Clock clock;
        u32 sum = 0;
        for (usize i = 0; i < nthreads; ++i)
        {
            threads[i].join();
            sum += sums[i];
        }

        const Timespan mtTime = clock.GetElapsed();
        file << nthreads << ',' << mtTime.AsNanoseconds() << ',' << sum << '\n';
        nthreads *= 2;
    }
}
} // namespace TKit::Perf
