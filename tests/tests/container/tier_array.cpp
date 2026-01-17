#include "tkit/container/tier_array.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <algorithm>
#include <array>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

// A non-trivial type to track construction/destruction
static u32 g_Constructions = 0;
static u32 g_Destructions = 0;
static TierAllocator s_Alloc{16_kib};
struct TTrackable
{
    u32 Value;
    TTrackable() : Value(0)
    {
        ++g_Constructions;
    }
    TTrackable(const u32 p_Value) : Value(p_Value)
    {
        ++g_Constructions;
    }
    TTrackable(const TTrackable &p_Other) : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    TTrackable(TTrackable &&p_Other) : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    ~TTrackable()
    {
        ++g_Destructions;
    }
    TTrackable &operator=(const TTrackable &p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }
    TTrackable &operator=(TTrackable &&p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }
};

TEST_CASE("TierArray: default, Append, Pop", "[TierArray]")
{
    TierArray<u32> arr{&s_Alloc};
    REQUIRE(arr.GetSize() == 0);
    REQUIRE(arr.IsEmpty());

    arr.Append(1u) = 5;
    arr.Append(2u);
    arr.Append(3u);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(arr[0] == 5);
    REQUIRE(arr.GetBack() == 3u);

    arr.Pop();
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(arr.GetBack() == 2u);
}

TEST_CASE("TierArray: constructor from size+fill args", "[TierArray]")
{
    const TierArray<u32> arr1(4, &s_Alloc);
    REQUIRE(arr1.GetSize() == 4);

    g_Constructions = g_Destructions = 0;
    const TierArray<TTrackable> arr2(2, &s_Alloc);
    REQUIRE(arr2.GetSize() == 2);
    REQUIRE(g_Constructions == 2);
}

TEST_CASE("TierArray: range and initializer_list constructors", "[TierArray]")
{
    const TierArray<u32> src{{10, 20, 30}, &s_Alloc};
    const TierArray<u32> arr1(src.begin(), src.end(), &s_Alloc);
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(std::equal(arr1.begin(), arr1.end(), src.begin()));

    const TierArray<u32> arr2{{5u, 6u, 7u}, &s_Alloc};
    REQUIRE(arr2.GetSize() == 3);
    REQUIRE(arr2[1] == 6u);
}

TEST_CASE("TierArray: copy/move ctor and assignment", "[TierArray]")
{
    const TierArray<u32> arr1{{1u, 2u, 3u}, &s_Alloc};
    TierArray<u32> arr2 = arr1;
    REQUIRE(arr2.GetSize() == 3);
    REQUIRE(std::equal(arr2.begin(), arr2.end(), arr1.begin()));

    const TierArray<u32> arr3 = std::move(arr2);
    REQUIRE(arr3.GetSize() == 3);
    REQUIRE(arr3[0] == 1u);

    TierArray<u32> arr4{&s_Alloc};
    arr4 = arr3;
    REQUIRE(arr4.GetSize() == 3);
    REQUIRE(arr4[1] == 2u);

    TierArray<u32> arr5{&s_Alloc};
    arr5 = std::move(arr4);
    REQUIRE(arr5.GetSize() == 3);
    REQUIRE(arr5[2] == 3u);
}

TEST_CASE("TierArray: Insert single and range", "[TierArray]")
{
    TierArray<u32> arr{{1u, 2u, 4u, 5u}, &s_Alloc};
    arr.Insert(arr.begin() + 2, 3u);
    REQUIRE(arr.GetSize() == 5);

    const std::array<u32, 2> extra{7u, 8u};
    arr.Insert(arr.begin() + 5, extra.begin(), extra.end());
    REQUIRE(arr.GetSize() == 7);
    REQUIRE(arr[5] == 7u);
    REQUIRE(arr[6] == 8u);
}

TEST_CASE("TierArray: RemoveOrdered and RemoveUnordered", "[TierArray]")
{
    TierArray<u32> arr{{10u, 20u, 30u, 40u, 50u}, &s_Alloc};
    arr.RemoveOrdered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 4);

    arr.RemoveOrdered(arr.begin() + 1, arr.begin() + 3);
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(arr[0] == 10u);
    REQUIRE(arr[1] == 50u);

    arr = TierArray<u32>{{1u, 2u, 3u, 4u}, &s_Alloc};
    arr.RemoveUnordered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(arr[1] != 2u);
}

TEST_CASE("TierArray: Resize, Clear, Shrink, iteration", "[TierArray]")
{
    TierArray<TTrackable> arr1{&s_Alloc};
    g_Constructions = g_Destructions = 0;

    arr1.Resize(3);
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(g_Constructions == 3);

    arr1.Resize(1);
    REQUIRE(arr1.GetSize() == 1);
    REQUIRE(g_Destructions == 2);

    arr1.Resize(4, 99u);
    REQUIRE(arr1.GetSize() == 4);
    for (usize i = 1; i < 4; ++i)
        REQUIRE(arr1[i].Value == 99u);

    arr1.Clear();
    REQUIRE(arr1.IsEmpty());

    TierArray<u32> arr2{{1u, 2u, 3u, 4u, 5u}, &s_Alloc};
    arr2.Shrink();
    REQUIRE(arr2.GetSize() == 5);

    u32 sum = 0;
    for (auto &x : arr2)
        sum += x;
    REQUIRE(sum == 15u);
}

TEST_CASE("TierArray<std::string>: non-trivial operations", "[TierArray][string]")
{
    TierArray<std::string> arr1{&s_Alloc};
    arr1.Append("one");
    arr1.Append("two");
    arr1.Append("three");
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(arr1[1] == "two");

    TierArray<std::string> arr2{arr1};
    arr2[1] = "TWO";
    REQUIRE(arr1[1] == "two");
    REQUIRE(arr2[1] == "TWO");

    const auto arr3 = std::move(arr2);
    REQUIRE(arr3.GetSize() == 3);
    arr2.Clear();
    REQUIRE(arr2.GetSize() == 0);

    arr1.Insert(arr1.begin() + 1, std::string("inserted"));
    REQUIRE(arr1[1] == "inserted");

    const std::array<std::string, 2> extras{"x", "y"};
    arr1.Insert(arr1.begin() + 4, extras.begin(), extras.end());
    REQUIRE(arr1.GetBack() == "y");

    arr1.RemoveOrdered(arr1.begin() + 1);
    REQUIRE(arr1.GetSize() == 5);

    arr1.RemoveUnordered(arr1.begin() + 1);
    REQUIRE(arr1.GetSize() == 4);

    arr1.Resize(5, std::string("fill"));
    REQUIRE(arr1.GetSize() == 5);
    REQUIRE(arr1[4] == "fill");

    arr1.Pop();
    REQUIRE(arr1.GetSize() == 4);

    arr1.Clear();
    REQUIRE(arr1.IsEmpty());
}
