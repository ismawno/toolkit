#include "tkit/container/dynamic_array.hpp"
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
struct DTrackable
{
    u32 Value;
    DTrackable() : Value(0)
    {
        ++g_Constructions;
    }
    DTrackable(const u32 p_Value) : Value(p_Value)
    {
        ++g_Constructions;
    }
    DTrackable(const DTrackable &p_Other) : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    DTrackable(DTrackable &&p_Other) : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    ~DTrackable()
    {
        ++g_Destructions;
    }
    DTrackable &operator=(const DTrackable &p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }
    DTrackable &operator=(DTrackable &&p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }
};

TEST_CASE("DynamicArray: default, Append, Pop", "[DynamicArray]")
{
    DynamicArray<u32> arr;
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

TEST_CASE("DynamicArray: constructor from size+fill args", "[DynamicArray]")
{
    const DynamicArray<u32> arr1(4);
    REQUIRE(arr1.GetSize() == 4);

    g_Constructions = g_Destructions = 0;
    const DynamicArray<DTrackable> arr2(2, 42u);
    REQUIRE(arr2.GetSize() == 2);
    REQUIRE(g_Constructions == 2);
    for (u32 i = 0; i < 2; ++i)
        REQUIRE(arr2[i].Value == 42u);
}

TEST_CASE("DynamicArray: range and initializer_list constructors", "[DynamicArray]")
{
    const DynamicArray<u32> src = {10, 20, 30};
    const DynamicArray<u32> arr1(src.begin(), src.end());
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(std::equal(arr1.begin(), arr1.end(), src.begin()));

    const DynamicArray<u32> arr2{5u, 6u, 7u};
    REQUIRE(arr2.GetSize() == 3);
    REQUIRE(arr2[1] == 6u);
}

TEST_CASE("DynamicArray: copy/move ctor and assignment", "[DynamicArray]")
{
    const DynamicArray<u32> arr1{1u, 2u, 3u};
    DynamicArray<u32> arr2 = arr1;
    REQUIRE(arr2.GetSize() == 3);
    REQUIRE(std::equal(arr2.begin(), arr2.end(), arr1.begin()));

    const DynamicArray<u32> arr3 = std::move(arr2);
    REQUIRE(arr3.GetSize() == 3);
    REQUIRE(arr3[0] == 1u);

    DynamicArray<u32> arr4;
    arr4 = arr3;
    REQUIRE(arr4.GetSize() == 3);
    REQUIRE(arr4[1] == 2u);

    DynamicArray<u32> arr5;
    arr5 = std::move(arr4);
    REQUIRE(arr5.GetSize() == 3);
    REQUIRE(arr5[2] == 3u);
}

TEST_CASE("DynamicArray: Insert single and range", "[DynamicArray]")
{
    DynamicArray<u32> arr{1u, 2u, 4u, 5u};
    arr.Insert(arr.begin() + 2, 3u);
    REQUIRE(arr.GetSize() == 5);

    const std::array<u32, 2> extra = {7u, 8u};
    arr.Insert(arr.begin() + 5, extra.begin(), extra.end());
    REQUIRE(arr.GetSize() == 7);
    REQUIRE(arr[5] == 7u);
    REQUIRE(arr[6] == 8u);
}

TEST_CASE("DynamicArray: RemoveOrdered and RemoveUnordered", "[DynamicArray]")
{
    DynamicArray<u32> arr{10u, 20u, 30u, 40u, 50u};
    arr.RemoveOrdered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 4);

    arr.RemoveOrdered(arr.begin() + 1, arr.begin() + 3);
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(arr[0] == 10u);
    REQUIRE(arr[1] == 50u);

    arr = DynamicArray<u32>{1u, 2u, 3u, 4u};
    arr.RemoveUnordered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(arr[1] != 2u);
}

TEST_CASE("DynamicArray: Resize, Clear, Shrink, iteration", "[DynamicArray]")
{
    DynamicArray<DTrackable> arr1;
    g_Constructions = g_Destructions = 0;

    arr1.Resize(3);
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(g_Constructions == 3);

    arr1.Resize(1);
    REQUIRE(arr1.GetSize() == 1);
    REQUIRE(g_Destructions == 2);

    arr1.Resize(4, 99u);
    REQUIRE(arr1.GetSize() == 4);
    for (u32 i = 1; i < 4; ++i)
        REQUIRE(arr1[i].Value == 99u);

    arr1.Clear();
    REQUIRE(arr1.IsEmpty());

    DynamicArray<u32> arr2{1u, 2u, 3u, 4u, 5u};
    arr2.Shrink();
    REQUIRE(arr2.GetSize() == 5);

    u32 sum = 0;
    for (auto &x : arr2)
        sum += x;
    REQUIRE(sum == 15u);
}

TEST_CASE("DynamicArray<std::string>: non-trivial operations", "[DynamicArray][string]")
{
    DynamicArray<std::string> arr1;
    arr1.Append("one");
    arr1.Append("two");
    arr1.Append("three");
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(arr1[1] == "two");

    auto arr2 = arr1;
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
