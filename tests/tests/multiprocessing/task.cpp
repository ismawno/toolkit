#include "tkit/memory/ptr.hpp"
#include "tkit/multiprocessing/task.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>
#include <chrono>

using namespace TKit;

TEST_CASE("Task<T> basic behavior", "[Task]")
{
    // Create a Task<u32> that doubles the thread index
    const auto task = Ref<Task<u32>>::Create([](const usize p_Index) { return u32(p_Index * 2); });
    REQUIRE(!task->IsFinished());

    // Invoke it with index 5
    (*task)(5u);
    REQUIRE(task->IsFinished());
    REQUIRE(task->WaitForResult() == 10);

    // Reset and ensure it can run again
    task->Reset();
    REQUIRE(!task->IsFinished());
    (*task)(3u);
    REQUIRE(task->IsFinished());
    REQUIRE(task->WaitForResult() == 6);
}

TEST_CASE("Task<void> basic behavior", "[Task]")
{
    usize counter = 0;
    // Create a Task<void> that adds the thread index to counter
    auto task = Ref<Task<void>>::Create([&](const usize p_Index) { counter += p_Index; });
    REQUIRE(!task->IsFinished());

    (*task)(7u);
    REQUIRE(task->IsFinished());
    REQUIRE(counter == 7u);

    task->Reset();
    REQUIRE(!task->IsFinished());
}

TEST_CASE("Task::WaitUntilFinished blocks from another thread", "[Task]")
{
    // Create a task that sleeps then returns its index
    const auto task = Ref<Task<u32>>::Create([](const usize p_Index) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return u32(p_Index);
    });

    std::thread runner([&] { (*task)(9u); });

    // This should block until runner invokes notifyCompleted()
    task->WaitUntilFinished();
    REQUIRE(task->IsFinished());
    REQUIRE(task->WaitForResult() == 9);

    runner.join();
}
