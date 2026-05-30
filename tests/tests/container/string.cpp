#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

static ArenaAllocator s_Arena{10_kib};
static StackAllocator s_Stack{10_kib};
static TierAllocator s_Tier{{.Allocator = &s_Arena, .MaxAllocation = 16_kib}};

template <typename T> using StaticAlloc64 = StaticAllocation<T, 64>;

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestStartsWithStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"Hello, world!", A<char>{args...}};
    REQUIRE(str.StartsWith("Hello"));
    REQUIRE(str.StartsWith("Hello, world!"));
    REQUIRE_FALSE(str.StartsWith("world"));
    REQUIRE_FALSE(str.StartsWith("Hello, world! and more"));
    REQUIRE(str.StartsWith(""));
}

template <template <typename> typename A, typename... Args> void TestStartsWithChar(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"Hello", A<char>{args...}};
    REQUIRE(str.StartsWith('H'));
    REQUIRE_FALSE(str.StartsWith('h'));

    const Str empty{"", A<char>{args...}};
    REQUIRE_FALSE(empty.StartsWith('H'));
}

template <template <typename> typename A, typename... Args> void TestEndsWithStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"Hello, world!", A<char>{args...}};
    REQUIRE(str.EndsWith("world!"));
    REQUIRE(str.EndsWith("Hello, world!"));
    REQUIRE_FALSE(str.EndsWith("Hello"));
    REQUIRE_FALSE(str.EndsWith("much longer than the string itself"));
    REQUIRE(str.EndsWith(""));
}

template <template <typename> typename A, typename... Args> void TestEndsWithChar(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"Hello!", A<char>{args...}};
    REQUIRE(str.EndsWith('!'));
    REQUIRE_FALSE(str.EndsWith('o'));

    const Str empty{"", A<char>{args...}};
    REQUIRE_FALSE(empty.EndsWith('!'));
}

template <template <typename> typename A, typename... Args> void TestContains(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"abcdef", A<char>{args...}};
    REQUIRE(str.Contains("cde"));
    REQUIRE(str.Contains("abcdef"));
    REQUIRE(str.Contains('a'));
    REQUIRE(str.Contains('f'));
    REQUIRE_FALSE(str.Contains("xyz"));
    REQUIRE_FALSE(str.Contains('z'));
}

template <template <typename> typename A, typename... Args> void TestFindChar(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"abcabc", A<char>{args...}};
    REQUIRE(str.Find('a') == 0);
    REQUIRE(str.Find('a', 1) == 3);
    REQUIRE(str.Find('c') == 2);
    REQUIRE(str.Find('z') == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello world hello", A<char>{args...}};
    REQUIRE(str.Find("hello") == 0);
    REQUIRE(str.Find("hello", 1) == 12);
    REQUIRE(str.Find("world") == 6);
    REQUIRE(str.Find("xyz") == Str::npos);
    REQUIRE(str.Find("") == 0);
}

template <template <typename> typename A, typename... Args> void TestFindLastChar(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"abcabc", A<char>{args...}};
    REQUIRE(str.FindLast('c') == 5);
    REQUIRE(str.FindLast('a') == 3);
    REQUIRE(str.FindLast('z') == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindLastStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"aaa bbb aaa", A<char>{args...}};
    REQUIRE(str.FindLast("aaa") == 8);
    REQUIRE(str.FindLast("bbb") == 4);
    REQUIRE(str.FindLast("ccc") == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindFirstLastOf(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello world", A<char>{args...}};
    REQUIRE(str.FindFirstOf("aeiou") == 1);
    REQUIRE(str.FindFirstOf("xyz") == Str::npos);
    REQUIRE(str.FindLastOf("aeiou") == 7);
    REQUIRE(str.FindLastOf("xyz") == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindFirstNotOf(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"   hello", A<char>{args...}};
    REQUIRE(str.FindFirstNotOf(" ") == 3);

    const Str allSpaces{"    ", A<char>{args...}};
    REQUIRE(allSpaces.FindFirstNotOf(" ") == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestSubString(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"Hello, world!", A<char>{args...}};

    const auto sub1 = str.SubString(7);
    REQUIRE(sub1.GetSize() == 6);
    REQUIRE(sub1 == "world!");

    const auto sub2 = str.SubString(0, 5);
    REQUIRE(sub2.GetSize() == 5);
    REQUIRE(sub2 == "Hello");

    const auto sub3 = str.SubString(7, 5);
    REQUIRE(sub3.GetSize() == 5);
    REQUIRE(sub3 == "world");

    const auto sub4 = str.SubString(str.GetSize());
    REQUIRE(sub4.GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestCompare(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"abc", A<char>{args...}};
    REQUIRE(str.Compare("abc") == 0);
    REQUIRE(str.Compare("abd") < 0);
    REQUIRE(str.Compare("abb") > 0);
    REQUIRE(str.Compare("ab") > 0);
    REQUIRE(str.Compare("abcd") < 0);

    REQUIRE(str == "abc");
    REQUIRE_FALSE(str == "xyz");
}

template <template <typename> typename A, typename... Args> void TestTrimLeft(Args... args)
{
    using Str = Array<char, A<char>>;
    Str str{"   hello   ", A<char>{args...}};
    str.TrimLeft();
    REQUIRE(str == "hello   ");

    Str allSpaces{"    ", A<char>{args...}};
    allSpaces.TrimLeft();
    REQUIRE(allSpaces.GetSize() == 0);

    Str noSpaces{"hello", A<char>{args...}};
    noSpaces.TrimLeft();
    REQUIRE(noSpaces == "hello");
}

template <template <typename> typename A, typename... Args> void TestTrimRight(Args... args)
{
    using Str = Array<char, A<char>>;
    Str str{"   hello   ", A<char>{args...}};
    str.TrimRight();
    REQUIRE(str == "   hello");

    Str allSpaces{"    ", A<char>{args...}};
    allSpaces.TrimRight();
    REQUIRE(allSpaces.GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestTrim(Args... args)
{
    using Str = Array<char, A<char>>;
    Str str{"  \t hello \n ", A<char>{args...}};
    str.Trim();
    REQUIRE(str == "hello");

    Str custom{"xxhelloxx", A<char>{args...}};
    custom.Trim("x");
    REQUIRE(custom == "hello");
}

template <template <typename> typename A, typename... Args> void TestAppendStr(Args... args)
{
    using Str = Array<char, A<char>>;
    Str str{"hello", A<char>{args...}};
    str += ", world!";
    REQUIRE(str == "hello, world!");
    REQUIRE(str.GetSize() == 13);
}

template <template <typename> typename A, typename... Args> void TestAppendChar(Args... args)
{
    using Str = Array<char, A<char>>;
    Str str{"abc", A<char>{args...}};
    str += 'd';
    str += 'e';
    REQUIRE(str == "abcde");
}

template <template <typename> typename A, typename... Args> void TestConcatStrStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str a{"hello", A<char>{args...}};
    const Str b{" world", A<char>{args...}};
    const auto result = a + b;
    REQUIRE(result == "hello world");
    REQUIRE(result.GetSize() == 11);
}

template <template <typename> typename A, typename... Args> void TestConcatStrLit(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str a{"hello", A<char>{args...}};
    const auto result = a + " world";
    REQUIRE(result == "hello world");
}

template <template <typename> typename A, typename... Args> void TestConcatLitStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str b{"world", A<char>{args...}};
    const auto result = "hello " + b;
    REQUIRE(result == "hello world");
}

template <template <typename> typename A, typename... Args> void TestReplaceSelfChar(Args... args)
{
    using Str = Array<char, A<char>>;
    Str str{"a.b.c.d", A<char>{args...}};
    str.Replace('.', '/');
    REQUIRE(str == "a/b/c/d");
}

template <template <typename> typename A, typename... Args> void TestReplaceCharConst(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"a-b-c", A<char>{args...}};
    const auto result = str.Replace('-', '_');
    REQUIRE(str == "a-b-c");
}

template <template <typename> typename A, typename... Args> void TestReplaceStrSameLen(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"foo bar foo", A<char>{args...}};
    const auto result = str.Replace("foo", "baz");
    REQUIRE(result == "baz bar baz");
    REQUIRE(str == "foo bar foo");
}

template <template <typename> typename A, typename... Args> void TestReplaceStrShorter(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"aabbcc", A<char>{args...}};
    const auto result = str.Replace("bb", "x");
    REQUIRE(result == "aaxcc");
}

template <template <typename> typename A, typename... Args> void TestReplaceStrLonger(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"a-b-c", A<char>{args...}};
    const auto result = str.Replace("-", "---");
    REQUIRE(result == "a---b---c");
}

template <template <typename> typename A, typename... Args> void TestReplaceEmptyFrom(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello", A<char>{args...}};
    const auto result = str.Replace("", "x");
    REQUIRE(result == "hello");
}

template <template <typename> typename A, typename... Args> void TestReplaceNoMatch(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello", A<char>{args...}};
    const auto result = str.Replace("xyz", "abc");
    REQUIRE(result == "hello");
}

template <template <typename> typename A, typename... Args> void TestReplaceDeletion(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello world", A<char>{args...}};
    const auto result = str.Replace(" ", "");
    REQUIRE(result == "helloworld");
}

template <template <typename> typename A, typename... Args> void TestSplitChar(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"a,b,c,d", A<char>{args...}};
    REQUIRE(str.GetSize() == 7);
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
    REQUIRE(parts[3] == "d");
}

template <template <typename> typename A, typename... Args> void TestSplitCharNoDelim(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello", A<char>{args...}};
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0] == "hello");
}

template <template <typename> typename A, typename... Args> void TestSplitCharEdges(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{",a,b,", A<char>{args...}};
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0].GetSize() == 0);
    REQUIRE(parts[1] == "a");
    REQUIRE(parts[2] == "b");
    REQUIRE(parts[3].GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestSplitCharConsecutive(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"a,,b", A<char>{args...}};
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1].GetSize() == 0);
    REQUIRE(parts[2] == "b");
}

template <template <typename> typename A, typename... Args> void TestSplitStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"one::two::three", A<char>{args...}};
    const auto parts = str.Split("::");
    REQUIRE(parts.GetSize() == 3);
    REQUIRE(parts[0] == "one");
    REQUIRE(parts[1] == "two");
    REQUIRE(parts[2] == "three");
}

template <template <typename> typename A, typename... Args> void TestSplitStrEmpty(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello", A<char>{args...}};
    const auto parts = str.Split("");
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0] == "hello");
}

template <template <typename> typename A, typename... Args> void TestSplitStrEdges(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"<>a<>b<>", A<char>{args...}};
    const auto parts = str.Split("<>");
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0].GetSize() == 0);
    REQUIRE(parts[1] == "a");
    REQUIRE(parts[2] == "b");
    REQUIRE(parts[3].GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestCountChar(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"banana", A<char>{args...}};
    REQUIRE(str.Count('a') == 3);
    REQUIRE(str.Count('n') == 2);
    REQUIRE(str.Count('b') == 1);
    REQUIRE(str.Count('z') == 0);
}

template <template <typename> typename A, typename... Args> void TestCountStr(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"abababab", A<char>{args...}};
    REQUIRE(str.Count("ab") == 4);
    REQUIRE(str.Count("aba") == 2);
    REQUIRE(str.Count("xyz") == 0);
    REQUIRE(str.Count("") == 0);
}

template <template <typename> typename A, typename... Args> void TestConstruction(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str{"hello", A<char>{args...}};
    REQUIRE(str.GetSize() == 5);
    REQUIRE(str == "hello");
    REQUIRE(std::strlen(str.CString()) == 5);
}

template <template <typename> typename A, typename... Args> void TestEmptyString(Args... args)
{
    using Str = Array<char, A<char>>;
    const Str empty{"", A<char>{args...}};
    REQUIRE(empty.GetSize() == 0);
    REQUIRE_FALSE(empty.Contains('a'));
    REQUIRE_FALSE(empty.StartsWith('a'));
    REQUIRE_FALSE(empty.EndsWith('a'));
    REQUIRE(empty.Find('a') == Str::npos);
    REQUIRE(empty.Count('a') == 0);

    const auto parts = empty.Split(',');
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0].GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestNullTermination(Args... args)
{
    using Str = Array<char, A<char>>;
    Str str{"hello", A<char>{args...}};
    str += " world";
    REQUIRE(std::strlen(str.CString()) == str.GetSize());

    str.TrimRight();
    REQUIRE(std::strlen(str.CString()) == str.GetSize());

    str.Replace('o', '0');
    REQUIRE(std::strlen(str.CString()) == str.GetSize());
}

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

#define STRING_TEST_ALL(Fn, ...)                                                                                       \
    Fn<ArenaAllocation>(&s_Arena, usize(__VA_ARGS__));                                                                 \
    Fn<DynamicAllocation>(usize(__VA_ARGS__));                                                                         \
    Fn<StackAllocation>(&s_Stack, usize(__VA_ARGS__));                                                                 \
    Fn<StaticAlloc64>();                                                                                               \
    Fn<TierAllocation>(&s_Tier, usize(__VA_ARGS__))

// ─── StartsWith / EndsWith ──────────────────────────────────────────

TEST_CASE("String: StartsWith(const char*)", "[String]")
{
    STRING_TEST_ALL(TestStartsWithStr, 32);
}
TEST_CASE("String: StartsWith(char)", "[String]")
{
    STRING_TEST_ALL(TestStartsWithChar, 16);
}
TEST_CASE("String: EndsWith(const char*)", "[String]")
{
    STRING_TEST_ALL(TestEndsWithStr, 32);
}
TEST_CASE("String: EndsWith(char)", "[String]")
{
    STRING_TEST_ALL(TestEndsWithChar, 16);
}

// ─── Contains ───────────────────────────────────────────────────────

TEST_CASE("String: Contains", "[String]")
{
    STRING_TEST_ALL(TestContains, 16);
}

// ─── Find ───────────────────────────────────────────────────────────

TEST_CASE("String: Find(char)", "[String]")
{
    STRING_TEST_ALL(TestFindChar, 16);
}
TEST_CASE("String: Find(const char*)", "[String]")
{
    STRING_TEST_ALL(TestFindStr, 32);
}
TEST_CASE("String: FindLast(char)", "[String]")
{
    STRING_TEST_ALL(TestFindLastChar, 16);
}
TEST_CASE("String: FindLast(const char*)", "[String]")
{
    STRING_TEST_ALL(TestFindLastStr, 16);
}
TEST_CASE("String: FindFirstOf / FindLastOf", "[String]")
{
    STRING_TEST_ALL(TestFindFirstLastOf, 16);
}
TEST_CASE("String: FindFirstNotOf", "[String]")
{
    STRING_TEST_ALL(TestFindFirstNotOf, 16);
}

// ─── SubString ──────────────────────────────────────────────────────

TEST_CASE("String: SubString", "[String]")
{
    STRING_TEST_ALL(TestSubString, 32);
}

// ─── Compare / operator== ───────────────────────────────────────────

TEST_CASE("String: Compare and operator==", "[String]")
{
    STRING_TEST_ALL(TestCompare, 16);
}

// ─── Trim ───────────────────────────────────────────────────────────

TEST_CASE("String: TrimLeft", "[String]")
{
    STRING_TEST_ALL(TestTrimLeft, 16);
}
TEST_CASE("String: TrimRight", "[String]")
{
    STRING_TEST_ALL(TestTrimRight, 16);
}
TEST_CASE("String: Trim", "[String]")
{
    STRING_TEST_ALL(TestTrim, 16);
}

// ─── operator+= ────────────────────────────────────────────────────

TEST_CASE("String: operator+=(const char*)", "[String]")
{
    STRING_TEST_ALL(TestAppendStr, 32);
}
TEST_CASE("String: operator+=(char)", "[String]")
{
    STRING_TEST_ALL(TestAppendChar, 16);
}

// ─── operator+ (concatenation) ──────────────────────────────────────

TEST_CASE("String: String + String", "[String]")
{
    STRING_TEST_ALL(TestConcatStrStr, 16);
}
TEST_CASE("String: String + const char*", "[String]")
{
    STRING_TEST_ALL(TestConcatStrLit, 16);
}
TEST_CASE("String: const char* + String", "[String]")
{
    STRING_TEST_ALL(TestConcatLitStr, 16);
}

// ─── Replace ────────────────────────────────────────────────────────

TEST_CASE("String: ReplaceSelf(char, char)", "[String]")
{
    STRING_TEST_ALL(TestReplaceSelfChar, 16);
}
TEST_CASE("String: Replace(char, char) const", "[String]")
{
    TestReplaceCharConst<ArenaAllocation>(&s_Arena, usize(16));
    TestReplaceCharConst<DynamicAllocation>(usize(16));
    // TestReplaceCharConst<StackAllocation>(&s_Stack, usize(16));
    TestReplaceCharConst<StaticAlloc64>();
    TestReplaceCharConst<TierAllocation>(&s_Tier, usize(16));
}
TEST_CASE("String: Replace(const char*, const char*) same length", "[String]")
{
    STRING_TEST_ALL(TestReplaceStrSameLen, 16);
}
TEST_CASE("String: Replace(const char*, const char*) shorter replacement", "[String]")
{
    STRING_TEST_ALL(TestReplaceStrShorter, 16);
}
TEST_CASE("String: Replace(const char*, const char*) longer replacement", "[String]")
{
    STRING_TEST_ALL(TestReplaceStrLonger, 16);
}
TEST_CASE("String: Replace with empty from returns copy", "[String]")
{
    STRING_TEST_ALL(TestReplaceEmptyFrom, 16);
}
TEST_CASE("String: Replace with no matches returns copy", "[String]")
{
    STRING_TEST_ALL(TestReplaceNoMatch, 16);
}
TEST_CASE("String: Replace to empty (deletion)", "[String]")
{
    STRING_TEST_ALL(TestReplaceDeletion, 16);
}

// ─── Split ──────────────────────────────────────────────────────────

TEST_CASE("String: Split(char)", "[String]")
{
    STRING_TEST_ALL(TestSplitChar, 16);
}
TEST_CASE("String: Split(char) no delimiter present", "[String]")
{
    STRING_TEST_ALL(TestSplitCharNoDelim, 16);
}
TEST_CASE("String: Split(char) delimiter at edges", "[String]")
{
    STRING_TEST_ALL(TestSplitCharEdges, 16);
}
TEST_CASE("String: Split(char) consecutive delimiters", "[String]")
{
    STRING_TEST_ALL(TestSplitCharConsecutive, 16);
}
TEST_CASE("String: Split(const char*)", "[String]")
{
    STRING_TEST_ALL(TestSplitStr, 32);
}
TEST_CASE("String: Split(const char*) empty delimiter returns whole string", "[String]")
{
    STRING_TEST_ALL(TestSplitStrEmpty, 16);
}
TEST_CASE("String: Split(const char*) multi-char at edges", "[String]")
{
    STRING_TEST_ALL(TestSplitStrEdges, 16);
}

// ─── Count ──────────────────────────────────────────────────────────

TEST_CASE("String: Count(char)", "[String]")
{
    STRING_TEST_ALL(TestCountChar, 16);
}
TEST_CASE("String: Count(const char*)", "[String]")
{
    STRING_TEST_ALL(TestCountStr, 16);
}

// ─── Format (tier-specific) ─────────────────────────────────────────

TEST_CASE("String: Format", "[String]")
{
    PushTier(&s_Tier);
    const auto str = TierString::Format("Hello, {}! x={}", "world", 42);
    REQUIRE(str == "Hello, world! x=42");
    REQUIRE(str.GetSize() == 18);
    PopTier();
}

// ─── Edge cases ─────────────────────────────────────────────────────

TEST_CASE("String: construction from const char*", "[String]")
{
    STRING_TEST_ALL(TestConstruction, 16);
}
TEST_CASE("String: empty string operations", "[String]")
{
    STRING_TEST_ALL(TestEmptyString, 16);
}
TEST_CASE("String: null termination preserved after mutations", "[String]")
{
    STRING_TEST_ALL(TestNullTermination, 32);
}

#undef STRING_TEST_ALL
