#include "tkit/container/span.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

TEST_CASE("Span static extent: default and pointer ctor", "[Span]")
{
    u32 raw[3] = {1, 2, 3};

    const Span<u32, 3> span1;
    REQUIRE(!span1);

    const Span<u32, 3> span2(raw);
    REQUIRE(span2);
    REQUIRE(span2.GetData() == raw);
    REQUIRE(span2.GetSize() == 3);
    REQUIRE(span2[0] == 1);
    REQUIRE(span2.At(2) == 3);
    REQUIRE(span2.GetFront() == 1);
    REQUIRE(span2.GetBack() == 3);

    // construction from Array<T,Extent>
    Array<u32, 3> arr = {10u, 20u, 30u};
    const Span<u32, 3> span3(arr);
    REQUIRE(span3.GetSize() == 3);
    REQUIRE(span3[1] == 20u);

    // conversion from Span<U,Extent>
    const Span<const u32, 3> span4 = span3;
    REQUIRE(span4.GetData() == span3.GetData());
}

TEST_CASE("Span dynamic extent: default and pointer+size ctor", "[Span]")
{
    const Span<u32> span1;
    REQUIRE(!span1);
    REQUIRE(span1.IsEmpty());

    u32 raw[4] = {4, 5, 6, 7};
    const Span<u32> span2(raw, 4);
    REQUIRE(span2);
    REQUIRE(!span2.IsEmpty());
    REQUIRE(span2.GetSize() == 4);
    REQUIRE(span2[0] == 4);
    REQUIRE(span2.At(3) == 7);
}

TEST_CASE("Span dynamic extent: from Array, StaticArray, WeakArray, DynamicArray", "[Span]")
{
    Array<u32, 3> arr = {2u, 4u, 6u};
    StaticArray<u32, 3> sarr{7u, 8u};
    DynamicArray<u32> darr{9u, 10u, 11u};
    Array<u32, 4> backing = {1u, 2u, 3u, 4u};
    const WeakArray<u32, 4> warr(backing.GetData(), 2);

    const Span<u32> span1(arr);
    REQUIRE(span1.GetSize() == 3);

    const Span<u32> span2(sarr);
    REQUIRE(span2.GetSize() == 2);

    const Span<u32> span3(darr);
    REQUIRE(span3.GetSize() == 3);

    const Span<u32> span4(warr);
    REQUIRE(span4.GetSize() == 2);

    // conversion from Span<U,?>
    const Span<const u32> span5 = span3;
    REQUIRE(span5.GetSize() == span3.GetSize());
}

TEST_CASE("Span dynamic extent: iteration and bool", "[Span]")
{
    DynamicArray<std::string> vec = {"a", "b", "c"};
    Span<std::string> span(vec.GetData(), vec.GetSize());
    REQUIRE(span);
    REQUIRE(!span.IsEmpty());

    std::string concat;
    for (auto &s : span)
        concat += s;
    REQUIRE(concat == "abc");

    span = Span<std::string>(nullptr, 0);
    REQUIRE(!span);
    REQUIRE(span.IsEmpty());
}
