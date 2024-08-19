#include "kit/multiprocessing/thread_pool.hpp"
#include <catch2/catch_test_macros.hpp>

KIT_NAMESPACE_BEGIN

TEST_CASE("ThreadPool", "[multiprocessing]")
{
    constexpr u32 threadCount = 4;
    constexpr u32 amount = 1000;
    ThreadPool pool(threadCount);

    SECTION("SubmitTask")
    {
        struct Number
        {
            u32 Value;
            std::byte Padding[KIT_CACHE_LINE_SIZE - sizeof(u32)];
        };
        for (u32 i = 0; i < amount; i++)
        {
            const auto task1 = pool.CreateAndSubmit([](const usz) { return 7 + 1; });
            const auto task2 = pool.CreateAndSubmit([](const usz) { return 9 + 11; });
            REQUIRE(task1->WaitForResult() == 8);
            REQUIRE(task2->WaitForResult() == 20);
        }
    }
}

KIT_NAMESPACE_END