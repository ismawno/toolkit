#include "tkit/multiprocessing/mpmc_stack.hpp" // adjust path if needed
#include "tkit/utils/alias.hpp"
#include <catch2/catch_test_macros.hpp>

#include <thread>

using namespace TKit;
using namespace TKit::Alias;
// simple payload
struct STTask
{
    u32 Value{0};
    STTask() = default;
    explicit STTask(u32 v) : Value(v)
    {
    }
    bool operator==(const STTask &o) const
    {
        return Value == o.Value;
    }
};

static std::atomic<u32> g_Constructions{0};
static std::atomic<u32> g_Destructions{0};
struct STTrackable
{
    u32 Value;
    STTrackable() : Value(0)
    {
        g_Constructions.fetch_add(1, std::memory_order_relaxed);
    }
    STTrackable(const u32 p_Value) : Value(p_Value)
    {
        g_Constructions.fetch_add(1, std::memory_order_relaxed);
    }
    STTrackable(const STTrackable &p_Other) : Value(p_Other.Value)
    {
        g_Constructions.fetch_add(1, std::memory_order_relaxed);
    }
    STTrackable(STTrackable &&p_Other) : Value(p_Other.Value)
    {
        g_Constructions.fetch_add(1, std::memory_order_relaxed);
    }
    ~STTrackable()
    {
        g_Destructions.fetch_add(1, std::memory_order_relaxed);
    }
    STTrackable &operator=(const STTrackable &p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }
    STTrackable &operator=(STTrackable &&p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }
};

TEST_CASE("MpmcStack: single-thread owner push/claim/recycle", "[MpmcStack]")
{
    g_Constructions.store(0, std::memory_order_relaxed);
    g_Destructions.store(0, std::memory_order_relaxed);

    {
        MpmcStack<STTrackable> stack{};
        constexpr u32 elements = 1000;
        using Node = MpmcStack<STTrackable>::Node;

        for (u32 i = 0; i < elements; ++i)
            stack.Push(i);

        REQUIRE(g_Constructions.load(std::memory_order_relaxed) == elements);
        REQUIRE(g_Destructions.load(std::memory_order_relaxed) == 0);

        Node *nodes = stack.Acquire();
        const Node *node = nodes;

        for (u32 i = 0; i < elements; ++i)
        {
            REQUIRE(node);
            REQUIRE(node->Value.Value == elements - 1 - i);
            node = node->Next;
        }
        REQUIRE(!node);

        stack.Reclaim(nodes);
    }

    REQUIRE(g_Destructions.load(std::memory_order_relaxed) == g_Constructions.load(std::memory_order_relaxed));
}
TEST_CASE("MpmcStack: single-thread owner push many/claim/recycle", "[MpmcStack]")
{
    g_Constructions.store(0, std::memory_order_relaxed);
    g_Destructions.store(0, std::memory_order_relaxed);

    {
        MpmcStack<STTrackable> stack{};
        constexpr u32 elements = 1000;
        using Node = MpmcStack<STTrackable>::Node;

        Node *head = stack.CreateNode(0u);
        Node *tail = head;
        for (u32 i = 0; i < elements - 1; ++i)
        {
            Node *prev = tail;
            tail = stack.CreateNode(i + 1);
            prev->Next = tail;
        }

        stack.Push(head, tail);
        REQUIRE(g_Constructions.load(std::memory_order_relaxed) == elements);

        Node *nodes = stack.Acquire();
        const Node *node = nodes;

        for (u32 i = 0; i < elements; ++i)
        {
            REQUIRE(node);
            REQUIRE(node->Value.Value == i);
            node = node->Next;
        }
        REQUIRE(!node);

        stack.Reclaim(nodes);
    }

    REQUIRE(g_Destructions.load(std::memory_order_relaxed) == g_Constructions.load(std::memory_order_relaxed));
}

TEST_CASE("MpmcStack: multi-thread owner push/claim/recycle", "[MpmcStack]")
{
    g_Constructions.store(0, std::memory_order_relaxed);
    g_Destructions.store(0, std::memory_order_relaxed);

    MpmcStack<STTrackable> stack{};
    constexpr u32 elements = 1000;
    constexpr u32 threads = 4;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    using Node = MpmcStack<STTrackable>::Node;

    std::atomic<u32> signal{0};
    std::atomic<u32> sum{0};
    for (u32 i = 0; i < threads; ++i)
    {
        producers.emplace_back([&, i] {
            for (u32 j = 0; j < elements; ++j)
                stack.Push(i * elements + j);

            signal.fetch_add(1, std::memory_order_release);
        });

        consumers.emplace_back([&] {
            for (;;)
            {
                const u32 s = signal.load(std::memory_order_acquire);
                const Node *nodes = stack.Acquire();
                while (nodes)
                {
                    const Node *node = nodes;
                    nodes = node->Next;

                    sum.fetch_add(node->Value.Value + 1, std::memory_order_relaxed);
                    stack.DestroyNode(node);
                }
                if (s == threads)
                    break;
            }
        });
    }
    for (u32 i = 0; i < threads; ++i)
    {
        producers[i].join();
        consumers[i].join();
    }

    constexpr u32 total = threads * elements;
    REQUIRE(sum.load(std::memory_order_relaxed) == (total * (total + 1)) / 2);
    REQUIRE(g_Constructions.load(std::memory_order_relaxed) == total);
    REQUIRE(g_Destructions.load(std::memory_order_relaxed) == g_Constructions.load(std::memory_order_relaxed));
}

TEST_CASE("MpmcStack: multi-thread owner many push/claim/recycle", "[MpmcStack]")
{
    g_Constructions.store(0, std::memory_order_relaxed);
    g_Destructions.store(0, std::memory_order_relaxed);

    MpmcStack<STTrackable> stack{};
    constexpr u32 elements = 1000;
    constexpr u32 threads = 4;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    using Node = MpmcStack<STTrackable>::Node;

    std::atomic<u32> signal{0};
    std::atomic<u32> sum{0};
    for (u32 i = 0; i < threads; ++i)
    {
        producers.emplace_back([&, i] {
            Node *head = stack.CreateNode(i * elements);
            Node *tail = head;
            for (u32 j = 0; j < elements - 1; ++j)
            {
                Node *prev = tail;
                tail = stack.CreateNode(i * elements + j + 1);
                prev->Next = tail;
            }

            stack.Push(head, tail);
            signal.fetch_add(1, std::memory_order_release);
        });

        consumers.emplace_back([&] {
            for (;;)
            {
                const u32 s = signal.load(std::memory_order_acquire);
                const Node *nodes = stack.Acquire();
                while (nodes)
                {
                    const Node *node = nodes;
                    nodes = node->Next;

                    sum.fetch_add(node->Value.Value + 1, std::memory_order_relaxed);
                    stack.DestroyNode(node);
                }
                if (s == threads)
                    break;
            }
        });
    }
    for (u32 i = 0; i < threads; ++i)
    {
        producers[i].join();
        consumers[i].join();
    }

    constexpr u32 total = threads * elements;
    REQUIRE(sum.load(std::memory_order_relaxed) == (total * (total + 1)) / 2);
    REQUIRE(g_Constructions.load(std::memory_order_relaxed) == total);
    REQUIRE(g_Destructions.load(std::memory_order_relaxed) == g_Constructions.load(std::memory_order_relaxed));
}
