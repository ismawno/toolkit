#include "tkit/multiprocessing/task.hpp"
#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <chrono>

using namespace TKit;

TEST_CASE("Task<T> basic behavior", "[Task]")
{
    // Create a Task<u32> that doubles an arbitrary number
    Task<u32> task{[](const u32 p_Index) { return u32(p_Index * 2); }, 5u};
    REQUIRE(!task.IsFinished());

    // Invoke it with index 5
    task();
    REQUIRE(task.IsFinished());
    REQUIRE(task.WaitForResult() == 10);

    // Reset and ensure it can run again
    task.Reset();
    REQUIRE(!task.IsFinished());
    task();
    REQUIRE(task.IsFinished());
    REQUIRE(task.WaitForResult() == 10);
}

TEST_CASE("Task<void> basic behavior", "[Task]")
{
    usize counter = 0;
    // Create a Task<void> that adds the thread index to counter
    Task<> task{[&](const u32 p_Index) { counter += p_Index; }, 5u};
    REQUIRE(!task.IsFinished());

    task();
    REQUIRE(task.IsFinished());
    REQUIRE(counter == 5u);

    task.Reset();
    REQUIRE(!task.IsFinished());
}

TEST_CASE("Task::WaitUntilFinished blocks from another thread", "[Task]")
{
    // Create a task that sleeps then returns its index
    Task<u32> task;
    task = [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return 32u;
    };

    std::thread runner([&] { task(); });

    // This should block until runner invokes notifyCompleted()
    task.WaitUntilFinished();
    REQUIRE(task.IsFinished());
    REQUIRE(task.WaitForResult() == 32u);

    runner.join();
}
