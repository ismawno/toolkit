#include "tkit/container/span.hpp"
#include "tkit/container/array.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
template <typename T, usize Capacity = Limits<usize>::max()> void RunSpanTest(Array<T, 5> p_Args)
{
    SECTION("Constructor")
    {
        Span<T, Capacity> span(p_Args);
        REQUIRE(span.size() == 5);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(span[i] == p_Args[i]);
    }

    SECTION("Copy constructor")
    {
        Span<T, Capacity> span1(p_Args);
        Span<T, Capacity> span2 = span1;
        REQUIRE(span1.size() == 5);
        REQUIRE(span2.size() == 5);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(span1[i] == span2[i]);
    }

    SECTION("Copy assignment")
    {
        Span<T, Capacity> span1(p_Args);
        Span<T, Capacity> span2;
        span2 = span1;
        REQUIRE(span1.size() == 5);
        REQUIRE(span2.size() == 5);
        for (usize i = 0; i < 5; ++i)
        {
            REQUIRE(span1[i] == span2[i]);
            REQUIRE(&span1[i] == &span2[i]);
        }
    }

    SECTION("Const elements")
    {
        const Span<T, Capacity> span1(p_Args);
        REQUIRE(span1.size() == 5);

        const Span<const T, Capacity> span2(p_Args);
        const Span<const T, Capacity> span3(span1);
        REQUIRE(span2.size() == 5);
        REQUIRE(span3.size() == 5);
    }
}

TEST_CASE("Span (i32) Dynamic capacity", "[core][container][span]")
{
    RunSpanTest<i32>({1, 2, 3, 4, 5});
}
TEST_CASE("Span (i32) Static capacity", "[core][container][span]")
{
    RunSpanTest<i32, 5>({1, 2, 3, 4, 5});
}

TEST_CASE("Span (f32) Dynamic capacity", "[core][container][span]")
{
    RunSpanTest<f32>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f});
}
TEST_CASE("Span (f32) Static capacity", "[core][container][span]")
{
    RunSpanTest<f32, 5>({1.0f, 2.0f, 3.0f, 4.0f, 5.0f});
}

TEST_CASE("Span (f64) Dynamic capacity", "[core][container][span]")
{
    RunSpanTest<f64>({1.0, 2.0, 3.0, 4.0, 5.0});
}
TEST_CASE("Span (f64) Static capacity", "[core][container][span]")
{
    RunSpanTest<f64, 5>({1.0, 2.0, 3.0, 4.0, 5.0});
}

TEST_CASE("Span (std::string) Dynamic capacity", "[core][container][span]")
{
    RunSpanTest<std::string>({"10", "20", "30", "40", "50"});
}
TEST_CASE("Span (std::string) Static capacity", "[core][container][span]")
{
    RunSpanTest<std::string, 5>({"10", "20", "30", "40", "50"});
}
} // namespace TKit