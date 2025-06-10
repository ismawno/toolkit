#include "tkit/container/static_array.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

// A simple non‑trivial type to track construction/destruction
static u32 g_Constructions = 0;
static u32 g_Destructions = 0;
struct STrackable
{
    u32 Value;
    STrackable() : Value(0)
    {
        ++g_Constructions;
    }
    STrackable(const u32 p_Value) : Value(p_Value)
    {
        ++g_Constructions;
    }
    STrackable(const STrackable &p_Other) : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    STrackable(STrackable &&p_Other) noexcept : Value(p_Other.Value)
    {
        ++g_Constructions;
    }
    ~STrackable()
    {
        ++g_Destructions;
    }
    STrackable &operator=(const STrackable &p_Other)
    {
        Value = p_Other.Value;
        return *this;
    }
    STrackable &operator=(STrackable &&p_Other) noexcept
    {
        Value = p_Other.Value;
        return *this;
    }
};

TEST_CASE("StaticArray: basic capacity/size queries", "[StaticArray]")
{
    StaticArray<u32, 4> arr;
    REQUIRE(arr.GetCapacity() == 4);
    REQUIRE(arr.GetSize() == 0);
    REQUIRE(arr.IsEmpty());
    REQUIRE(!arr.IsFull());

    // fill to capacity
    arr.Append(10) = 15; // note: Append returns reference
    arr.Append(20);
    arr.Append(30);
    arr.Append(40);
    REQUIRE(arr.GetSize() == 4);
    REQUIRE(arr.IsFull());
    REQUIRE_FALSE(arr.IsEmpty());

    // reading via operator[]
    REQUIRE(arr[0] == 15);
    REQUIRE(arr[3] == 40);

    // GetFront/GetBack
    REQUIRE(arr.GetFront() == 15);
    REQUIRE(arr.GetBack() == 40);
}

TEST_CASE("StaticArray: Append and Pop", "[StaticArray]")
{
    StaticArray<STrackable, 3> arr;
    g_Constructions = g_Destructions = 0;

    // Append default-constructed
    auto &r0 = arr.Append(); // one default construct
    REQUIRE(arr.GetSize() == 1);
    REQUIRE(g_Constructions == 1);
    r0.Value = 7;

    // Append with args
    const auto &r1 = arr.Append(13u);
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(g_Constructions == 2);
    REQUIRE(r1.Value == 13);

    // Pop
    arr.Pop();
    REQUIRE(arr.GetSize() == 1);
    REQUIRE(g_Destructions == 1); // destroyed the STrackable(13)

    // Pop to empty
    arr.Pop();
    REQUIRE(arr.GetSize() == 0);
    REQUIRE(g_Destructions == 2);
}

TEST_CASE("StaticArray: constructor from size+fill args", "[StaticArray]")
{
    // trivially default-constructible => no actual constructions happen
    const StaticArray<u32, 5> arr(3);
    REQUIRE(arr.GetSize() == 3);

    // non-trivial => should invoke CTOR for each
    g_Constructions = g_Destructions = 0;
    const StaticArray<STrackable, 5> nt(2, 42);
    REQUIRE(nt.GetSize() == 2);
    REQUIRE(g_Constructions == 2);
    for (usize i = 0; i < 2; ++i)
        REQUIRE(nt[i].Value == 42);
    // destructor runs in ~StaticArray
}

TEST_CASE("StaticArray: initializer_list & range constructors", "[StaticArray]")
{
    // initializer_list
    const StaticArray<u32, 4> arr{5u, 6u, 7u};
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(std::equal(arr.begin(), arr.end(), TKit::Array<u32, 3>{5, 6, 7}.begin()));

    // range constructor from another container
    const TKit::Array<u32, 4> src = {10, 20, 30, 40};
    const StaticArray<u32, 4> rg(src.begin() + 1, src.begin() + 4);
    REQUIRE(rg.GetSize() == 3);
    REQUIRE(rg[0] == 20);
    REQUIRE(rg[2] == 40);
}

TEST_CASE("StaticArray: copy/move ctor and assignment", "[StaticArray]")
{
    StaticArray<u32, 4> arr1{1, 2, 3};
    const StaticArray<u32, 4> arr2 = arr1; // copy ctor
    REQUIRE(arr2.GetSize() == 3);
    REQUIRE(std::equal(arr2.begin(), arr2.end(), arr1.begin()));

    const StaticArray<u32, 4> arr3 = std::move(arr1); // move ctor
    REQUIRE(arr3.GetSize() == 3);
    REQUIRE(arr3[0] == 1);

    // copy assignment
    StaticArray<u32, 4> arr4;
    arr4 = arr3;
    REQUIRE(arr4.GetSize() == 3);
    REQUIRE(arr4[1] == 2);

    // move assignment
    StaticArray<u32, 4> arr5;
    arr5 = std::move(arr4);
    REQUIRE(arr5.GetSize() == 3);
    REQUIRE(arr5[2] == 3);
}

TEST_CASE("StaticArray: member Insert wrappers", "[StaticArray]")
{
    StaticArray<u32, 7> arr{1, 2, 4, 5};
    arr.Insert(arr.begin() + 2, 3u); // insert single at pos 2
    REQUIRE(arr.GetSize() == 5);

    // insert range
    TKit::Array<u32, 2> extra = {7, 8};
    arr.Insert(arr.begin() + 5, extra.begin(), extra.end());
    REQUIRE(arr.GetSize() == 7); // capacity 5 => insertion of 2 at end should assert; skip if asserts disabled
}

TEST_CASE("StaticArray: member RemoveOrdered/Unordered wrappers", "[StaticArray]")
{
    StaticArray<u32, 6> arr{10, 20, 30, 40, 50};
    REQUIRE(arr.GetSize() == 5);

    arr.RemoveOrdered(arr.begin() + 1); // remove '20'
    REQUIRE(arr.GetSize() == 4);

    arr.RemoveOrdered(arr.begin() + 1, arr.begin() + 3); // remove '30','40'
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 50);

    // unordered remove
    arr = StaticArray<u32, 6>{1, 2, 3, 4};
    arr.RemoveUnordered(arr.begin() + 1); // replace at idx1 with last
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(arr[1] == 4);
}

TEST_CASE("StaticArray: Resize", "[StaticArray]")
{
    StaticArray<STrackable, 5> arr;
    g_Constructions = g_Destructions = 0;

    // grow with default
    arr.Resize(3);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(g_Constructions == 3);

    // shrink
    arr.Resize(1);
    REQUIRE(arr.GetSize() == 1);
    REQUIRE(g_Destructions == 2);

    // grow with args
    arr.Resize(4, 99u);
    REQUIRE(arr.GetSize() == 4);
    REQUIRE(g_Constructions == 3 + 3); // two phases: initial 3, +3 new (from 1→4)
    for (u32 i = 1; i < 4; ++i)
        REQUIRE(arr[i].Value == 99u);
}

TEST_CASE("StaticArray: Clear and iteration", "[StaticArray]")
{
    StaticArray<u32, 4> arr1{9, 8, 7};
    arr1.Clear();
    REQUIRE(arr1.GetSize() == 0);
    REQUIRE(arr1.IsEmpty());

    // range-for
    const StaticArray<u32, 4> arr2{1, 2, 3};
    u32 sum = 0;
    for (const u32 x : arr2)
        sum += x;
    REQUIRE(sum == 6);
}

TEST_CASE("StaticArray aliases", "[StaticArray]")
{
    const StaticArray4<u32> a4;
    const StaticArray8<u32> a8;
    const StaticArray16<u32> a16;
    REQUIRE(a4.GetCapacity() == 4);
    REQUIRE(a8.GetCapacity() == 8);
    REQUIRE(a16.GetCapacity() == 16);
}

TEST_CASE("StaticArray<std::string>: basic operations", "[StaticArray][string]")
{
    StaticArray<std::string, 15> arr1;
    REQUIRE(arr1.GetSize() == 0);
    REQUIRE(arr1.IsEmpty());

    // Append & operator[]
    arr1.Append("one");
    arr1.Append("two");
    arr1.Append("three");
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(arr1[0] == "one");
    REQUIRE(arr1[1] == "two");
    REQUIRE(arr1[2] == "three");

    // Copy‐ctor isolates storage
    auto arr2 = arr1;
    REQUIRE(arr2.GetSize() == 3);
    arr2[1] = "TWO";
    REQUIRE(arr1[1] == "two"); // original unchanged
    REQUIRE(arr2[1] == "TWO");

    // Move‐ctor leaves moved‐from in valid state
    auto arr3 = std::move(arr2);
    REQUIRE(arr3.GetSize() == 3);
    REQUIRE(arr3[0] == "one");
    // arr2 is in valid but unspecified state; we at least can call Clear()
    arr2.Clear();
    REQUIRE(arr2.GetSize() == 0);

    // Insert single
    arr1.Insert(arr1.begin() + 1, std::string("inserted"));
    REQUIRE(arr1.GetSize() == 4);
    REQUIRE(arr1[1] == "inserted");
    REQUIRE(arr1[2] == "two");

    // Insert range
    const Array<std::string, 3> extras{"x", "y", "z"};
    arr1.Insert(arr1.begin() + 4, extras.begin(), extras.end());
    REQUIRE(arr1.GetSize() == 7);
    REQUIRE(arr1[4] == "x");
    REQUIRE(arr1[6] == "z");

    // RemoveOrdered single
    arr1.RemoveOrdered(arr1.begin() + 1);
    REQUIRE(arr1.GetSize() == 6);
    REQUIRE(arr1[1] == "two");

    // RemoveOrdered range
    arr1.RemoveOrdered(arr1.begin() + 2, arr1.begin() + 4); // remove two elements
    REQUIRE(arr1.GetSize() == 4);

    // RemoveUnordered
    arr1 = StaticArray<std::string, 5>{"A", "B", "C", "D"};
    arr1.RemoveUnordered(arr1.begin() + 1);
    REQUIRE(arr1.GetSize() == 3);
    // element at idx1 should be one of the originals, but not "B"
    REQUIRE(arr1[1] != "B");
    REQUIRE((arr1[1] == "D" || arr1[1] == "C" || arr1[1] == "A"));

    // Resize up with fill‐args
    arr1.Resize(5, std::string("fill"));
    REQUIRE(arr1.GetSize() == 5);
    REQUIRE(arr1[3] == "fill");
    REQUIRE(arr1[4] == "fill");

    // Resize down
    arr1.Resize(2);
    REQUIRE(arr1.GetSize() == 2);

    arr1.Pop();
    REQUIRE(arr1.GetSize() == 1);

    arr1.Clear();
    REQUIRE(arr1.IsEmpty());
}
