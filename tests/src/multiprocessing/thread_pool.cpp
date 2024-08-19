#include "kit/multiprocessing/thread_pool.hpp"
#include "kit/multiprocessing/foreach.hpp"
#include "kit/multiprocessing/spin_mutex.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>

KIT_NAMESPACE_BEGIN

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

    SECTION("Parallel for")
    {
        struct Number
        {
            u32 Value;
            std::byte Padding[KIT_CACHE_LINE_SIZE - sizeof(u32)];
        };

        std::array<Number, amount> numbers;
        u32 realSum = 0;
        for (u32 i = 0; i < amount; i++)
        {
            numbers[i].Value = i;
            realSum += i;
        }

        std::array<Ref<Task<u32>>, threadCount> tasks;
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
}

TEST_CASE("ThreadPool (std::mutex)", "[multiprocessing]")
{
    RunThreadPoolTest<std::mutex>();
}

TEST_CASE("ThreadPool (SpinMutex)", "[multiprocessing]")
{
    RunThreadPoolTest<SpinMutex>();
}

KIT_NAMESPACE_END