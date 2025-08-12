#include "tkit/multiprocessing/chase_lev_deque.hpp" // adjust path if needed
#include <catch2/catch_test_macros.hpp>

#include <thread>

using namespace TKit;
using namespace TKit::Alias;
// simple payload
struct DTask
{
    u32 Value{0};
    DTask() = default;
    explicit DTask(u32 v) : Value(v)
    {
    }
    bool operator==(const DTask &o) const noexcept
    {
        return Value == o.Value;
    }
};

TEST_CASE("ChaseLevDeque: single-thread owner push/pop back", "[ChaseLevDeque]")
{
    constexpr u64 cap = 16;
    ChaseLevDeque<DTask, cap> q;

    for (u32 i = 0; i < 8; ++i)
        q.PushBack(i);
    for (u32 i = 0; i < 8; ++i)
    {
        auto item = q.PopBack();
        REQUIRE(item);
        REQUIRE(item->Value == 7 - i);
    }
    REQUIRE_FALSE(q.PopBack());
    REQUIRE_FALSE(q.PopFront());
}

TEST_CASE("ChaseLevDeque: uniqueness", "[ChaseLevDeque][uniqueness]")
{
    ChaseLevDeque<DTask, 1> q;
    q.PushBack(3u);

    std::atomic<u32> winners{0};
    std::vector<std::thread> stealers{};
    for (u32 i = 0; i < 4; ++i)
        stealers.emplace_back([&]() {
            if (q.PopFront())
                winners.fetch_add(1, std::memory_order_relaxed);
        });

    for (std::thread &t : stealers)
        t.join();

    REQUIRE(winners.load(std::memory_order_relaxed) == 1);
}

static void sort(std::vector<DTask> &p_Vector)
{
    std::sort(p_Vector.begin(), p_Vector.end(), [](const DTask &a, const DTask &b) { return a.Value < b.Value; });
}

TEST_CASE("ChaseLevDeque: many thieves steal while owner pushes", "[ChaseLevDeque][wrap]")
{
    // small capacity to force frequent index wrap
    constexpr u64 cap = 32;
    constexpr u32 total = 3000;
    constexpr u32 thieves = 4;

    ChaseLevDeque<DTask, cap> q;

    std::atomic<bool> run{true};
    std::atomic<u32> remaining{cap};

    std::vector<std::vector<DTask>> stolen(thieves);
    std::vector<std::thread> ts;

    ts.reserve(thieves);

    for (u32 t = 0; t < thieves; ++t)
    {
        ts.emplace_back([&, t] {
            while (run.load(std::memory_order_relaxed))
                if (auto it = q.PopFront())
                {
                    stolen[t].push_back(*it);
                    remaining.fetch_add(1, std::memory_order_relaxed);
                }
                else
                    std::this_thread::yield();
        });
    }

    // owner pushes in bursts to keep pressure on wrap-around
    u32 pushed = 0;
    while (pushed < total)
    {
        const u32 r = remaining.load(std::memory_order_relaxed);
        const u32 burst = std::min<u32>(r, total - pushed);
        remaining.fetch_sub(burst, std::memory_order_relaxed);

        for (u32 k = 0; k < burst; ++k)
            q.PushBack(pushed + k);

        pushed += burst;
        std::this_thread::yield();
    }

    std::vector<DTask> all;
    for (;;)
        if (const auto val = q.PopBack())
            all.push_back(*val);
        else
            break;

    run.store(false, std::memory_order_relaxed);
    for (auto &th : ts)
        th.join();

    for (auto &v : stolen)
        all.insert(all.end(), v.begin(), v.end());

    REQUIRE(all.size() == total);

    sort(all);
    for (u32 i = 0; i < total; ++i)
        REQUIRE(all[i].Value == i);
}
