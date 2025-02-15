#include "tkit/container/static_array.hpp"
#include "tkit/utils/alias.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
// Assumed args contains 5 elements
template <typename T, typename... Args> void RunStaticArrayConstructorTest(Args... p_Args)
{
    SECTION("Default constructor")
    {
        StaticArray<T, 10> array;
        REQUIRE(array.size() == 0);
        REQUIRE(array.capacity() == 10);
    }

    SECTION("Size constructor")
    {
        StaticArray<T, 10> array(5);
        REQUIRE(array.size() == 5);
        REQUIRE(array.capacity() == 10);
    }

    SECTION("Iterator constructor")
    {
        Array<T, 5> values = {p_Args...};
        StaticArray<T, 10> array(values.begin(), values.end());
        REQUIRE(array.size() == 5);
        REQUIRE(array.capacity() == 10);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(array[i] == values[i]);
    }

    SECTION("Copy constructor")
    {
        StaticArray<T, 10> array1 = {p_Args...};
        StaticArray<T, 10> array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 10);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    SECTION("Copy constructor (different capacity)")
    {
        StaticArray<T, 10> array1 = {p_Args...};
        StaticArray<T, 5> array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 5);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    SECTION("Copy assignment")
    {
        StaticArray<T, 10> array1 = {p_Args...};
        StaticArray<T, 10> array2{5};
        array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 10);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    SECTION("Copy assignment (different capacity)")
    {
        StaticArray<T, 10> array1 = {p_Args...};
        StaticArray<T, 5> array2{5};
        array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 5);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    if constexpr (std::integral<T>)
    {
        SECTION("Initializer list")
        {
            StaticArray<T, 10> array = {p_Args...};
            REQUIRE(array.size() == 5);
            REQUIRE(array.capacity() == 10);
            for (i32 i = 0; i < 5; ++i)
                REQUIRE(array[i] == i + 1);
        }
    }
}

// Assumed args contains 5 elements
template <typename T, typename... Args> void RunStaticArrayOperatorTests(Args... p_Args)
{
    StaticArray<T, 10> array = {p_Args...};
    SECTION("Push back")
    {
        for (usize i = 0; i < 5; ++i)
        {
            array.push_back(array[i]);
            REQUIRE(array.size() == i + 6);
            REQUIRE(array.back() == array[i]);
        }
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
            Array<T, 5> values = {p_Args...};
            for (const T &value : values)
                array.insert(array.begin(), value);

            usize index = 0;
            for (auto it = array.rbegin(); it != array.rend(); ++it)
                REQUIRE(*it == values[index++]);

            StaticArray<T, 400> bigArray;
            while (!bigArray.full())
                bigArray.insert(bigArray.begin(), values[0]);
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

        array.insert(array.end(), {p_Args...});
        array.erase(array.begin(), array.end());
        REQUIRE(array.size() == 0);
    }

    SECTION("Resize")
    {
        Array<T, 5> values = {p_Args...};
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
        Array<T, 5> values = {p_Args...};
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

TEST_CASE("StaticArray (i32)", "[core][container][static_array]")
{
    RunStaticArrayConstructorTest<i32>(1, 2, 3, 4, 5);
    RunStaticArrayOperatorTests<i32>(1, 2, 3, 4, 5);
}

TEST_CASE("StaticArray (f32)", "[core][container][static_array]")
{
    RunStaticArrayConstructorTest<f32>(1.0f, 2.0f, 3.0f, 4.0f, 5.0f);
    RunStaticArrayOperatorTests<f32>(1.0f, 2.0f, 3.0f, 4.0f, 5.0f);
}

TEST_CASE("StaticArray (f64)", "[core][container][static_array]")
{
    RunStaticArrayConstructorTest<f64>(1.0, 2.0, 3.0, 4.0, 5.0);
    RunStaticArrayOperatorTests<f64>(1.0, 2.0, 3.0, 4.0, 5.0);
}

TEST_CASE("StaticArray (std::string)", "[core][container][static_array]")
{
    RunStaticArrayConstructorTest<std::string>("10", "20", "30", "40", "50");
    RunStaticArrayOperatorTests<std::string>("10", "20", "30", "40", "50");
}

TEST_CASE("StaticArray cleanup check", "[core][container][static_array]")
{
    StaticArray<NonTrivialData, 10> array{5};
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
}
} // namespace TKit