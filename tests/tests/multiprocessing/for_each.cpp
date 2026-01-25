#include "tkit/multiprocessing/for_each.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <array>

using namespace TKit;
using namespace TKit::Alias;

static ArenaAllocator s_Alloc{16_kib, TKIT_CACHE_LINE_SIZE};

TEST_CASE("NonBlockingForEach (void) with ThreadPool sums all elements", "[NonBlockingForEach][ThreadPool]")
{
    ThreadPool pool(&s_Alloc, 4);
    constexpr usize firstIndex = 0;
    constexpr usize lastIndex = 100;
    constexpr usize partitionCount = 5;
    std::atomic<usize> totalSum{0};

    // Partition [0,100) into 5 chunks; each chunk adds its length to totalSum
    std::array<Task<>, partitionCount> tasks{};

    NonBlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount,
                       [&](const usize start, const usize end) {
                           totalSum.fetch_add(end - start, std::memory_order_relaxed);
                       });

    for (const Task<> &task : tasks)
        pool.WaitUntilFinished(task);

    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}

TEST_CASE("NonBlockingForEach with output iterator collects and executes all", "[NonBlockingForEach][ThreadPool]")
{
    ThreadPool pool(&s_Alloc, 3);
    constexpr usize firstIndex = 10;
    constexpr usize lastIndex = 25;
    constexpr usize partitionCount = 5;
    std::atomic<usize> totalSum{0};

    std::array<Task<>, partitionCount> tasks{};

    // Partition [10,25) into 5 chunks; capture tasks and sum chunk sizes
    NonBlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount,
                       [&](const usize start, const usize end) {
                           totalSum.fetch_add(end - start, std::memory_order_relaxed);
                       });

    // we should have one task per partition
    REQUIRE(static_cast<usize>(tasks.size()) == partitionCount);

    // wait for each task to finish
    for (const Task<> &task : tasks)
        pool.WaitUntilFinished(task);

    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}

TEST_CASE("BlockingForEach with output iterator partitions and returns main result", "[BlockingForEach][ThreadPool]")
{
    ThreadPool pool(&s_Alloc, 3);
    const usize firstIndex = 0;
    const usize lastIndex = 100;
    const usize partitionCount = 4;
    std::atomic<usize> otherSum{0};
    std::array<Task<usize>, partitionCount - 1> tasks{};

    // Callable returns usize length for main partition; others add to otherSum
    const auto callable = [&](const usize start, const usize end) -> usize {
        if (start != firstIndex)
        {
            otherSum.fetch_add(end - start, std::memory_order_relaxed);
            return 0;
        }
        return end - start;
    };

    const usize mainLength = BlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount, callable);

    // main partition [0,25) length = 25
    REQUIRE(mainLength == (lastIndex - firstIndex) / partitionCount);
    // should have enqueued partitionCount-1 tasks
    REQUIRE(static_cast<usize>(tasks.size()) == partitionCount - 1);

    // wait all other partitions
    for (const Task<usize> &task : tasks)
        pool.WaitUntilFinished(task);

    // sum of other partitions = total length minus mainLength
    REQUIRE(otherSum.load(std::memory_order_relaxed) == (lastIndex - firstIndex) - mainLength);
}

TEST_CASE("BlockingForEach without output iterator executes all partitions", "[BlockingForEach][ThreadPool]")
{
    ThreadPool pool(&s_Alloc, 2);
    const usize firstIndex = 10;
    const usize lastIndex = 30;
    const usize partitionCount = 5;
    std::atomic<usize> totalSum{0};
    std::array<Task<>, partitionCount - 1> tasks{};

    // Callable adds length of each range to totalSum
    const auto callable = [&](const usize start, const usize end) {
        totalSum.fetch_add(end - start, std::memory_order_relaxed);
    };

    // This overload does not return a value
    BlockingForEach(pool, firstIndex, lastIndex, tasks.begin(), partitionCount, callable);

    for (const Task<> &task : tasks)
        pool.WaitUntilFinished(task);

    // entire range length = lastIndex - firstIndex
    REQUIRE(totalSum.load(std::memory_order_relaxed) == lastIndex - firstIndex);
}
