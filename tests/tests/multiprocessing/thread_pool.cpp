#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/multiprocessing/for_each.hpp"
#include "tkit/multiprocessing/spin_lock.hpp"
#include "tkit/container/array.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
template <Mutex MTX> void RunThreadPoolTest()
{
    constexpr u32 threadCount = 4;
    constexpr u32 amount = 1000;
    ThreadPool<MTX> pool(threadCount);

    SECTION("CreateAndSubmit")
    {
        for (u32 i = 0; i < amount; i++)
        {
            const auto task1 = pool.CreateAndSubmit([](const usize) { return 7 + 1; });
            const auto task2 = pool.CreateAndSubmit([](const usize) { return 9 + 11; });
            REQUIRE(task1->WaitForResult() == 8);
            REQUIRE(task2->WaitForResult() == 20);
        }
    }

    struct Number
    {
        u32 Value;
        std::byte Padding[TKIT_CACHE_LINE_SIZE - sizeof(u32)];
    };

    SECTION("Parallel for")
    {
        Array<Number, amount> numbers;
        u32 realSum = 0;
        for (u32 i = 0; i < amount; i++)
        {
            numbers[i].Value = i;
            realSum += i;
        }

        Array<Ref<Task<u32>>, threadCount> tasks;
        ForEach(pool, numbers.begin(), numbers.end(), tasks.begin(), threadCount,
                [](auto p_It1, auto p_It2, const usize) {
                    u32 sum = 0;
                    for (auto it = p_It1; it != p_It2; ++it)
                        sum += it->Value;
                    return sum;
                });
        u32 sum = 0;
        for (auto &task : tasks)
            sum += task->WaitForResult();
        REQUIRE(sum == realSum);
    }

    SECTION("Parallel for (void return)")
    {
        Array<Number, amount> numbers;
        u32 realSum = 0;
        for (u32 i = 0; i < amount; i++)
        {
            numbers[i].Value = i;
            realSum += i;
        }

        std::atomic<u32> taskSum = 0;
        ForEach(pool, numbers.begin(), numbers.end(), threadCount, [&taskSum](auto p_It1, auto p_It2, const usize) {
            u32 sum = 0;
            for (auto it = p_It1; it != p_It2; ++it)
                sum += it->Value;

            taskSum.fetch_add(sum, std::memory_order_relaxed);
        });
        pool.AwaitPendingTasks();
        REQUIRE(taskSum.load(std::memory_order_relaxed) == realSum);
    }

    SECTION("Parallel for with main thread")
    {
        Array<Number, amount> numbers;
        u32 realSum = 0;
        for (u32 i = 0; i < amount; i++)
        {
            numbers[i].Value = i;
            realSum += i;
        }

        Array<Ref<Task<u32>>, threadCount - 1> tasks;
        u32 finalSum;
        ForEachMainThreadLead(pool, numbers.begin(), numbers.end(), tasks.begin(), &finalSum, threadCount,
                              [](auto p_It1, auto p_It2, const usize) {
                                  u32 sum = 0;
                                  for (auto it = p_It1; it != p_It2; ++it)
                                      sum += it->Value;
                                  return sum;
                              });
        for (auto &task : tasks)
            finalSum += task->WaitForResult();
        REQUIRE(finalSum == realSum);
    }

    SECTION("Parallel for with main thread (void return)")
    {
        Array<Number, amount> numbers;
        u32 realSum = 0;
        for (u32 i = 0; i < amount; i++)
        {
            numbers[i].Value = i;
            realSum += i;
        }

        std::atomic<u32> taskSum = 0;
        ForEachMainThreadLead(pool, numbers.begin(), numbers.end(), threadCount,
                              [&taskSum](auto p_It1, auto p_It2, const usize) {
                                  u32 sum = 0;
                                  for (auto it = p_It1; it != p_It2; ++it)
                                      sum += it->Value;

                                  taskSum.fetch_add(sum, std::memory_order_relaxed);
                              });
        pool.AwaitPendingTasks();
        REQUIRE(taskSum.load(std::memory_order_relaxed) == realSum);
    }
}

TEST_CASE("ThreadPool (std::mutex)", "[multiprocessing]")
{
    RunThreadPoolTest<std::mutex>();
}

TEST_CASE("ThreadPool (SpinLock)", "[multiprocessing]")
{
    RunThreadPoolTest<SpinLock>();
}
} // namespace TKit