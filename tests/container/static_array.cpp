#include "kit/container/static_array.hpp"
#include "kit/core/alias.hpp"
#include "data_types.hpp"
#include <catch2/catch_test_macros.hpp>

// Assumed args contains 5 elements
template <typename T, typename... Args> void RunStaticArrayConstructorTest(Args... args)
{
    SECTION("Default constructor")
    {
        KIT::StaticArray<T, 10> array;
        REQUIRE(array.size() == 0);
        REQUIRE(array.capacity() == 10);
    }

    SECTION("Size constructor")
    {
        KIT::StaticArray<T, 10> array(5);
        REQUIRE(array.size() == 5);
        REQUIRE(array.capacity() == 10);
    }

    SECTION("Iterator constructor")
    {
        std::array<T, 5> values = {args...};
        KIT::StaticArray<T, 10> array(values.begin(), values.end());
        REQUIRE(array.size() == 5);
        REQUIRE(array.capacity() == 10);
        for (size_t i = 0; i < 5; ++i)
            REQUIRE(array[i] == values[i]);
    }

    SECTION("Copy constructor")
    {
        KIT::StaticArray<T, 10> array1 = {args...};
        KIT::StaticArray<T, 10> array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 10);
        for (size_t i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    SECTION("Copy constructor (different capacity)")
    {
        KIT::StaticArray<T, 10> array1 = {args...};
        KIT::StaticArray<T, 5> array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 5);
        for (size_t i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    SECTION("Copy assignment")
    {
        KIT::StaticArray<T, 10> array1 = {args...};
        KIT::StaticArray<T, 10> array2{5};
        array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 10);
        for (size_t i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    SECTION("Copy assignment (different capacity)")
    {
        KIT::StaticArray<T, 10> array1 = {args...};
        KIT::StaticArray<T, 5> array2{5};
        array2 = array1;
        REQUIRE(array1.size() == 5);
        REQUIRE(array1.capacity() == 10);
        REQUIRE(array2.size() == 5);
        REQUIRE(array2.capacity() == 5);
        for (size_t i = 0; i < 5; ++i)
            REQUIRE(array1[i] == array2[i]);
    }

    if constexpr (std::integral<T>)
    {
        SECTION("Initializer list")
        {
            KIT::StaticArray<T, 10> array = {args...};
            REQUIRE(array.size() == 5);
            REQUIRE(array.capacity() == 10);
            for (int i = 0; i < 5; ++i)
                REQUIRE(array[i] == i + 1);
        }
    }
}

// Assumed args contains 5 elements
template <typename T, typename... Args> void RunStaticArrayOperatorTests(Args... args)
{
    KIT::StaticArray<T, 10> array = {args...};
    REQUIRE_THROWS(array[6]);
    SECTION("Push back")
    {
        for (size_t i = 0; i < 5; i++)
        {
            array.push_back(array[i]);
            REQUIRE(array.size() == i + 6);
            REQUIRE(array.back() == array[i]);
        }
        REQUIRE(array.full());
        REQUIRE_THROWS(array.push_back(array.front()));
    }

    SECTION("Pop back")
    {
        while (!array.empty())
            array.pop_back();

        REQUIRE_THROWS(array.pop_back());
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
        for (size_t i = 4; i < 7; ++i)
            REQUIRE(array[i] == array[i + 3]);
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
    }

    SECTION("Clear")
    {
        array.clear();
        REQUIRE(array.size() == 0);
    }
}

TEST_CASE("StaticArray (integer)", "[core][container][StaticArray]")
{
    RunStaticArrayConstructorTest<int>(1, 2, 3, 4, 5);
    RunStaticArrayOperatorTests<int>(1, 2, 3, 4, 5);
}

TEST_CASE("StaticArray (float)", "[core][container][StaticArray]")
{
    RunStaticArrayConstructorTest<float>(1.0f, 2.0f, 3.0f, 4.0f, 5.0f);
    RunStaticArrayOperatorTests<float>(1.0f, 2.0f, 3.0f, 4.0f, 5.0f);
}

TEST_CASE("StaticArray (double)", "[core][container][StaticArray]")
{
    RunStaticArrayConstructorTest<double>(1.0, 2.0, 3.0, 4.0, 5.0);
    RunStaticArrayOperatorTests<double>(1.0, 2.0, 3.0, 4.0, 5.0);
}

TEST_CASE("StaticArray (string)", "[core][container][StaticArray]")
{
    RunStaticArrayConstructorTest<KIT::String>("10", "20", "30", "40", "50");
    RunStaticArrayOperatorTests<KIT::String>("10", "20", "30", "40", "50");
}

TEST_CASE("StaticArray cleanup check", "[core][container][StaticArray]")
{
    KIT::StaticArray<NonTrivialData, 10> array{5};
    REQUIRE(NonTrivialData::Instances == 5);
    array.pop_back();
    REQUIRE(NonTrivialData::Instances == 4);
    array.erase(array.begin());
    REQUIRE(NonTrivialData::Instances == 3);
    array.erase(array.begin(), array.begin() + 2);
    REQUIRE(NonTrivialData::Instances == 1);
    array.clear();
    REQUIRE(NonTrivialData::Instances == 0);

    {
        NonTrivialData data1;
        NonTrivialData data2;
        NonTrivialData data3;
        NonTrivialData data4;
        NonTrivialData data5;

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
    array.clear();
    REQUIRE(NonTrivialData::Instances == 0);
}