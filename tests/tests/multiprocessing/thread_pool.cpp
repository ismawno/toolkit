#include "tkit/memory/ptr.hpp"
#include "tkit/multiprocessing/task.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/container/dynamic_array.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>

using namespace TKit;

TEST_CASE("ThreadPool executes Task<void>s", "[ThreadPool]")
{
    constexpr usize threadCount = 4;
    constexpr usize taskCount = 10;
    ThreadPool pool(threadCount);

    std::atomic<usize> counter{0};
    DynamicArray<Ref<Task<void>>> tasks;
    tasks.Reserve(taskCount);

    // Submit several void tasks
    for (usize i = 0; i < taskCount; ++i)
    {
        const auto t = Ref<Task<void>>::Create([&]() { counter.fetch_add(1, std::memory_order_relaxed); });
        tasks.Append(t);
        pool.SubmitTask(t); // implicitly converts to Ref<ITask>
    }

    pool.AwaitPendingTasks();
    REQUIRE(counter.load(std::memory_order_relaxed) == taskCount);
}

TEST_CASE("ThreadPool executes Task<usize>s and preserves results", "[ThreadPool]")
{
    constexpr usize threadCount = 3;
    constexpr usize taskCount = 6;
    ThreadPool pool(threadCount);

    DynamicArray<Ref<Task<usize>>> tasks;
    tasks.Reserve(taskCount);

    // Submit tasks that encode (taskIndex * 10 + threadIndex)
    for (usize i = 0; i < taskCount; ++i)
    {
        const auto t = Ref<Task<usize>>::Create([i]() {
            thread_local const usize idx = ITaskManager::GetThreadIndex();
            return i * 10 + idx;
        });
        tasks.Append(t);
        pool.SubmitTask(t);
    }

    pool.AwaitPendingTasks();

    for (usize i = 0; i < taskCount; ++i)
    {
        const usize res = tasks[i]->WaitForResult();
        REQUIRE(res / 10 == i);           // correct task index
        REQUIRE(res % 10 >= 1);           // valid thread index
        REQUIRE(res % 10 <= threadCount); // valid thread index
    }
}
