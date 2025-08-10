#include "tkit/container/static_deque.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

// Global counters to test object lifetimes
static u32 g_Constructions = 0;
static u32 g_Destructions = 0;

struct SQTrackable
{
    u32 Value;
    SQTrackable() : Value(0)
    {
        ++g_Constructions;
    }
    SQTrackable(const u32 p_Value) : Value(p_Value)
    {
        ++g_Constructions;
    }
    SQTrackable(const SQTrackable &p_Other) : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    SQTrackable(SQTrackable &&p_Other) noexcept : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    ~SQTrackable()
    {
        ++g_Destructions;
    }

    SQTrackable &operator=(const SQTrackable &p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }

    SQTrackable &operator=(SQTrackable &&p_Other) noexcept
    {
        Value = p_Other.Value;
        return *this;
    }
};

TEST_CASE("StaticDeque: various constructors", "[StaticDeque][constructors]")
{
    // Default constructor
    {
        StaticDeque<int, 3> dq;
        REQUIRE(dq.GetSize() == 0);
        REQUIRE(dq.GetCapacity() == 3);
        REQUIRE(dq.IsEmpty());
    }

    // Size + fill value constructor
    {
        StaticDeque<int, 8> dq(3, 42);
        REQUIRE(dq.GetSize() == 3);
        REQUIRE(dq[0] == 42);
        REQUIRE(dq[1] == 42);
        REQUIRE(dq[2] == 42);

        dq.PushBack(6u);
        dq.PushFront(7u);

        REQUIRE(dq.GetBack() == 6u);
        REQUIRE(dq.GetFront() == 7u);
    }

    // Size constructor with non-trivial type
    {
        g_Constructions = g_Destructions = 0;
        StaticDeque<SQTrackable, 2> dq(2, SQTrackable{99});
        REQUIRE(dq.GetSize() == 2);
        REQUIRE(dq[0].Value == 99);
        REQUIRE(dq[1].Value == 99);
        REQUIRE(g_Constructions == 3); // 1 temp + 2 copies
    }

    // Range constructor
    {
        std::vector<int> v = {1, 2, 3};
        StaticDeque<int, 5> dq(v.begin(), v.end());
        REQUIRE(dq.GetSize() == 3);
        REQUIRE(dq[0] == 1);
        REQUIRE(dq[1] == 2);
        REQUIRE(dq[2] == 3);

        dq.PushBack(6u);
        dq.PushFront(7u);

        REQUIRE(dq.GetBack() == 6u);
        REQUIRE(dq.GetFront() == 7u);
    }

    // Initializer list constructor
    {
        StaticDeque<std::string, 4> dq({"a", "b", "c"});
        REQUIRE(dq.GetSize() == 3);
        REQUIRE(dq[0] == "a");
        REQUIRE(dq[1] == "b");
        REQUIRE(dq[2] == "c");
    }

    // Cross-capacity construction (smaller → larger)
    {
        StaticDeque<int, 2> small(2, 7);
        StaticDeque<int, 5> large(small);
        REQUIRE(large.GetSize() == 2);
        REQUIRE(large[0] == 7);
        REQUIRE(large[1] == 7);
    }
}

TEST_CASE("StaticDeque: basic operations", "[StaticDeque]")
{
    StaticDeque<u32, 4> dq;
    REQUIRE(dq.GetCapacity() == 4);
    REQUIRE(dq.GetSize() == 0);
    REQUIRE(dq.IsEmpty());
    REQUIRE_FALSE(dq.IsFull());

    dq.PushBack(1);
    REQUIRE(dq.GetFront() == 1);
    dq.PushFront(2);
    dq.PushBack(3);
    dq.PushFront(4);
    REQUIRE(dq.GetSize() == 4);
    REQUIRE(dq.IsFull());
    REQUIRE_FALSE(dq.IsEmpty());

    REQUIRE(dq.GetFront() == 4);
    REQUIRE(dq.GetBack() == 3);

    dq.PopFront();
    REQUIRE(dq.GetFront() == 2);

    dq.PopBack();
    REQUIRE(dq.GetBack() == 1);
}

TEST_CASE("StaticDeque: SQTrackable lifetime", "[StaticDeque]")
{
    StaticDeque<SQTrackable, 3> dq;
    g_Constructions = g_Destructions = 0;

    dq.PushBack(10u);
    dq.PushFront(20u);
    REQUIRE(g_Constructions == 2);
    REQUIRE(dq.GetSize() == 2);

    dq.PopBack();
    REQUIRE(g_Destructions == 1);
    REQUIRE(dq.GetSize() == 1);

    dq.PopFront();
    REQUIRE(g_Destructions == 2);
    REQUIRE(dq.GetSize() == 0);
}

TEST_CASE("StaticDeque: copy and move operations", "[StaticDeque]")
{
    StaticDeque<u32, 4> dq1;
    dq1.PushBack(1);
    dq1.PushBack(2);

    StaticDeque<u32, 4> dq2(dq1); // copy ctor
    REQUIRE(dq2.GetSize() == 2);
    REQUIRE(dq2.GetFront() == 1);
    REQUIRE(dq2.GetBack() == 2);

    StaticDeque<u32, 4> dq3(std::move(dq1)); // move ctor
    REQUIRE(dq3.GetSize() == 2);
    REQUIRE(dq3.GetFront() == 1);
    REQUIRE(dq3.GetBack() == 2);

    StaticDeque<u32, 4> dq4;
    dq4 = dq3; // copy assignment
    REQUIRE(dq4.GetSize() == 2);
    REQUIRE(dq4.GetFront() == 1);

    StaticDeque<u32, 4> dq5;
    dq5 = std::move(dq4); // move assignment
    REQUIRE(dq5.GetSize() == 2);
    REQUIRE(dq5.GetFront() == 1);
}

TEST_CASE("StaticDeque: indexing", "[StaticDeque]")
{
    StaticDeque<u32, 3> dq;
    dq.PushBack(5);
    dq.PushBack(10);
    dq.PushBack(15);

    REQUIRE(dq[0] == 5);
    REQUIRE(dq[1] == 10);
    REQUIRE(dq[2] == 15);
}

TEST_CASE("StaticDeque<std::string>: basic operations", "[StaticDeque][string]")
{
    StaticDeque<std::string, 3> dq;
    dq.PushBack("a");
    dq.PushFront("b");
    REQUIRE(dq.GetFront() == "b");
    REQUIRE(dq.GetBack() == "a");

    dq.PushBack("c");
    REQUIRE(dq.IsFull());

    dq.PopFront();
    REQUIRE(dq.GetFront() == "a");

    dq.PopBack();
    REQUIRE(dq.GetBack() == "a");
    REQUIRE(dq.GetSize() == 1);
}

TEST_CASE("StaticDeque: clear", "[StaticDeque]")
{
    StaticDeque<u32, 5> dq;
    dq.PushBack(1);
    dq.PushBack(2);
    dq.Clear();
    REQUIRE(dq.IsEmpty());
    REQUIRE(dq.GetSize() == 0);
}

TEST_CASE("StaticDeque: wrapping around", "[StaticDeque]")
{
    StaticDeque<u32, 3> dq;
    dq.PushBack(1);
    dq.PushBack(2);
    dq.PopFront();
    dq.PushBack(3);
    dq.PushBack(4); // should wrap around internally

    REQUIRE(dq.IsFull());
    REQUIRE(dq[0] == 4);
    REQUIRE(dq[1] == 2);
    REQUIRE(dq[2] == 3);
}

TEST_CASE("StaticDeque: constructors with different capacities", "[StaticDeque]")
{
    StaticDeque<u32, 2> smallDq;
    smallDq.PushBack(1);
    smallDq.PushBack(2);

    StaticDeque<u32, 4> largeDq(smallDq);
    // REQUIRE(largeDq.GetSize() == 2);
    REQUIRE(largeDq.GetFront() == 1);
    REQUIRE(largeDq.GetBack() == 2);

    largeDq.PushBack(3);
    largeDq.PushBack(4);
    REQUIRE(largeDq.IsFull());

    StaticDeque<u32, 2> smallDq2(std::move(smallDq));
    REQUIRE(smallDq2.GetSize() == 2);
    REQUIRE(smallDq2.GetFront() == 1);
}

TEST_CASE("StaticDeque: iteration using indices", "[StaticDeque]")
{
    StaticDeque<u32, 5> dq;
    dq.PushBack(1);
    dq.PushBack(2);
    dq.PushBack(3);

    u32 sum = 0;
    for (u32 i = dq.GetFrontIndex(); i != dq.GetBackEnd(); i = dq.NextIndex(i))
        sum += dq.At(i);
    REQUIRE(sum == 6);
}
