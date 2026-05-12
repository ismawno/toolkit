#include "tkit/container/tier_array.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

static ArenaAllocator s_Arena{10_kib};
static TierAllocator s_Alloc{{.Allocator = &s_Arena, .MaxAllocation = 16_kib}};

// ─── StartsWith / EndsWith ──────────────────────────────────────────

TEST_CASE("TierString: StartsWith(const char*)", "[TierString]")
{
    const TierString str{"Hello, world!", &s_Alloc};
    REQUIRE(str.StartsWith("Hello"));
    REQUIRE(str.StartsWith("Hello, world!"));
    REQUIRE_FALSE(str.StartsWith("world"));
    REQUIRE_FALSE(str.StartsWith("Hello, world! and more"));
    REQUIRE(str.StartsWith(""));
}

TEST_CASE("TierString: StartsWith(char)", "[TierString]")
{
    const TierString str{"Hello", &s_Alloc};
    REQUIRE(str.StartsWith('H'));
    REQUIRE_FALSE(str.StartsWith('h'));

    const TierString empty{"", &s_Alloc};
    REQUIRE_FALSE(empty.StartsWith('H'));
}

TEST_CASE("TierString: EndsWith(const char*)", "[TierString]")
{
    const TierString str{"Hello, world!", &s_Alloc};
    REQUIRE(str.EndsWith("world!"));
    REQUIRE(str.EndsWith("Hello, world!"));
    REQUIRE_FALSE(str.EndsWith("Hello"));
    REQUIRE_FALSE(str.EndsWith("much longer than the string itself"));
    REQUIRE(str.EndsWith(""));
}

TEST_CASE("TierString: EndsWith(char)", "[TierString]")
{
    const TierString str{"Hello!", &s_Alloc};
    REQUIRE(str.EndsWith('!'));
    REQUIRE_FALSE(str.EndsWith('o'));

    const TierString empty{"", &s_Alloc};
    REQUIRE_FALSE(empty.EndsWith('!'));
}

// ─── Contains ───────────────────────────────────────────────────────

TEST_CASE("TierString: Contains", "[TierString]")
{
    const TierString str{"abcdef", &s_Alloc};
    REQUIRE(str.Contains("cde"));
    REQUIRE(str.Contains("abcdef"));
    REQUIRE(str.Contains('a'));
    REQUIRE(str.Contains('f'));
    REQUIRE_FALSE(str.Contains("xyz"));
    REQUIRE_FALSE(str.Contains('z'));
}

// ─── Find ───────────────────────────────────────────────────────────

TEST_CASE("TierString: Find(char)", "[TierString]")
{
    const TierString str{"abcabc", &s_Alloc};
    REQUIRE(str.Find('a') == 0);
    REQUIRE(str.Find('a', 1) == 3);
    REQUIRE(str.Find('c') == 2);
    REQUIRE(str.Find('z') == TierString::npos);
}

TEST_CASE("TierString: Find(const char*)", "[TierString]")
{
    const TierString str{"hello world hello", &s_Alloc};
    REQUIRE(str.Find("hello") == 0);
    REQUIRE(str.Find("hello", 1) == 12);
    REQUIRE(str.Find("world") == 6);
    REQUIRE(str.Find("xyz") == TierString::npos);
    REQUIRE(str.Find("") == 0);
}

TEST_CASE("TierString: FindLast(char)", "[TierString]")
{
    const TierString str{"abcabc", &s_Alloc};
    REQUIRE(str.FindLast('c') == 5);
    REQUIRE(str.FindLast('a') == 3);
    REQUIRE(str.FindLast('z') == TierString::npos);
}

TEST_CASE("TierString: FindLast(const char*)", "[TierString]")
{
    const TierString str{"aaa bbb aaa", &s_Alloc};
    REQUIRE(str.FindLast("aaa") == 8);
    REQUIRE(str.FindLast("bbb") == 4);
    REQUIRE(str.FindLast("ccc") == TierString::npos);
}

TEST_CASE("TierString: FindFirstOf / FindLastOf", "[TierString]")
{
    const TierString str{"hello world", &s_Alloc};
    REQUIRE(str.FindFirstOf("aeiou") == 1); // 'e'
    REQUIRE(str.FindFirstOf("xyz") == TierString::npos);
    REQUIRE(str.FindLastOf("aeiou") == 7); // 'o' in "world"
    REQUIRE(str.FindLastOf("xyz") == TierString::npos);
}

TEST_CASE("TierString: FindFirstNotOf", "[TierString]")
{
    const TierString str{"   hello", &s_Alloc};
    REQUIRE(str.FindFirstNotOf(" ") == 3);

    const TierString allSpaces{"    ", &s_Alloc};
    REQUIRE(allSpaces.FindFirstNotOf(" ") == TierString::npos);
}

// ─── SubString ──────────────────────────────────────────────────────

TEST_CASE("TierString: SubString", "[TierString]")
{
    const TierString str{"Hello, world!", &s_Alloc};

    const auto sub1 = str.SubString(7);
    REQUIRE(sub1.GetSize() == 6);
    REQUIRE(sub1 == "world!");

    const auto sub2 = str.SubString(0, 5);
    REQUIRE(sub2.GetSize() == 5);
    REQUIRE(sub2 == "Hello");

    const auto sub3 = str.SubString(7, 5);
    REQUIRE(sub3.GetSize() == 5);
    REQUIRE(sub3 == "world");

    // pos == size => empty substring
    const auto sub4 = str.SubString(str.GetSize());
    REQUIRE(sub4.GetSize() == 0);
}

// ─── Compare / operator== ───────────────────────────────────────────

TEST_CASE("TierString: Compare and operator==", "[TierString]")
{
    const TierString str{"abc", &s_Alloc};
    REQUIRE(str.Compare("abc") == 0);
    REQUIRE(str.Compare("abd") < 0);
    REQUIRE(str.Compare("abb") > 0);
    REQUIRE(str.Compare("ab") > 0);
    REQUIRE(str.Compare("abcd") < 0);

    REQUIRE(str == "abc");
    REQUIRE_FALSE(str == "xyz");
}

// ─── Trim ───────────────────────────────────────────────────────────

TEST_CASE("TierString: TrimLeft", "[TierString]")
{
    TierString str{"   hello   ", &s_Alloc};
    str.TrimLeft();
    REQUIRE(str == "hello   ");

    TierString allSpaces{"    ", &s_Alloc};
    allSpaces.TrimLeft();
    REQUIRE(allSpaces.GetSize() == 0);

    TierString noSpaces{"hello", &s_Alloc};
    noSpaces.TrimLeft();
    REQUIRE(noSpaces == "hello");
}

TEST_CASE("TierString: TrimRight", "[TierString]")
{
    TierString str{"   hello   ", &s_Alloc};
    str.TrimRight();
    REQUIRE(str == "   hello");

    TierString allSpaces{"    ", &s_Alloc};
    allSpaces.TrimRight();
    REQUIRE(allSpaces.GetSize() == 0);
}

TEST_CASE("TierString: Trim", "[TierString]")
{
    TierString str{"  \t hello \n ", &s_Alloc};
    str.Trim();
    REQUIRE(str == "hello");

    TierString custom{"xxhelloxx", &s_Alloc};
    custom.Trim("x");
    REQUIRE(custom == "hello");
}

// ─── operator+= ────────────────────────────────────────────────────

TEST_CASE("TierString: operator+=(const char*)", "[TierString]")
{
    TierString str{"hello", &s_Alloc};
    str += ", world!";
    REQUIRE(str == "hello, world!");
    REQUIRE(str.GetSize() == 13);
}

TEST_CASE("TierString: operator+=(char)", "[TierString]")
{
    TierString str{"abc", &s_Alloc};
    str += 'd';
    str += 'e';
    REQUIRE(str == "abcde");
}

// ─── operator+ (concatenation) ──────────────────────────────────────

TEST_CASE("TierString: Array + Array", "[TierString]")
{
    const TierString a{"hello", &s_Alloc};
    const TierString b{" world", &s_Alloc};
    const auto result = a + b;
    REQUIRE(result == "hello world");
    REQUIRE(result.GetSize() == 11);
}

TEST_CASE("TierString: Array + const char*", "[TierString]")
{
    const TierString a{"hello", &s_Alloc};
    const auto result = a + " world";
    REQUIRE(result == "hello world");
}

TEST_CASE("TierString: const char* + Array", "[TierString]")
{
    const TierString b{"world", &s_Alloc};
    const auto result = "hello " + b;
    REQUIRE(result == "hello world");
}

// ─── Replace ────────────────────────────────────────────────────────

TEST_CASE("TierString: ReplaceSelf(char, char)", "[TierString]")
{
    TierString str{"a.b.c.d", &s_Alloc};
    str.Replace('.', '/');
    REQUIRE(str == "a/b/c/d");
}

TEST_CASE("TierString: Replace(char, char) const", "[TierString]")
{
    const TierString str{"a-b-c", &s_Alloc};
    const auto result = str.Replace('-', '_');
    // original unchanged
    REQUIRE(str == "a-b-c");
}

TEST_CASE("TierString: Replace(const char*, const char*) same length", "[TierString]")
{
    const TierString str{"foo bar foo", &s_Alloc};
    const auto result = str.Replace("foo", "baz");
    REQUIRE(result == "baz bar baz");
    // original unchanged
    REQUIRE(str == "foo bar foo");
}

TEST_CASE("TierString: Replace(const char*, const char*) shorter replacement", "[TierString]")
{
    const TierString str{"aabbcc", &s_Alloc};
    const auto result = str.Replace("bb", "x");
    REQUIRE(result == "aaxcc");
}

TEST_CASE("TierString: Replace(const char*, const char*) longer replacement", "[TierString]")
{
    const TierString str{"a-b-c", &s_Alloc};
    const auto result = str.Replace("-", "---");
    REQUIRE(result == "a---b---c");
}

TEST_CASE("TierString: Replace with empty from returns copy", "[TierString]")
{
    const TierString str{"hello", &s_Alloc};
    const auto result = str.Replace("", "x");
    REQUIRE(result == "hello");
}

TEST_CASE("TierString: Replace with no matches returns copy", "[TierString]")
{
    const TierString str{"hello", &s_Alloc};
    const auto result = str.Replace("xyz", "abc");
    REQUIRE(result == "hello");
}

TEST_CASE("TierString: Replace to empty (deletion)", "[TierString]")
{
    const TierString str{"hello world", &s_Alloc};
    const auto result = str.Replace(" ", "");
    REQUIRE(result == "helloworld");
}

// ─── Split ──────────────────────────────────────────────────────────

TEST_CASE("TierString: Split(char)", "[TierString]")
{
    const TierString str{"a,b,c,d", &s_Alloc};
    REQUIRE(str.GetSize() == 7);
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
    REQUIRE(parts[3] == "d");
}

TEST_CASE("TierString: Split(char) no delimiter present", "[TierString]")
{
    const TierString str{"hello", &s_Alloc};
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0] == "hello");
}

TEST_CASE("TierString: Split(char) delimiter at edges", "[TierString]")
{
    const TierString str{",a,b,", &s_Alloc};
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0].GetSize() == 0);
    REQUIRE(parts[1] == "a");
    REQUIRE(parts[2] == "b");
    REQUIRE(parts[3].GetSize() == 0);
}

TEST_CASE("TierString: Split(char) consecutive delimiters", "[TierString]")
{
    const TierString str{"a,,b", &s_Alloc};
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1].GetSize() == 0);
    REQUIRE(parts[2] == "b");
}

TEST_CASE("TierString: Split(const char*)", "[TierString]")
{
    const TierString str{"one::two::three", &s_Alloc};
    const auto parts = str.Split("::");
    REQUIRE(parts.GetSize() == 3);
    REQUIRE(parts[0] == "one");
    REQUIRE(parts[1] == "two");
    REQUIRE(parts[2] == "three");
}

TEST_CASE("TierString: Split(const char*) empty delimiter returns whole string", "[TierString]")
{
    const TierString str{"hello", &s_Alloc};
    const auto parts = str.Split("");
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0] == "hello");
}

TEST_CASE("TierString: Split(const char*) multi-char at edges", "[TierString]")
{
    const TierString str{"<>a<>b<>", &s_Alloc};
    const auto parts = str.Split("<>");
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0].GetSize() == 0);
    REQUIRE(parts[1] == "a");
    REQUIRE(parts[2] == "b");
    REQUIRE(parts[3].GetSize() == 0);
}

// ─── Count ──────────────────────────────────────────────────────────

TEST_CASE("TierString: Count(char)", "[TierString]")
{
    const TierString str{"banana", &s_Alloc};
    REQUIRE(str.Count('a') == 3);
    REQUIRE(str.Count('n') == 2);
    REQUIRE(str.Count('b') == 1);
    REQUIRE(str.Count('z') == 0);
}

TEST_CASE("TierString: Count(const char*)", "[TierString]")
{
    const TierString str{"abababab", &s_Alloc};
    REQUIRE(str.Count("ab") == 4);
    REQUIRE(str.Count("aba") == 2); // non-overlapping
    REQUIRE(str.Count("xyz") == 0);
    REQUIRE(str.Count("") == 0);
}

// ─── Format ─────────────────────────────────────────────────────────

TEST_CASE("TierString: Format", "[TierString]")
{
    PushTier(&s_Alloc);
    const auto str = TierString::Format("Hello, {}! x={}", "world", 42);
    REQUIRE(str == "Hello, world! x=42");
    REQUIRE(str.GetSize() == 18);
    PopTier();
}

// ─── Edge cases ─────────────────────────────────────────────────────

TEST_CASE("TierString: construction from const char*", "[TierString]")
{
    const TierString str{"hello", &s_Alloc};
    REQUIRE(str.GetSize() == 5);
    REQUIRE(str == "hello");
    REQUIRE(std::strlen(str.GetData()) == 5);
}

TEST_CASE("TierString: empty string operations", "[TierString]")
{
    const TierString empty{"", &s_Alloc};
    REQUIRE(empty.GetSize() == 0);
    REQUIRE_FALSE(empty.Contains('a'));
    REQUIRE_FALSE(empty.StartsWith('a'));
    REQUIRE_FALSE(empty.EndsWith('a'));
    REQUIRE(empty.Find('a') == TierString::npos);
    REQUIRE(empty.Count('a') == 0);

    const auto parts = empty.Split(',');
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0].GetSize() == 0);
}

TEST_CASE("TierString: null termination preserved after mutations", "[TierString]")
{
    TierString str{"hello", &s_Alloc};
    str += " world";
    REQUIRE(std::strlen(str.GetData()) == str.GetSize());

    str.TrimRight();
    REQUIRE(std::strlen(str.GetData()) == str.GetSize());

    str.Replace('o', '0');
    REQUIRE(std::strlen(str.GetData()) == str.GetSize());
}
