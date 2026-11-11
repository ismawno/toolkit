#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace TKit;
using namespace TKit::Alias;

static ArenaAllocator s_Arena{10_kib};
static StackAllocator s_Stack{10_kib};
static TierAllocator s_Tier{{.MaxAllocation = 16_kib}};

// Internal linkage: every test file defines these, each with its own allocators
namespace
{
// Allocator backed containers take their allocator from the pushed ones, so every test case pushes this file's
struct PushedAllocators
{
    PushedAllocators()
    {
        PushArena(&s_Arena);
        PushStack(&s_Stack);
        PushTier(&s_Tier);
    }
    ~PushedAllocators()
    {
        PopTier();
        PopStack();
        PopArena();
    }
};

// Default constructs and reserves the given capacity, if any (static allocations get none)
template <typename C, typename... Capacity> C Reserved(const Capacity... capacity)
{
    C c{};
    (c.Reserve(capacity), ...);
    return c;
}

template <typename S, typename... Capacity> S MakeString(const char *str, const Capacity... capacity)
{
    S s = Reserved<S>(capacity...);
    s += str;
    return s;
}
} // namespace

template <typename T> using StaticAlloc64 = StaticAllocation<T, 64>;

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestStartsWithStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("Hello, world!", args...);
    REQUIRE(str.StartsWith("Hello"));
    REQUIRE(str.StartsWith("Hello, world!"));
    REQUIRE_FALSE(str.StartsWith("world"));
    REQUIRE_FALSE(str.StartsWith("Hello, world! and more"));
    REQUIRE(str.StartsWith(""));
}

template <template <typename> typename A, typename... Args> void TestStartsWithChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("Hello", args...);
    REQUIRE(str.StartsWith('H'));
    REQUIRE_FALSE(str.StartsWith('h'));

    const Str empty = MakeString<Str>("", args...);
    REQUIRE_FALSE(empty.StartsWith('H'));
}

template <template <typename> typename A, typename... Args> void TestEndsWithStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("Hello, world!", args...);
    REQUIRE(str.EndsWith("world!"));
    REQUIRE(str.EndsWith("Hello, world!"));
    REQUIRE_FALSE(str.EndsWith("Hello"));
    REQUIRE_FALSE(str.EndsWith("much longer than the string itself"));
    REQUIRE(str.EndsWith(""));
}

template <template <typename> typename A, typename... Args> void TestEndsWithChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("Hello!", args...);
    REQUIRE(str.EndsWith('!'));
    REQUIRE_FALSE(str.EndsWith('o'));

    const Str empty = MakeString<Str>("", args...);
    REQUIRE_FALSE(empty.EndsWith('!'));
}

template <template <typename> typename A, typename... Args> void TestContains([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("abcdef", args...);
    REQUIRE(str.Contains("cde"));
    REQUIRE(str.Contains("abcdef"));
    REQUIRE(str.Contains('a'));
    REQUIRE(str.Contains('f'));
    REQUIRE_FALSE(str.Contains("xyz"));
    REQUIRE_FALSE(str.Contains('z'));
}

template <template <typename> typename A, typename... Args> void TestFindChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("abcabc", args...);
    REQUIRE(str.Find('a') == 0);
    REQUIRE(str.Find('a', 1) == 3);
    REQUIRE(str.Find('c') == 2);
    REQUIRE(str.Find('z') == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello world hello", args...);
    REQUIRE(str.Find("hello") == 0);
    REQUIRE(str.Find("hello", 1) == 12);
    REQUIRE(str.Find("world") == 6);
    REQUIRE(str.Find("xyz") == Str::npos);
    REQUIRE(str.Find("") == 0);
}

template <template <typename> typename A, typename... Args> void TestFindLastChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("abcabc", args...);
    REQUIRE(str.FindLast('c') == 5);
    REQUIRE(str.FindLast('a') == 3);
    REQUIRE(str.FindLast('z') == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindLastStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("aaa bbb aaa", args...);
    REQUIRE(str.FindLast("aaa") == 8);
    REQUIRE(str.FindLast("bbb") == 4);
    REQUIRE(str.FindLast("ccc") == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindFirstLastOf([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello world", args...);
    REQUIRE(str.FindFirstOf("aeiou") == 1);
    REQUIRE(str.FindFirstOf("xyz") == Str::npos);
    REQUIRE(str.FindLastOf("aeiou") == 7);
    REQUIRE(str.FindLastOf("xyz") == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestFindFirstNotOf([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("   hello", args...);
    REQUIRE(str.FindFirstNotOf(" ") == 3);

    const Str allSpaces = MakeString<Str>("    ", args...);
    REQUIRE(allSpaces.FindFirstNotOf(" ") == Str::npos);
}

template <template <typename> typename A, typename... Args> void TestSubString([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("Hello, world!", args...);

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

template <template <typename> typename A, typename... Args> void TestCompare([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("abc", args...);
    REQUIRE(str.Compare("abc") == 0);
    REQUIRE(str.Compare("abd") < 0);
    REQUIRE(str.Compare("abb") > 0);
    REQUIRE(str.Compare("ab") > 0);
    REQUIRE(str.Compare("abcd") < 0);

    REQUIRE(str == "abc");
    REQUIRE_FALSE(str == "xyz");
}

template <template <typename> typename A, typename... Args> void TestTrimLeft([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    Str str = MakeString<Str>("   hello   ", args...);
    str.TrimLeft();
    REQUIRE(str == "hello   ");

    Str allSpaces = MakeString<Str>("    ", args...);
    allSpaces.TrimLeft();
    REQUIRE(allSpaces.GetSize() == 0);

    Str noSpaces = MakeString<Str>("hello", args...);
    noSpaces.TrimLeft();
    REQUIRE(noSpaces == "hello");
}

template <template <typename> typename A, typename... Args> void TestTrimRight([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    Str str = MakeString<Str>("   hello   ", args...);
    str.TrimRight();
    REQUIRE(str == "   hello");

    Str allSpaces = MakeString<Str>("    ", args...);
    allSpaces.TrimRight();
    REQUIRE(allSpaces.GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestTrim([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    Str str = MakeString<Str>("  \t hello \n ", args...);
    str.Trim();
    REQUIRE(str == "hello");

    Str custom = MakeString<Str>("xxhelloxx", args...);
    custom.Trim("x");
    REQUIRE(custom == "hello");
}

template <template <typename> typename A, typename... Args> void TestAppendStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    Str str = MakeString<Str>("hello", args...);
    str += ", world!";
    REQUIRE(str == "hello, world!");
    REQUIRE(str.GetSize() == 13);
}

template <template <typename> typename A, typename... Args> void TestAppendChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    Str str = MakeString<Str>("abc", args...);
    str += 'd';
    str += 'e';
    REQUIRE(str == "abcde");
}

template <template <typename> typename A, typename... Args> void TestConcatStrStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str a = MakeString<Str>("hello", args...);
    const Str b = MakeString<Str>(" world", args...);
    const auto result = a + b;
    REQUIRE(result == "hello world");
    REQUIRE(result.GetSize() == 11);
}

template <template <typename> typename A, typename... Args> void TestConcatStrLit([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str a = MakeString<Str>("hello", args...);
    const auto result = a + " world";
    REQUIRE(result == "hello world");
}

template <template <typename> typename A, typename... Args> void TestConcatLitStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str b = MakeString<Str>("world", args...);
    const auto result = "hello " + b;
    REQUIRE(result == "hello world");
}

template <template <typename> typename A, typename... Args> void TestReplaceSelfChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    Str str = MakeString<Str>("a.b.c.d", args...);
    str.Replace('.', '/');
    REQUIRE(str == "a/b/c/d");
}

template <template <typename> typename A, typename... Args> void TestReplaceCharConst([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("a-b-c", args...);
    const auto result = str.Replace('-', '_');
    REQUIRE(str == "a-b-c");
}

template <template <typename> typename A, typename... Args> void TestReplaceStrSameLen([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("foo bar foo", args...);
    const auto result = str.Replace("foo", "baz");
    REQUIRE(result == "baz bar baz");
    REQUIRE(str == "foo bar foo");
}

template <template <typename> typename A, typename... Args> void TestReplaceStrShorter([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("aabbcc", args...);
    const auto result = str.Replace("bb", "x");
    REQUIRE(result == "aaxcc");
}

template <template <typename> typename A, typename... Args> void TestReplaceStrLonger([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("a-b-c", args...);
    const auto result = str.Replace("-", "---");
    REQUIRE(result == "a---b---c");
}

template <template <typename> typename A, typename... Args> void TestReplaceEmptyFrom([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello", args...);
    const auto result = str.Replace("", "x");
    REQUIRE(result == "hello");
}

template <template <typename> typename A, typename... Args> void TestReplaceNoMatch([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello", args...);
    const auto result = str.Replace("xyz", "abc");
    REQUIRE(result == "hello");
}

template <template <typename> typename A, typename... Args> void TestReplaceDeletion([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello world", args...);
    const auto result = str.Replace(" ", "");
    REQUIRE(result == "helloworld");
}

template <template <typename> typename A, typename... Args> void TestSplitChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("a,b,c,d", args...);
    REQUIRE(str.GetSize() == 7);
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
    REQUIRE(parts[3] == "d");
}

template <template <typename> typename A, typename... Args> void TestSplitCharNoDelim([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello", args...);
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0] == "hello");
}

template <template <typename> typename A, typename... Args> void TestSplitCharEdges([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>(",a,b,", args...);
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0].GetSize() == 0);
    REQUIRE(parts[1] == "a");
    REQUIRE(parts[2] == "b");
    REQUIRE(parts[3].GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestSplitCharConsecutive([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("a,,b", args...);
    const auto parts = str.Split(',');
    REQUIRE(parts.GetSize() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1].GetSize() == 0);
    REQUIRE(parts[2] == "b");
}

template <template <typename> typename A, typename... Args> void TestSplitStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("one::two::three", args...);
    const auto parts = str.Split("::");
    REQUIRE(parts.GetSize() == 3);
    REQUIRE(parts[0] == "one");
    REQUIRE(parts[1] == "two");
    REQUIRE(parts[2] == "three");
}

template <template <typename> typename A, typename... Args> void TestSplitStrEmpty([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello", args...);
    const auto parts = str.Split("");
    REQUIRE(parts.GetSize() == 1);
    REQUIRE(parts[0] == "hello");
}

template <template <typename> typename A, typename... Args> void TestSplitStrEdges([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("<>a<>b<>", args...);
    const auto parts = str.Split("<>");
    REQUIRE(parts.GetSize() == 4);
    REQUIRE(parts[0].GetSize() == 0);
    REQUIRE(parts[1] == "a");
    REQUIRE(parts[2] == "b");
    REQUIRE(parts[3].GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestCountChar([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("banana", args...);
    REQUIRE(str.Count('a') == 3);
    REQUIRE(str.Count('n') == 2);
    REQUIRE(str.Count('b') == 1);
    REQUIRE(str.Count('z') == 0);
}

template <template <typename> typename A, typename... Args> void TestCountStr([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("abababab", args...);
    REQUIRE(str.Count("ab") == 4);
    REQUIRE(str.Count("aba") == 2);
    REQUIRE(str.Count("xyz") == 0);
    REQUIRE(str.Count("") == 0);
}

template <template <typename> typename A, typename... Args> void TestConstruction([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str str = MakeString<Str>("hello", args...);
    REQUIRE(str.GetSize() == 5);
    REQUIRE(str == "hello");
    REQUIRE(std::strlen(str.CString()) == 5);
}

template <template <typename> typename A, typename... Args> void TestEmptyString([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    const Str empty = MakeString<Str>("", args...);
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

template <template <typename> typename A, typename... Args> void TestNullTermination([[maybe_unused]] const Args... args)
{
    using Str = Array<char, A<char>>;
    Str str = MakeString<Str>("hello", args...);
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
    Fn<ArenaAllocation>(usize(__VA_ARGS__));                                                                 \
    Fn<DynamicAllocation>(usize(__VA_ARGS__));                                                                         \
    Fn<StackAllocation>(usize(__VA_ARGS__));                                                                 \
    Fn<StaticAlloc64>();                                                                                               \
    Fn<TierAllocation>(usize(__VA_ARGS__))

// ─── StartsWith / EndsWith ──────────────────────────────────────────

TEST_CASE("String: StartsWith(const char*)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestStartsWithStr, 32);
}
TEST_CASE("String: StartsWith(char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestStartsWithChar, 16);
}
TEST_CASE("String: EndsWith(const char*)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestEndsWithStr, 32);
}
TEST_CASE("String: EndsWith(char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestEndsWithChar, 16);
}

// ─── Contains ───────────────────────────────────────────────────────

TEST_CASE("String: Contains", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestContains, 16);
}

// ─── Find ───────────────────────────────────────────────────────────

TEST_CASE("String: Find(char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestFindChar, 16);
}
TEST_CASE("String: Find(const char*)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestFindStr, 32);
}
TEST_CASE("String: FindLast(char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestFindLastChar, 16);
}
TEST_CASE("String: FindLast(const char*)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestFindLastStr, 16);
}
TEST_CASE("String: FindFirstOf / FindLastOf", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestFindFirstLastOf, 16);
}
TEST_CASE("String: FindFirstNotOf", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestFindFirstNotOf, 16);
}

// ─── SubString ──────────────────────────────────────────────────────

TEST_CASE("String: SubString", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSubString, 32);
}

// ─── Compare / operator== ───────────────────────────────────────────

TEST_CASE("String: Compare and operator==", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestCompare, 16);
}

// ─── Trim ───────────────────────────────────────────────────────────

TEST_CASE("String: TrimLeft", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestTrimLeft, 16);
}
TEST_CASE("String: TrimRight", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestTrimRight, 16);
}
TEST_CASE("String: Trim", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestTrim, 16);
}

// ─── operator+= ────────────────────────────────────────────────────

TEST_CASE("String: operator+=(const char*)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestAppendStr, 32);
}
TEST_CASE("String: operator+=(char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestAppendChar, 16);
}

// ─── operator+ (concatenation) ──────────────────────────────────────

TEST_CASE("String: String + String", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestConcatStrStr, 16);
}
TEST_CASE("String: String + const char*", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestConcatStrLit, 16);
}
TEST_CASE("String: const char* + String", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestConcatLitStr, 16);
}

// ─── Replace ────────────────────────────────────────────────────────

TEST_CASE("String: ReplaceSelf(char, char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestReplaceSelfChar, 16);
}
TEST_CASE("String: Replace(char, char) const", "[String]")
{
    const PushedAllocators pushed{};
    TestReplaceCharConst<ArenaAllocation>(usize(16));
    TestReplaceCharConst<DynamicAllocation>(usize(16));
    // TestReplaceCharConst<StackAllocation>(usize(16));
    TestReplaceCharConst<StaticAlloc64>();
    TestReplaceCharConst<TierAllocation>(usize(16));
}
TEST_CASE("String: Replace(const char*, const char*) same length", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestReplaceStrSameLen, 16);
}
TEST_CASE("String: Replace(const char*, const char*) shorter replacement", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestReplaceStrShorter, 16);
}
TEST_CASE("String: Replace(const char*, const char*) longer replacement", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestReplaceStrLonger, 16);
}
TEST_CASE("String: Replace with empty from returns copy", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestReplaceEmptyFrom, 16);
}
TEST_CASE("String: Replace with no matches returns copy", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestReplaceNoMatch, 16);
}
TEST_CASE("String: Replace to empty (deletion)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestReplaceDeletion, 16);
}

// ─── Split ──────────────────────────────────────────────────────────

TEST_CASE("String: Split(char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSplitChar, 16);
}
TEST_CASE("String: Split(char) no delimiter present", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSplitCharNoDelim, 16);
}
TEST_CASE("String: Split(char) delimiter at edges", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSplitCharEdges, 16);
}
TEST_CASE("String: Split(char) consecutive delimiters", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSplitCharConsecutive, 16);
}
TEST_CASE("String: Split(const char*)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSplitStr, 32);
}
TEST_CASE("String: Split(const char*) empty delimiter returns whole string", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSplitStrEmpty, 16);
}
TEST_CASE("String: Split(const char*) multi-char at edges", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestSplitStrEdges, 16);
}

// ─── Count ──────────────────────────────────────────────────────────

TEST_CASE("String: Count(char)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestCountChar, 16);
}
TEST_CASE("String: Count(const char*)", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestCountStr, 16);
}

// ─── Format (tier-specific) ─────────────────────────────────────────

TEST_CASE("String: Format", "[String]")
{
    const PushedAllocators pushed{};
    PushTier(&s_Tier);
    const auto str = TierString::Format("Hello, {}! x={}", "world", 42);
    REQUIRE(str == "Hello, world! x=42");
    REQUIRE(str.GetSize() == 18);
    PopTier();
}

// ─── Edge cases ─────────────────────────────────────────────────────

TEST_CASE("String: construction from const char*", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestConstruction, 16);
}
TEST_CASE("String: empty string operations", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestEmptyString, 16);
}
TEST_CASE("String: null termination preserved after mutations", "[String]")
{
    const PushedAllocators pushed{};
    STRING_TEST_ALL(TestNullTermination, 32);
}

#undef STRING_TEST_ALL
