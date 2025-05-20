#include "tkit/multiprocessing/for_each.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/memory/ptr.hpp"
#include "tkit/container/dynamic_array.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>

using namespace TKit;
using namespace TKit::Alias;

TEST_CASE("ForEach (void) with ThreadPool sums all elements", "[ForEach][ThreadPool]")
{
    ThreadPool pool(4);
    const usize firstIndex = 0;
    const usize lastIndex = 100;
    const usize partitionCount = 5;
    std::atomic<usize> totalSum{0};

    // Partition [0,100) into 5 chunks; each chunk adds its length to totalSum
    ForEach(pool, firstIndex, lastIndex, partitionCount,
            [&](const usize p_Start, const usize p_End, const usize /*p_ThreadIndex*/) {
                totalSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
            });

    pool.AwaitPendingTasks();
    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}

TEST_CASE("ForEach with output iterator collects and executes all", "[ForEach][ThreadPool]")
{
    ThreadPool pool(3);
    const usize firstIndex = 10;
    const usize lastIndex = 25;
    const usize partitionCount = 5;
    std::atomic<usize> totalSum{0};
    DynamicArray<Ref<ITask>> tasks;
    tasks.Resize(partitionCount);

    // Partition [10,25) into 5 chunks; capture tasks and sum chunk sizes
    ForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount,
            [&](const usize p_Start, const usize p_End, const usize /*p_ThreadIndex*/) {
                totalSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
            });

    // we should have one task per partition
    REQUIRE(tasks.GetSize() == partitionCount);

    // wait for each task to finish
    for (const auto &t : tasks)
        t->WaitUntilFinished();

    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}

TEST_CASE("ForEachMainThreadLead with output iterator partitions and returns main result",
          "[ForEachMainThreadLead][ThreadPool]")
{
    ThreadPool pool(3);
    const usize firstIndex = 0;
    const usize lastIndex = 100;
    const usize partitionCount = 4;
    std::atomic<usize> otherSum{0};
    DynamicArray<Ref<ITask>> tasks;
    tasks.Resize(partitionCount - 1);

    // Callable returns usize length for main partition; others add to otherSum
    const auto callable = [&](const usize p_Start, const usize p_End, const usize /*p_ThreadIndex*/) -> usize {
        if (p_Start != firstIndex)
        {
            otherSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
            return 0;
        }
        return p_End - p_Start;
    };

    const usize mainLength =
        ForEachMainThreadLead(pool, firstIndex, lastIndex, tasks.begin(), partitionCount, callable);

    // main partition [0,25) length = 25
    REQUIRE(mainLength == (lastIndex - firstIndex) / partitionCount);
    // should have enqueued partitionCount-1 tasks
    REQUIRE(tasks.GetSize() == partitionCount - 1);

    // wait all other partitions
    for (const auto &t : tasks)
        t->WaitUntilFinished();
    pool.AwaitPendingTasks();

    // sum of other partitions = total length minus mainLength
    REQUIRE(otherSum.load(std::memory_order_relaxed) == (lastIndex - firstIndex) - mainLength);
}

TEST_CASE("ForEachMainThreadLead without output iterator executes all partitions",
          "[ForEachMainThreadLead][ThreadPool]")
{
    ThreadPool pool(2);
    const usize firstIndex = 10;
    const usize lastIndex = 30;
    const usize partitionCount = 5;
    std::atomic<usize> totalSum{0};

    // Callable adds length of each range to totalSum
    const auto callable = [&](const usize p_Start, const usize p_End, const usize /*p_ThreadIndex*/) {
        totalSum.fetch_add(p_End - p_Start, std::memory_order_relaxed);
    };

    // This overload does not return a value
    ForEachMainThreadLead(pool, firstIndex, lastIndex, partitionCount, callable);

    pool.AwaitPendingTasks();

    // entire range length = lastIndex - firstIndex
    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}
