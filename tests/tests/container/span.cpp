#include "tkit/container/span.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
TEST_CASE("Span (i32)", "[core][container][Span]")
{
    SECTION("Constructor (static extent)")
    {
        i32 values[5] = {1, 2, 3, 4, 5};
        Span<i32, 5> span(values);
        REQUIRE(span.size() == 5);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(span[i] == values[i]);
    }
    SECTION("Constructor (dynamic extent)")
    {
        i32 values[5] = {1, 2, 3, 4, 5};
        Span<i32> span(values, 5);
        REQUIRE(span.size() == 5);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(span[i] == values[i]);
    }

    SECTION("Copy constructor (static extent)")
    {
        i32 values[5] = {1, 2, 3, 4, 5};
        Span<i32, 5> span1(values);
        Span<i32, 5> span2 = span1;
        REQUIRE(span1.size() == 5);
        REQUIRE(span2.size() == 5);
        for (usize i = 0; i < 5; ++i)
            REQUIRE(span1[i] == span2[i]);
    }
    SECTION("Copy constructor (dynamic extent)")
    {
        i32 values[5] = {1, 2, 3, 4, 5};
        Span<i32> span1(values, 5);
        Span<i32> span2 = span1;
        REQUIRE(span1.size() == 5);
        REQUIRE(span2.size() == 5);
        for (usize i = 0; i < 5; ++i)
        {
            REQUIRE(span1[i] == span2[i]);
            REQUIRE(&span1[i] == &span2[i]);
        }
    }

    SECTION("Copy assignment (static extent)")
    {
        i32 values[5] = {1, 2, 3, 4, 5};
        Span<i32, 5> span1(values);
        Span<i32, 5> span2;
        span2 = span1;
        REQUIRE(span1.size() == 5);
        REQUIRE(span2.size() == 5);
        for (usize i = 0; i < 5; ++i)
        {
            REQUIRE(span1[i] == span2[i]);
            REQUIRE(&span1[i] == &span2[i]);
        }
    }
    SECTION("Copy assignment (dynamic extent)")
    {
        i32 values[5] = {1, 2, 3, 4, 5};
        Span<i32> span1(values, 5);
        Span<i32> span2;
        span2 = span1;
        REQUIRE(span1.size() == 5);
        REQUIRE(span2.size() == 5);
        for (usize i = 0; i < 5; ++i)
        {
            REQUIRE(span1[i] == span2[i]);
            REQUIRE(&span1[i] == &span2[i]);
        }
    }
}
} // namespace TKit