#include "tkit/container/span.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/static_array.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

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

TEST_CASE("Span dynamic extent: from FixedArray, StaticArray, DynamicArray", "[Span]")
{
    FixedArray<u32, 3> arr = {2u, 4u, 6u};
    StaticArray<u32, 3> sarr{{7u, 8u}};
    DynamicArray<u32> darr{{9u, 10u, 11u}};

    const Span<u32> span1(arr);
    REQUIRE(span1.GetSize() == 3);

    const Span<u32> span2(sarr);
    REQUIRE(span2.GetSize() == 2);

    const Span<u32> span3(darr);
    REQUIRE(span3.GetSize() == 3);

    // conversion from Span<U,?>
    const Span<const u32> span5 = span3;
    REQUIRE(span5.GetSize() == span3.GetSize());
}

TEST_CASE("Span dynamic extent: iteration and bool", "[Span]")
{
    DynamicArray<std::string> vec = {{"a", "b", "c"}};
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
TEST_CASE("Span string: Count", "[Span]")
{
    const StringView sv("hello world");
    REQUIRE(sv.Count('l') == 3);
    REQUIRE(sv.Count('o') == 2);
    REQUIRE(sv.Count('z') == 0);
    REQUIRE(sv.Count("ll") == 1);
    REQUIRE(sv.Count("l") == 3);
    REQUIRE(sv.Count("xyz") == 0);
    REQUIRE(sv.Count("") == 0);
}

TEST_CASE("Span string: TrimLeft", "[Span]")
{
    const StringView sv("  \t hello ");
    const StringView trimmed = sv.TrimLeft();
    REQUIRE(trimmed.Compare("hello ") == 0);

    const StringView noLeft("hello ");
    REQUIRE(noLeft.TrimLeft() == "hello ");

    const StringView allSpaces("   ");
    const StringView empty = allSpaces.TrimLeft();
    REQUIRE(empty.GetSize() == 0);

    const StringView custom("xxyhello");
    REQUIRE(custom.TrimLeft("xy") == "hello");
}

TEST_CASE("Span string: TrimRight", "[Span]")
{
    const StringView sv(" hello  \t ");
    const StringView trimmed = sv.TrimRight();
    REQUIRE(trimmed == " hello");

    const StringView noRight(" hello");
    REQUIRE(noRight.TrimRight() == " hello");

    const StringView allSpaces("   ");
    const StringView empty = allSpaces.TrimRight();
    REQUIRE(empty.GetSize() == 0);

    const StringView custom("helloxyyx");
    REQUIRE(custom.TrimRight("xy") == "hello");
}

TEST_CASE("Span string: Trim", "[Span]")
{
    const StringView sv("  hello  ");
    REQUIRE(sv.Trim() == "hello");

    const StringView clean("hello");
    REQUIRE(clean.Trim() == "hello");

    const StringView allSpaces("  \t\n ");
    REQUIRE(allSpaces.Trim().GetSize() == 0);
}

TEST_CASE("Span string: TrimLeft/TrimRight mutating", "[Span]")
{
    char buf[] = "  hello  ";
    Span<char> sv(buf, 9);
    sv.TrimLeft();
    REQUIRE(sv == "hello  ");
    sv.TrimRight();
    REQUIRE(sv == "hello");
}

TEST_CASE("Span string: Replace (mutable)", "[Span]")
{
    char buf[] = "hello world";
    Span<char> sv(buf, 11);
    sv.Replace('l', 'L');
    REQUIRE(sv == "heLLo worLd");

    char buf2[] = "aabbcc";
    Span<char> sv2(buf2, 6);
    sv2.Replace('z', 'x');
    REQUIRE(sv2 == "aabbcc");
}

TEST_CASE("Span string: Compare and equality with Span", "[Span]")
{
    const StringView a("abc");
    const StringView b("abc");
    const StringView c("abd");
    const StringView d("ab");

    REQUIRE(a.Compare(b) == 0);
    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE(a != d);
    REQUIRE(a.Compare(c) < 0);
    REQUIRE(c.Compare(a) > 0);
    REQUIRE(a.Compare(d) > 0);
    REQUIRE(d.Compare(a) < 0);
}

TEST_CASE("Span string: equality with Array", "[Span]")
{
    DynamicString arr("hello");
    const StringView sv("hello");
    const StringView other("world");

    REQUIRE(sv == arr);
    REQUIRE(arr == sv);
    REQUIRE(other != arr);
    REQUIRE(arr != other);
}

TEST_CASE("Span string: concatenation with Array", "[Span]")
{
    DynamicString arr("hello");
    const StringView sv(" world");

    const DynamicString result1 = arr + sv;
    REQUIRE(result1 == "hello world");

    const DynamicString result2 = sv + arr;
    REQUIRE(result2 == " worldhello");
}
