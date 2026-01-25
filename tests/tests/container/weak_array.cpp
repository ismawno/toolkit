#include "tkit/container/weak_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/static_array.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

// track constructions/destructions
static u32 g_CtorCount = 0;
static u32 g_DtorCount = 0;
struct WTrackable
{
    u32 Value;
    WTrackable() : Value(0)
    {
        ++g_CtorCount;
    }
    WTrackable(const u32 value) : Value(value)
    {
        ++g_CtorCount;
    }
    WTrackable(const WTrackable &other) : Value(other.Value)
    {
        ++g_CtorCount;
    }
    WTrackable(WTrackable &&other) : Value(other.Value)
    {
        ++g_CtorCount;
    }
    ~WTrackable()
    {
        ++g_DtorCount;
    }
    WTrackable &operator=(const WTrackable &other)
    {
        Value = other.Value;
        return *this;
    }
    WTrackable &operator=(WTrackable &&other)
    {
        Value = other.Value;
        return *this;
    }
};

TEST_CASE("WeakArray static: default, pointer, pointer+size", "[WeakArray]")
{
    u32 buffer[4] = {1, 2, 3, 4};

    const WeakArray<u32, 4> arr1;
    REQUIRE(!arr1);
    REQUIRE(arr1.IsEmpty());
    REQUIRE(arr1.GetSize() == 0);

    const WeakArray<u32, 4> arr2(buffer);
    REQUIRE(arr2);
    REQUIRE(arr2.GetData() == buffer);
    REQUIRE(arr2.GetSize() == 0);

    const WeakArray<u32, 4> arr3(buffer, 3);
    REQUIRE(arr3.GetSize() == 3);
    REQUIRE(arr3[2] == 3);
    REQUIRE(arr3.GetFront() == 1);
    REQUIRE(arr3.GetBack() == 3);
}

TEST_CASE("WeakArray static: from FixedArray and StaticArray", "[WeakArray]")
{
    FixedArray<u32, 3> rawArr{10u, 20u, 30u};
    const WeakArray<u32, 3> arr1(rawArr, 2);
    REQUIRE(arr1.GetData() == rawArr.GetData());
    REQUIRE(arr1.GetSize() == 2);

    StaticArray<u32, 3> staticArr{{5u, 6u}};
    const WeakArray<u32, 3> arr2(staticArr);
    REQUIRE(arr2.GetSize() == 2);
    REQUIRE(arr2[1] == 6u);
}

TEST_CASE("WeakArray static: move ctor and move assign", "[WeakArray]")
{
    u32 buffer[2] = {7, 8};
    WeakArray<u32, 2> source(buffer, 2);

    WeakArray<u32, 2> arr1(std::move(source));
    REQUIRE(arr1.GetSize() == 2);
    REQUIRE(!source);

    WeakArray<u32, 2> arr2(buffer, 1);
    arr2 = std::move(arr1);
    REQUIRE(arr2.GetSize() == 2);
    REQUIRE(!arr1);
}

TEST_CASE("WeakArray static: modify elements", "[WeakArray]")
{
    u32 buffer[5] = {};
    WeakArray<u32, 5> arr(buffer, 0);

    arr.Append(1);
    arr.Append(2);
    arr.Append(3);
    REQUIRE(arr.GetSize() == 3);

    arr.Pop();
    REQUIRE(arr.GetSize() == 2);

    arr.Insert(arr.begin() + 1, 9);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(arr[1] == 9);

    const TKit::FixedArray<u32, 2> extra = {7, 8};
    arr.Insert(arr.begin() + 3, extra.begin(), extra.end());
    REQUIRE(arr.GetSize() == 5);
    REQUIRE(arr[3] == 7);
    REQUIRE(arr[4] == 8);

    arr.RemoveOrdered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 4);

    arr.RemoveOrdered(arr.begin() + 1, arr.begin() + 3);
    REQUIRE(arr.GetSize() == 2);

    arr = WeakArray<u32, 5>(buffer, 2);
    arr.RemoveUnordered(arr.begin());
    REQUIRE(arr.GetSize() == 1);
}

TEST_CASE("WeakArray static: Resize, Clear, iteration", "[WeakArray]")
{
    TKit::FixedArray<WTrackable, 4> backing;
    WeakArray<WTrackable, 4> arr(backing.GetData(), 0);
    g_CtorCount = g_DtorCount = 0;

    arr.Resize(3, 42u);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(g_CtorCount == 3);
    for (u32 i = 0; i < 3; ++i)
        REQUIRE(arr[i].Value == 42u);

    arr.Resize(1);
    REQUIRE(arr.GetSize() == 1);
    REQUIRE(g_DtorCount == 2);

    arr.Clear();
    REQUIRE(arr.IsEmpty());

    // iteration
    arr.Resize(2, 7u);
    u32 sum = 0;
    for (const auto &x : arr)
        sum += x.Value;
    REQUIRE(sum == 14u);
}

TEST_CASE("WeakArray static<string>: non-trivial", "[WeakArray][string]")
{
    std::string buffer[4];
    WeakArray<std::string, 4> arr(buffer, 0);

    arr.Append("a");
    arr.Append("b");
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(arr[1] == "b");

    auto arr2 = std::move(arr);
    REQUIRE(arr2.GetSize() == 2);
    arr2.Clear();
    REQUIRE(arr2.IsEmpty());
}

TEST_CASE("WeakArray dynamic: default, pointer+capacity, pointer+capacity+size", "[WeakArray]")
{
    u32 buffer[5] = {1, 2, 3, 4, 5};

    const WeakArray<u32> arr1;
    REQUIRE(!arr1);
    REQUIRE(arr1.IsEmpty());
    REQUIRE(arr1.GetCapacity() == 0);

    const WeakArray<u32> arr2(buffer, 5);
    REQUIRE(arr2.GetCapacity() == 5);
    REQUIRE(arr2.GetSize() == 0);

    const WeakArray<u32> arr3(buffer, 5, 3);
    REQUIRE(arr3.GetSize() == 3);
    REQUIRE(arr3[2] == 3);
}

TEST_CASE("WeakArray dynamic: from FixedArray, StaticArray, DynamicArray", "[WeakArray]")
{
    FixedArray<u32, 3> rawArr{2u, 4u, 6u};
    StaticArray<u32, 3> staticArr{{7u, 8u}};
    DynamicArray<u32> dynArr{{9u, 10u, 11u}};

    const WeakArray<u32> arr1(rawArr, 2);
    const WeakArray<u32> arr2(staticArr);
    const WeakArray<u32> arr3(dynArr);

    REQUIRE(arr1.GetCapacity() == 3);
    REQUIRE(arr1.GetSize() == 2);
    REQUIRE(arr2.GetCapacity() == 3);
    REQUIRE(arr2.GetSize() == 2);
    REQUIRE(arr3.GetCapacity() == dynArr.GetCapacity());
    REQUIRE(arr3.GetSize() == 3);
}

TEST_CASE("WeakArray dynamic: move ctor and move assign", "[WeakArray]")
{
    u32 buffer[4] = {};
    WeakArray<u32> source(buffer, 4, 2);

    WeakArray<u32> arr1(std::move(source));
    REQUIRE(arr1.GetSize() == 2);
    REQUIRE(!source);

    WeakArray<u32> arr2;
    arr2 = std::move(arr1);
    REQUIRE(arr2.GetSize() == 2);
    REQUIRE(!arr1);
}

TEST_CASE("WeakArray dynamic: modify & inspect", "[WeakArray]")
{
    u32 buffer[6] = {};
    WeakArray<u32> arr(buffer, 6, 0);

    arr.Append(5);
    arr.Append(6);
    REQUIRE(arr.GetSize() == 2);

    arr.Pop();
    REQUIRE(arr.GetSize() == 1);

    arr.Insert(arr.begin() + 1, 7);
    REQUIRE(arr[1] == 7);

    const FixedArray<u32, 3> extra{8, 9, 10};
    arr.Insert(arr.begin() + 2, extra.begin(), extra.end());
    REQUIRE(arr.GetSize() == 5);
    REQUIRE(arr.GetBack() == 10);

    arr.RemoveOrdered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 4);

    arr.RemoveUnordered(arr.begin());
    REQUIRE(arr.GetSize() == 3);

    arr.Resize(4, 3);
    REQUIRE(arr.GetSize() == 4);
    REQUIRE(arr[3] == 3);

    arr.Clear();
    REQUIRE(arr.IsEmpty());
}

TEST_CASE("WeakArray dynamic<string>: non-trivial", "[WeakArray][string]")
{
    std::string buffer[5];
    WeakArray<std::string> arr(buffer, 5, 0);

    arr.Append("x");
    arr.Append("y");
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(arr[0] == "x");

    auto arr2 = std::move(arr);
    REQUIRE(arr2.GetSize() == 2);
    arr2.Clear();
    REQUIRE(arr2.IsEmpty());
}
