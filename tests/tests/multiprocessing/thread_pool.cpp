#include "tkit/multiprocessing/task.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <array>

using namespace TKit;

TEST_CASE("ThreadPool executes Task<void>s", "[ThreadPool]")
{
    constexpr usize threadCount = 4;
    constexpr usize taskCount = 10;
    ThreadPool pool(threadCount);

    std::atomic<usize> counter{0};
    std::array<Task<>, taskCount> tasks;

    // Submit several void tasks
    for (usize i = 0; i < taskCount; ++i)
    {
        tasks[i] = [&] { counter.fetch_add(1, std::memory_order_relaxed); };
        pool.SubmitTask(&tasks[i]); // implicitly converts to Ref<ITask>
    }

    for (usize i = 0; i < taskCount; ++i)
        tasks[i].WaitUntilFinished();

    REQUIRE(counter.load(std::memory_order_relaxed) == taskCount);
}

TEST_CASE("ThreadPool executes Task<usize>s and preserves results", "[ThreadPool]")
{
    constexpr usize threadCount = 3;
    constexpr usize taskCount = 6;
    ThreadPool pool(threadCount);

    std::array<Task<usize>, taskCount> tasks;

    // Submit tasks that encode (taskIndex * 10 + threadIndex)
    for (usize i = 0; i < taskCount; ++i)
    {
        tasks[i] = [i] {
            thread_local const usize idx = ITaskManager::GetThreadIndex();
            return i * 10 + idx;
        };
        pool.SubmitTask(&tasks[i]);
    }

    for (usize i = 0; i < taskCount; ++i)
    {
        const usize res = tasks[i].WaitForResult();
        REQUIRE(res / 10 == i);           // correct task index
        REQUIRE(res % 10 >= 1);           // valid thread index
        REQUIRE(res % 10 <= threadCount); // valid thread index
    }
}
