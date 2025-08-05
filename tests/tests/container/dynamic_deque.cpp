#include "tkit/container/dynamic_deque.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <algorithm>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

// A non-trivial type to track construction/destruction
static u32 g_Constructions = 0;
static u32 g_Destructions = 0;

struct DQTrackable
{
    u32 Value;
    DQTrackable() : Value(0)
    {
        ++g_Constructions;
    }
    DQTrackable(const u32 p_Value) : Value(p_Value)
    {
        ++g_Constructions;
    }
    DQTrackable(const DQTrackable &p_Other) : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    DQTrackable(DQTrackable &&p_Other) noexcept : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    ~DQTrackable()
    {
        ++g_Destructions;
    }

    DQTrackable &operator=(const DQTrackable &p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }

    DQTrackable &operator=(DQTrackable &&p_Other) noexcept
    {
        Value = p_Other.Value;
        return *this;
    }
};

TEST_CASE("DynamicDeque: basic operations (Push/Pop)", "[DynamicDeque]")
{
    DynamicDeque<u32> dq;
    REQUIRE(dq.GetSize() == 0);
    REQUIRE(dq.IsEmpty());

    dq.PushBack(1u);
    dq.PushFront(2u);
    dq.PushBack(3u);
    REQUIRE(dq.GetSize() == 3);

    REQUIRE(dq.GetFront() == 2u);
    REQUIRE(dq.GetBack() == 3u);

    dq.PopFront();
    REQUIRE(dq.GetFront() == 1u);

    dq.PopBack();
    REQUIRE(dq.GetBack() == 1u);
    REQUIRE(dq.GetSize() == 1);
}

TEST_CASE("DynamicDeque: copy/move constructors and assignments", "[DynamicDeque]")
{
    DynamicDeque<u32> dq1;
    dq1.PushBack(5u);
    dq1.PushBack(10u);

    DynamicDeque<u32> dq2(dq1);
    REQUIRE(dq2.GetSize() == 2);
    REQUIRE(dq2[0] == 5u);
    REQUIRE(dq2[1] == 10u);

    DynamicDeque<u32> dq3(std::move(dq1));
    REQUIRE(dq3.GetSize() == 2);
    REQUIRE(dq3[0] == 5u);
    REQUIRE(dq1.IsEmpty());

    DynamicDeque<u32> dq4;
    dq4 = dq2;
    REQUIRE(dq4.GetSize() == 2);
    REQUIRE(dq4[1] == 10u);

    DynamicDeque<u32> dq5;
    dq5 = std::move(dq4);
    REQUIRE(dq5.GetSize() == 2);
    REQUIRE(dq4.IsEmpty());
}

TEST_CASE("DynamicDeque: indexing operator and At", "[DynamicDeque]")
{
    DynamicDeque<u32> dq;
    dq.PushBack(100u);
    dq.PushBack(200u);
    dq.PushBack(300u);

    REQUIRE(dq[0] == 100u);
    REQUIRE(dq.At(1) == 200u);
    REQUIRE(dq[2] == 300u);
}

TEST_CASE("DynamicDeque: object lifetime (DQTrackable)", "[DynamicDeque]")
{
    DynamicDeque<DQTrackable> dq;
    g_Constructions = g_Destructions = 0;

    dq.PushBack(42u);
    dq.PushFront(84u);
    REQUIRE(g_Constructions == 2);
    REQUIRE(dq.GetSize() == 2);

    dq.PopFront();
    REQUIRE(g_Destructions == 1);

    dq.PopBack();
    REQUIRE(g_Destructions == 2);
    REQUIRE(dq.IsEmpty());
}

TEST_CASE("DynamicDeque: clear operation", "[DynamicDeque]")
{
    DynamicDeque<u32> dq;
    dq.PushBack(1u);
    dq.PushBack(2u);
    dq.Clear();

    REQUIRE(dq.GetSize() == 0);
    REQUIRE(dq.IsEmpty());
}

TEST_CASE("DynamicDeque: wrapping around behavior", "[DynamicDeque]")
{
    DynamicDeque<u32> dq;
    dq.PushBack(1u);
    dq.PushBack(2u);
    dq.PopFront();
    dq.PushBack(3u);
    dq.PushBack(4u);

    REQUIRE(dq.GetSize() == 3);
    REQUIRE(dq.GetFront() == 2u);
    REQUIRE(dq.GetBack() == 4u);
}

TEST_CASE("DynamicDeque: growth beyond initial capacity", "[DynamicDeque]")
{
    DynamicDeque<u32> dq;
    for (u32 i = 0u; i < 20u; ++i)
        dq.PushBack(i);

    REQUIRE(dq.GetSize() == 20u);
    for (u32 i = 0u; i < 20u; ++i)
        REQUIRE(dq[i] == i);
}

TEST_CASE("DynamicDeque<std::string>: non-trivial operations", "[DynamicDeque][string]")
{
    DynamicDeque<std::string> dq;
    dq.PushBack("first");
    dq.PushFront("zero");
    dq.PushBack("second");

    REQUIRE(dq.GetFront() == "zero");
    REQUIRE(dq[1] == "first");
    REQUIRE(dq.GetBack() == "second");

    dq.PopFront();
    REQUIRE(dq.GetFront() == "first");

    dq.PopBack();
    REQUIRE(dq.GetBack() == "first");
    REQUIRE(dq.GetSize() == 1);
}

TEST_CASE("DynamicDeque: iteration using indices", "[DynamicDeque]")
{
    DynamicDeque<u32> dq;
    dq.PushBack(10u);
    dq.PushBack(20u);
    dq.PushBack(30u);

    u32 sum = 0u;
    for (u32 i = dq.GetFrontIndex(), count = 0u; count < dq.GetSize(); i = dq.NextIndex(i), ++count)
        sum += dq.At(i);

    REQUIRE(sum == 60u);
}

TEST_CASE("DynamicDeque: large number of Push/Pop operations", "[DynamicDeque]")
{
    DynamicDeque<u32> dq;

    for (u32 i = 0u; i < 1000u; ++i)
        dq.PushBack(i);

    REQUIRE(dq.GetSize() == 1000u);

    for (u32 i = 0u; i < 500u; ++i)
        dq.PopFront();

    REQUIRE(dq.GetSize() == 500u);
    REQUIRE(dq.GetFront() == 500u);
    REQUIRE(dq.GetBack() == 999u);
}
