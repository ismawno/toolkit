#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

static u32 g_Constructions = 0;
static u32 g_Destructions = 0;
static ArenaAllocator s_Arena{1_mib};
static StackAllocator s_Stack{1_mib};
static TierAllocator s_Tier{{.Allocator = &s_Arena, .MaxAllocation = 16_kib}};

struct Test_Tracker
{
    u32 Value;
    Test_Tracker() : Value(0)
    {
        ++g_Constructions;
    }
    Test_Tracker(const u32 value) : Value(value)
    {
        ++g_Constructions;
    }
    Test_Tracker(const Test_Tracker &other) : Value(other.Value)
    {
        ++g_Constructions;
    }
    Test_Tracker(Test_Tracker &&other) : Value(other.Value)
    {
        ++g_Constructions;
    }
    ~Test_Tracker()
    {
        ++g_Destructions;
    }
    Test_Tracker &operator=(const Test_Tracker &other)
    {
        if (this != &other)
        {
            Value = other.Value;
        }
        return *this;
    }
    Test_Tracker &operator=(Test_Tracker &&other)
    {
        if (this != &other)
        {
            Value = other.Value;
        }
        return *this;
    }
};

template <typename T> using StaticAlloc4 = StaticAllocation<T, 4>;
template <typename T> using StaticAlloc5 = StaticAllocation<T, 5>;
template <typename T> using StaticAlloc6 = StaticAllocation<T, 6>;
template <typename T> using StaticAlloc7 = StaticAllocation<T, 7>;
template <typename T> using StaticAlloc15 = StaticAllocation<T, 15>;

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestBasic(Args... args)
{
    Array<u32, A<u32>> arr{A<u32>{args...}};
    REQUIRE(arr.GetCapacity() == 4);
    REQUIRE(arr.GetSize() == 0);
    REQUIRE(arr.IsEmpty());
    REQUIRE(!arr.IsFull());

    arr.Append(10) = 15;
    arr.Append(20);
    arr.Append(30);
    arr.Append(40);
    REQUIRE(arr.GetSize() == 4);
    REQUIRE(arr.IsFull());
    REQUIRE_FALSE(arr.IsEmpty());

    REQUIRE(arr[0] == 15);
    REQUIRE(arr[3] == 40);
    REQUIRE(arr.GetFront() == 15);
    REQUIRE(arr.GetBack() == 40);
}

template <template <typename> typename A, typename... Args> void TestAppendPop(Args... args)
{
    Array<Test_Tracker, A<Test_Tracker>> arr{A<Test_Tracker>{args...}};
    g_Constructions = g_Destructions = 0;

    auto &r0 = arr.Append();
    REQUIRE(arr.GetSize() == 1);
    REQUIRE(g_Constructions == 1);
    r0.Value = 7;

    const auto &r1 = arr.Append(13u);
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(g_Constructions == 2);
    REQUIRE(r1.Value == 13);

    arr.Pop();
    REQUIRE(arr.GetSize() == 1);
    REQUIRE(g_Destructions == 1);

    arr.Pop();
    REQUIRE(arr.GetSize() == 0);
    REQUIRE(g_Destructions == 2);
}

template <template <typename> typename A, typename... Args> void TestSizeFillCtor(Args... args)
{
    const Array<u32, A<u32>> arr{3, A<u32>{args...}};
    REQUIRE(arr.GetSize() == 3);

    g_Constructions = g_Destructions = 0;
    const Array<Test_Tracker, A<Test_Tracker>> nt{2, A<Test_Tracker>{args...}};
    REQUIRE(nt.GetSize() == 2);
    REQUIRE(g_Constructions == 2);
}

template <template <typename> typename A, typename... Args> void TestInitListRange(Args... args)
{
    const Array<u32, A<u32>> arr{{5u, 6u, 7u}, A<u32>{args...}};
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(std::equal(arr.begin(), arr.end(), FixedArray<u32, 3>{5, 6, 7}.begin()));

    const FixedArray<u32, 4> src = {10, 20, 30, 40};
    const Array<u32, A<u32>> rg(src.begin() + 1, src.begin() + 4, A<u32>{args...});
    REQUIRE(rg.GetSize() == 3);
    REQUIRE(rg[0] == 20);
    REQUIRE(rg[2] == 40);
}

template <template <typename> typename A, typename... Args> void TestCopyMove(Args... args)
{
    const Array<u32, A<u32>> arr1{{1, 2, 3}, A<u32>{args...}};
    Array<u32, A<u32>> arr2 = arr1;
    REQUIRE(arr2.GetSize() == 3);
    REQUIRE(std::equal(arr2.begin(), arr2.end(), arr1.begin()));

    const Array<u32, A<u32>> arr3 = std::move(arr2);
    REQUIRE(arr3.GetSize() == 3);
    REQUIRE(arr3[0] == 1);

    Array<u32, A<u32>> arr4{A<u32>{args...}};
    arr4 = arr3;
    REQUIRE(arr4.GetSize() == 3);
    REQUIRE(arr4[1] == 2);

    Array<u32, A<u32>> arr5{A<u32>{args...}};
    arr5 = std::move(arr4);
    REQUIRE(arr5.GetSize() == 3);
    REQUIRE(arr5[2] == 3);
}

template <template <typename> typename A, typename... Args> void TestInsert(Args... args)
{
    Array<u32, A<u32>> arr{{1, 2, 4, 5}, A<u32>{args...}};
    u32 &ref = arr.Insert(arr.begin() + 2, 3u);
    REQUIRE(arr.GetSize() == 5);
    REQUIRE(ref == 3);

    FixedArray<u32, 2> extra = {7, 8};
    arr.Insert(arr.begin() + 5, extra.begin(), extra.end());
    REQUIRE(arr.GetSize() == 7);
}

template <template <typename> typename A, typename... Args> void TestRemove(Args... args)
{
    Array<u32, A<u32>> arr{{10, 20, 30, 40, 50}, A<u32>{args...}};
    REQUIRE(arr.GetSize() == 5);

    arr.RemoveOrdered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 4);

    arr.RemoveOrdered(arr.begin() + 1, arr.begin() + 3);
    REQUIRE(arr.GetSize() == 2);
    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 50);

    if constexpr (A<u32>::Type == Array_Stack)
    {
        arr.Clear();
        arr.Deallocate();
    }
    arr = Array<u32, A<u32>>{{1, 2, 3, 4}, A<u32>{args...}};
    arr.RemoveUnordered(arr.begin() + 1);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(arr[1] == 4);
}

template <template <typename> typename A, typename... Args> void TestResize(Args... args)
{
    Array<Test_Tracker, A<Test_Tracker>> arr{A<Test_Tracker>{args...}};
    g_Constructions = g_Destructions = 0;

    arr.Resize(3);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(g_Constructions == 3);

    arr.Resize(1);
    REQUIRE(arr.GetSize() == 1);
    REQUIRE(g_Destructions == 2);

    arr.Resize(4, 99u);
    REQUIRE(arr.GetSize() == 4);
    REQUIRE(g_Constructions == 3 + 3);
    for (usize i = 1; i < 4; ++i)
        REQUIRE(arr[i].Value == 99u);
}

template <template <typename> typename A, typename... Args> void TestClearIteration(Args... args)
{
    Array<u32, A<u32>> arr1{{9, 8, 7}, A<u32>{args...}};
    arr1.Clear();
    REQUIRE(arr1.GetSize() == 0);
    REQUIRE(arr1.IsEmpty());

    const Array<u32, A<u32>> arr2{{1, 2, 3}, A<u32>{args...}};
    u32 sum = 0;
    for (const u32 x : arr2)
        sum += x;
    REQUIRE(sum == 6);
}

template <template <typename> typename A, typename... Args> void TestStringOps(Args... args)
{
    Array<std::string, A<std::string>> arr1{A<std::string>{args...}};
    REQUIRE(arr1.GetSize() == 0);
    REQUIRE(arr1.IsEmpty());

    arr1.Append("one");
    arr1.Append("two");
    arr1.Append("three");
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(arr1[0] == "one");
    REQUIRE(arr1[1] == "two");
    REQUIRE(arr1[2] == "three");

    {
        auto arr2 = arr1;
        REQUIRE(arr2.GetSize() == 3);
        arr2[1] = "TWO";
        REQUIRE(arr1[1] == "two");
        REQUIRE(arr2[1] == "TWO");

        auto arr3 = std::move(arr2);
        REQUIRE(arr3.GetSize() == 3);
        REQUIRE(arr3[0] == "one");
        arr2.Clear();
        REQUIRE(arr2.GetSize() == 0);
    }

    arr1.Insert(arr1.begin() + 1, std::string("inserted"));
    REQUIRE(arr1.GetSize() == 4);
    REQUIRE(arr1[1] == "inserted");
    REQUIRE(arr1[2] == "two");

    const FixedArray<std::string, 3> extras{"x", "y", "z"};
    arr1.Insert(arr1.begin() + 4, extras.begin(), extras.end());
    REQUIRE(arr1.GetSize() == 7);
    REQUIRE(arr1[4] == "x");
    REQUIRE(arr1[6] == "z");

    arr1.RemoveOrdered(arr1.begin() + 1);
    REQUIRE(arr1.GetSize() == 6);
    REQUIRE(arr1[1] == "two");

    arr1.RemoveOrdered(arr1.begin() + 2, arr1.begin() + 4);
    REQUIRE(arr1.GetSize() == 4);

    if constexpr (A<std::string>::Type == Array_Stack)
    {
        arr1.Clear();
        arr1.Deallocate();
    }
    arr1 = Array<std::string, A<std::string>>{{"A", "B", "C", "D"}, A<std::string>{args...}};
    arr1.RemoveUnordered(arr1.begin() + 1);
    REQUIRE(arr1.GetSize() == 3);
    REQUIRE(arr1[1] != "B");
    REQUIRE((arr1[1] == "D" || arr1[1] == "C" || arr1[1] == "A"));

    arr1.Resize(5, std::string("fill"));
    REQUIRE(arr1.GetSize() == 5);
    REQUIRE(arr1[3] == "fill");
    REQUIRE(arr1[4] == "fill");

    arr1.Resize(2);
    REQUIRE(arr1.GetSize() == 2);

    arr1.Pop();
    REQUIRE(arr1.GetSize() == 1);

    arr1.Clear();
    REQUIRE(arr1.IsEmpty());
}

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

TEST_CASE("Array: basic capacity/size queries", "[Array]")
{
    TestBasic<ArenaAllocation>(&s_Arena, usize(4));
    TestBasic<DynamicAllocation>(usize(4));
    TestBasic<StackAllocation>(&s_Stack, usize(4));
    TestBasic<StaticAlloc4>();
    TestBasic<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Array: Append and Pop", "[Array]")
{
    TestAppendPop<ArenaAllocation>(&s_Arena, usize(4));
    TestAppendPop<DynamicAllocation>(usize(4));
    TestAppendPop<StackAllocation>(&s_Stack, usize(4));
    TestAppendPop<StaticAlloc4>();
    TestAppendPop<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Array: constructor from size+fill args", "[Array]")
{
    TestSizeFillCtor<ArenaAllocation>(&s_Arena, usize(4));
    TestSizeFillCtor<DynamicAllocation>(usize(4));
    TestSizeFillCtor<StackAllocation>(&s_Stack, usize(4));
    TestSizeFillCtor<StaticAlloc4>();
    TestSizeFillCtor<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Array: initializer_list & range constructors", "[Array]")
{
    TestInitListRange<ArenaAllocation>(&s_Arena, usize(4));
    TestInitListRange<DynamicAllocation>(usize(4));
    TestInitListRange<StackAllocation>(&s_Stack, usize(4));
    TestInitListRange<StaticAlloc4>();
    TestInitListRange<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Array: copy/move ctor and assignment", "[Array]")
{
    TestCopyMove<ArenaAllocation>(&s_Arena, usize(4));
    TestCopyMove<DynamicAllocation>(usize(4));
    TestCopyMove<StackAllocation>(&s_Stack, usize(4));
    TestCopyMove<StaticAlloc4>();
    TestCopyMove<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Array: member Insert wrappers", "[Array]")
{
    TestInsert<ArenaAllocation>(&s_Arena, usize(7));
    TestInsert<DynamicAllocation>(usize(7));
    TestInsert<StackAllocation>(&s_Stack, usize(7));
    TestInsert<StaticAlloc7>();
    TestInsert<TierAllocation>(&s_Tier, usize(7));
}

TEST_CASE("Array: member RemoveOrdered/Unordered wrappers", "[Array]")
{
    TestRemove<ArenaAllocation>(&s_Arena, usize(6));
    TestRemove<DynamicAllocation>(usize(6));
    TestRemove<StackAllocation>(&s_Stack, usize(6));
    TestRemove<StaticAlloc6>();
    TestRemove<TierAllocation>(&s_Tier, usize(6));
}

TEST_CASE("Array: Resize", "[Array]")
{
    TestResize<ArenaAllocation>(&s_Arena, usize(5));
    TestResize<DynamicAllocation>(usize(5));
    TestResize<StackAllocation>(&s_Stack, usize(5));
    TestResize<StaticAlloc5>();
    TestResize<TierAllocation>(&s_Tier, usize(5));
}

TEST_CASE("Array: Clear and iteration", "[Array]")
{
    TestClearIteration<ArenaAllocation>(&s_Arena, usize(4));
    TestClearIteration<DynamicAllocation>(usize(4));
    TestClearIteration<StackAllocation>(&s_Stack, usize(4));
    TestClearIteration<StaticAlloc4>();
    TestClearIteration<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Array<std::string>: basic operations", "[Array][string]")
{
    TestStringOps<ArenaAllocation>(&s_Arena, usize(15));
    TestStringOps<DynamicAllocation>(usize(15));
    TestStringOps<StackAllocation>(&s_Stack, usize(15));
    TestStringOps<StaticAlloc15>();
    TestStringOps<TierAllocation>(&s_Tier, usize(15));
}
TEST_CASE("Bug1: RemoveOrdered single missing null terminator", "[Array][string][bug]")
{
    using DynStr = Array<char, DynamicAllocation<char>>;
    DynStr s("abcde");
    REQUIRE(std::string(s.CString()) == "abcde");

    s.RemoveOrdered(s.begin() + 2); // remove 'c'
    REQUIRE(s.GetSize() == 4);
    // CString() should read "abde", but no null was written at position 4.
    // The old 'e' still sits there, so CString() returns "abdee" or worse
    // depending on what happened to be in memory.
    REQUIRE(std::string(s.CString()) == "abde"); // FAILS
}
TEST_CASE("Bug2: RemoveOrdered range missing null terminator", "[Array][string][bug]")
{
    using DynStr = Array<char, DynamicAllocation<char>>;
    DynStr s("abcdef");

    s.RemoveOrdered(s.begin() + 1, s.begin() + 4); // remove "bcd"
    REQUIRE(s.GetSize() == 3);
    // Should be "aef", but stale characters remain after the logical end
    REQUIRE(std::string(s.CString()) == "aef"); // FAILS
}
TEST_CASE("Bug3: Append self-ref dangles on realloc", "[Array][bug]")
{
    Array<std::string, DynamicAllocation<std::string>> arr{};
    arr.Append("first");
    arr.Append("second");
    REQUIRE(arr.IsFull());

    // arr[0] references into the current buffer.
    // Append grows capacity, freeing that buffer before reading the reference.
    arr.Append(arr[0]);
    REQUIRE(arr.GetSize() == 3);
    REQUIRE(arr[2] == "first"); // FAILS or UB: reads freed memory
}

TEST_CASE("Bug3b: Append self-ref dangles on realloc (Tier)", "[Array][tier][bug]")
{
    ArenaAllocator arena{1_mib};
    TierAllocator tier{{.Allocator = &arena, .MaxAllocation = 16_kib}};
    {
        Array<std::string, TierAllocation<std::string>> arr(&tier);
        arr.Append("hello");
        arr.Append("world");
        REQUIRE(arr.IsFull());

        arr.Append(arr[0]);
        REQUIRE(arr.GetSize() == 3);
        REQUIRE(arr[2] == "hello"); // FAILS or UB
    }
}
TEST_CASE("Bug4: Range Insert self-iterators dangle on realloc", "[Array][bug]")
{
    Array<u32, DynamicAllocation<u32>> arr{};
    arr.Append(1);
    arr.Append(2);
    arr.Append(3);
    arr.Append(4);
    arr.Append(5);
    REQUIRE(arr.IsFull());

    // Source iterators point into arr. Insert triggers realloc.
    // ppos is adjusted, pbegin/pend are not.
    arr.Insert(arr.begin() + 2, arr.begin(), arr.begin() + 2);
    REQUIRE(arr.GetSize() == 7);
    REQUIRE(arr[2] == 1); // FAILS or UB
    REQUIRE(arr[3] == 2);
}
TEST_CASE("Bug5: Insert single element from shifted range", "[Array][bug]")
{
    Array<std::string, DynamicAllocation<std::string>> arr{};
    arr.Append("A");
    arr.Append("B");
    arr.Append("C");
    arr.Append("D");

    // Insert arr[2] ("C") at position 0.
    // Shift moves "A","B","C","D" → _,"A","B","C","D"
    // arr[2] now holds "B" (shifted from index 1), not the original "C"
    arr.Insert(arr.begin(), arr[2]);
    REQUIRE(arr.GetSize() == 5);
    REQUIRE(arr[0] == "C"); // FAILS: gets "B" or empty string
}
