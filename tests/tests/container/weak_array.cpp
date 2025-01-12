#include "tkit/container/weak_array.hpp"
#include "tkit/memory/arena_allocator.hpp"
#include "tkit/core/alias.hpp"
#include "tkit/core/literals.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
using namespace Literals;
ArenaAllocator g_Arena(1_mb);

template <typename T, usize Capacity> WeakArray<T, Capacity> CreateArray()
{
    if constexpr (Capacity == Limits<usize>::max())
        return WeakArray<T, Capacity>(g_Arena.Push<T>(10), 10);
    else
        return WeakArray<T, Capacity>(g_Arena.Push<T>(Capacity));
}

template <typename T, usize Capacity = Limits<usize>::max()>
void RunStaticArrayOperatorTests(const std::array<T, 5> &p_Args)
{
    WeakArray<T, Capacity> array = CreateArray<T, Capacity>();
    for (usize i = 0; i < 5; i++)
        array.push_back(p_Args[i]);

    REQUIRE(array.size() == 5);
    SECTION("Push back")
    {
        for (usize i = 0; i < 5; i++)
        {
            array.push_back(array[i]);
            REQUIRE(array.size() == i + 6);
            REQUIRE(array.back() == array[i]);
        }
        if constexpr (Capacity == 10)
            REQUIRE(array.full());
    }

    SECTION("Pop back")
    {
        while (!array.empty())
            array.pop_back();
        REQUIRE(array.size() == 0);
    }

    SECTION("Insert")
    {
        // Must insert explicit copies, because perfect forwarding will catch those as references and the array will be
        // mangled *before* inserting the element
        T elem0 = array[0];
        T elem2 = array[2];
        array.insert(array.begin(), elem2);
        REQUIRE(array.size() == 6);
        REQUIRE(elem2 == array[0]);
        array.insert(array.begin() + 2, elem0);
        REQUIRE(array.size() == 7);
        REQUIRE(elem0 == array[2]);

        T elem4 = array[4];
        T elem5 = array[5];
        T elem6 = array[6];
        array.insert(array.begin() + 4, {elem4, elem5, elem6});
        REQUIRE(array.size() == 10);
        for (usize i = 4; i < 7; ++i)
            REQUIRE(array[i] == array[i + 3]);

        SECTION("Build in reverse")
        {
            array.clear();
            std::array<T, 5> values = p_Args;
            for (const T &value : values)
                array.insert(array.begin(), value);

            usize index = 0;
            for (auto it = array.rbegin(); it != array.rend(); ++it)
                REQUIRE(*it == values[index++]);
        }
    }

    SECTION("Erase")
    {
        T elem1 = array[1];
        T elem3 = array[3];
        array.erase(array.begin());
        REQUIRE(array.size() == 4);
        REQUIRE(array[0] == elem1);
        array.erase(array.begin(), array.begin() + 2);
        REQUIRE(array.size() == 2);
        REQUIRE(array[0] == elem3);

        array.insert(array.end(), {elem1, elem3});
        while (!array.empty())
        {
            if (array.size() > 1)
            {
                const T elem = array[1];
                array.erase(array.begin());
                REQUIRE(array[0] == elem);
            }
            else
                array.erase(array.begin());
        }

        array.insert(array.end(), p_Args.begin(), p_Args.end());
        array.erase(array.begin(), array.end());
        REQUIRE(array.size() == 0);
    }

    SECTION("Resize")
    {
        std::array<T, 5> values = p_Args;
        SECTION("Clear from resize")
        {
            array.resize(0);
            REQUIRE(array.size() == 0);
            REQUIRE(array.empty());
        }

        SECTION("Decrease size")
        {
            array.resize(3);
            REQUIRE(array.size() == 3);
            for (usize i = 0; i < 3; ++i)
                REQUIRE(array[i] == values[i]);
        }

        SECTION("Increase size")
        {
            array.resize(7);
            REQUIRE(array.size() == 7);
            for (usize i = 0; i < 3; ++i)
                REQUIRE(array[i] == values[i]);
        }
    }

    SECTION("Emplace back")
    {
        std::array<T, 5> values = p_Args;
        array.clear();
        for (usize i = 0; i < 5; ++i)
            array.emplace_back(values[i]);

        for (usize i = 0; i < 5; ++i)
            REQUIRE(array[i] == values[i]);
    }

    SECTION("Clear")
    {
        array.clear();
        REQUIRE(array.size() == 0);
    }
}

TEST_CASE("WeakArray (i32) Dynamic capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<i32>({1, 2, 3, 4, 5});
    g_Arena.Reset();
}
TEST_CASE("WeakArray (i32) Static capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<i32, 10>({1, 2, 3, 4, 5});
    g_Arena.Reset();
}

TEST_CASE("WeakArray (f32) Dynamic capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<f32>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f});
    g_Arena.Reset();
}
TEST_CASE("WeakArray (f32) Static capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<f32, 10>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f});
    g_Arena.Reset();
}

TEST_CASE("WeakArray (f64) Dynamic capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<f64>({1.0, 2.0, 3.0, 4.0, 5.0});
    g_Arena.Reset();
}
TEST_CASE("WeakArray (f64) Static capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<f64, 10>({1.0, 2.0, 3.0, 4.0, 5.0});
    g_Arena.Reset();
}

TEST_CASE("WeakArray (std::string) Dynamic capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<std::string>({"10", "20", "30", "40", "50"});
    g_Arena.Reset();
}
TEST_CASE("WeakArray (std::string) Static capacity", "[core][container][WeakArray]")
{
    RunStaticArrayOperatorTests<std::string, 10>({"10", "20", "30", "40", "50"});
    g_Arena.Reset();
}

TEST_CASE("WeakArray cleanup check", "[core][container][WeakArray]")
{
    WeakArray<NonTrivialData, 10> array = CreateArray<NonTrivialData, 10>();
    for (usize i = 0; i < 5; i++)
        array.emplace_back();
    REQUIRE(NonTrivialData::Instances == 5);
    array.pop_back();
    REQUIRE(NonTrivialData::Instances == 4);
    array.erase(array.begin());
    REQUIRE(NonTrivialData::Instances == 3);
    array.erase(array.begin(), array.begin() + 2);
    REQUIRE(NonTrivialData::Instances == 1);
    array.clear();
    REQUIRE(NonTrivialData::Instances == 0);

    SECTION("Cleanup check with erase and resize")
    {
        NonTrivialData data1;
        NonTrivialData data2;
        NonTrivialData data3;
        NonTrivialData data4;
        NonTrivialData data5;

        SECTION("Insert and erase")
        {
            array.push_back(data1);
            REQUIRE(NonTrivialData::Instances == 1 + 5);
            array.insert(array.begin(), data2);
            REQUIRE(NonTrivialData::Instances == 2 + 5);
            array.insert(array.begin() + 1, {data3, data4, data5});
            REQUIRE(NonTrivialData::Instances == 5 + 5);

            array.erase(array.begin());
            REQUIRE(NonTrivialData::Instances == 4 + 5);
            array.erase(array.begin(), array.begin() + 2);
            REQUIRE(NonTrivialData::Instances == 2 + 5);
        }

        SECTION("Resize")
        {
            array.insert(array.end(), {data1, data2, data3, data4, data5, data1, data2, data3, data4, data5});
            REQUIRE(NonTrivialData::Instances == 10 + 5);

            array.resize(7);
            REQUIRE(NonTrivialData::Instances == 7 + 5);

            array.resize(10);
            REQUIRE(NonTrivialData::Instances == 10 + 5);

            array.resize(2);
            REQUIRE(NonTrivialData::Instances == 2 + 5);

            array.resize(5);
            REQUIRE(NonTrivialData::Instances == 5 + 5);

            array.resize(0);
            REQUIRE(NonTrivialData::Instances == 0 + 5);
        }
    }

    array.clear();
    REQUIRE(NonTrivialData::Instances == 0);
    g_Arena.Reset();
}
} // namespace TKit