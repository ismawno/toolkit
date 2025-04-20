#include "tkit/memory/ptr.hpp"
#include "tkit/multiprocessing/task.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <vector>

using namespace TKit;

TEST_CASE("ThreadPool executes Task<void>s", "[ThreadPool]")
{
    constexpr usize THREADS = 4;
    constexpr usize TASKS = 10;
    ThreadPool pool(THREADS);

    std::atomic<usize> counter{0};
    std::vector<Ref<Task<void>>> tasks;
    tasks.reserve(TASKS);

    // Submit several void tasks
    for (usize i = 0; i < TASKS; ++i)
    {
        auto t = Ref<Task<void>>::Create([&](usize) { counter.fetch_add(1, std::memory_order_relaxed); });
        tasks.push_back(t);
        pool.SubmitTask(t); // implicitly converts to Ref<ITask>
    }

    pool.AwaitPendingTasks();
    REQUIRE(counter.load(std::memory_order_relaxed) == TASKS);
}

TEST_CASE("ThreadPool executes Task<u32>s and preserves results", "[ThreadPool]")
{
    constexpr usize THREADS = 3;
    constexpr usize TASKS = 6;
    ThreadPool pool(THREADS);

    std::vector<Ref<Task<u32>>> tasks;
    tasks.reserve(TASKS);

    // Submit tasks that encode (taskIndex * 10 + threadIndex)
    for (usize i = 0; i < TASKS; ++i)
    {
        auto t = Ref<Task<u32>>::Create([i](usize idx) { return u32(i * 10 + idx); });
        tasks.push_back(t);
        pool.SubmitTask(t);
    }

    pool.AwaitPendingTasks();

    for (usize i = 0; i < TASKS; ++i)
    {
        u32 res = tasks[i]->WaitForResult();
        REQUIRE(res / 10 == u32(i));  // correct task index
        REQUIRE(res % 10 >= 1);       // valid thread index
        REQUIRE(res % 10 <= THREADS); // valid thread index
    }
}
