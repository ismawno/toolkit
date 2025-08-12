#include "tkit/multiprocessing/for_each.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/memory/ptr.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <vector>

using namespace TKit;
using namespace TKit::Alias;

TEST_CASE("NonBlockingForEach (void) with ThreadPool sums all elements", "[NonBlockingForEach][ThreadPool]")
{
    ThreadPool pool(4);
    constexpr usize firstIndex = 0;
    constexpr usize lastIndex = 100;
    constexpr usize partitionCount = 5;
    std::atomic<usize> totalSum{0};

    // Partition [0,100) into 5 chunks; each chunk adds its length to totalSum
    using Task = Ref<Task<void>>;
    Array<Task, partitionCount> tasks{};
    NonBlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount,
                       [&](const usize p_Start, const usize p_End) {
                           totalSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
                       });

    for (usize i = 0; i < partitionCount; ++i)
        tasks[i]->WaitUntilFinished();
    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}

TEST_CASE("NonBlockingForEach with output iterator collects and executes all", "[NonBlockingForEach][ThreadPool]")
{
    ThreadPool pool(3);
    constexpr usize firstIndex = 10;
    constexpr usize lastIndex = 25;
    constexpr usize partitionCount = 5;
    std::atomic<usize> totalSum{0};

    using Task = Ref<Task<void>>;
    std::vector<Task> tasks;
    tasks.resize(partitionCount);

    // Partition [10,25) into 5 chunks; capture tasks and sum chunk sizes
    NonBlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount,
                       [&](const usize p_Start, const usize p_End) {
                           totalSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
                       });

    // we should have one task per partition
    REQUIRE(static_cast<usize>(tasks.size()) == partitionCount);

    // wait for each task to finish
    for (const auto &t : tasks)
        t->WaitUntilFinished();

    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}

TEST_CASE("BlockingForEach with output iterator partitions and returns main result", "[BlockingForEach][ThreadPool]")
{
    ThreadPool pool(3);
    const usize firstIndex = 0;
    const usize lastIndex = 100;
    const usize partitionCount = 4;
    std::atomic<usize> otherSum{0};
    std::vector<Ref<ITask>> tasks;
    tasks.resize(partitionCount - 1);

    // Callable returns usize length for main partition; others add to otherSum
    const auto callable = [&](const usize p_Start, const usize p_End) -> usize {
        if (p_Start != firstIndex)
        {
            otherSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
            return 0;
        }
        return p_End - p_Start;
    };

    const usize mainLength = BlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount, callable);

    // main partition [0,25) length = 25
    REQUIRE(mainLength == (lastIndex - firstIndex) / partitionCount);
    // should have enqueued partitionCount-1 tasks
    REQUIRE(static_cast<usize>(tasks.size()) == partitionCount - 1);

    // wait all other partitions
    for (const auto &t : tasks)
        t->WaitUntilFinished();
    pool.AwaitPendingTasks();

    // sum of other partitions = total length minus mainLength
    REQUIRE(otherSum.load(std::memory_order_relaxed) == (lastIndex - firstIndex) - mainLength);
}

TEST_CASE("BlockingForEach without output iterator executes all partitions", "[BlockingForEach][ThreadPool]")
{
    ThreadPool pool(2);
    const usize firstIndex = 10;
    const usize lastIndex = 30;
    const usize partitionCount = 5;
    std::atomic<usize> totalSum{0};
    using Task = Ref<Task<void>>;
    Array<Task, partitionCount - 1> tasks{};

    // Callable adds length of each range to totalSum
    const auto callable = [&](const usize p_Start, const usize p_End) {
        totalSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
    };

    // This overload does not return a value
    BlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount, callable);

    for (usize i = 0; i < partitionCount - 1; ++i)
        tasks[i]->WaitUntilFinished();

    // entire range length = lastIndex - firstIndex
    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}
