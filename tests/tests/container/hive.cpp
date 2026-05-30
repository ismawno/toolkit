#include "tkit/container/hive.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

static ArenaAllocator s_Arena{1_mib};
static StackAllocator s_Stack{1_mib};
static TierAllocator s_Tier{{.Allocator = &s_Arena, .MaxAllocation = 16_kib}};

template <typename T> using StaticAlloc4 = StaticAllocation<T, 4>;
template <typename T> using StaticAlloc8 = StaticAllocation<T, 8>;

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestHiveBasic(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};
    REQUIRE(hive.GetSize() == 0);
    REQUIRE(hive.IsEmpty());
    REQUIRE_FALSE(hive.IsFull());

    const usize id0 = hive.Insert(10u);
    const usize id1 = hive.Insert(20u);
    REQUIRE(hive.GetSize() == 2);
    REQUIRE_FALSE(hive.IsEmpty());
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id1));
}

template <template <typename> typename A, typename... Args> void TestHiveInsertLookup(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};

    const usize id0 = hive.Insert(100u);
    const usize id1 = hive.Insert(200u);
    const usize id2 = hive.Insert(300u);

    REQUIRE(hive[id0] == 100u);
    REQUIRE(hive[id1] == 200u);
    REQUIRE(hive[id2] == 300u);

    hive[id1] = 999u;
    REQUIRE(hive[id1] == 999u);

    REQUIRE(hive.GetSize() == 3);
}

template <template <typename> typename A, typename... Args> void TestHiveRemoveCompact(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};

    const usize id0 = hive.Insert(1u);
    const usize id1 = hive.Insert(2u);
    const usize id2 = hive.Insert(3u);

    hive.Remove(id1);

    REQUIRE(hive.GetSize() == 2);
    REQUIRE_FALSE(hive.Contains(id1));
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id2));

    REQUIRE(hive[id0] == 1u);
    REQUIRE(hive[id2] == 3u);

    u32 sum = 0;
    for (const u32 x : hive)
        sum += x;
    REQUIRE(sum == 4u);
}

template <template <typename> typename A, typename... Args> void TestHiveRemoveLast(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};

    const usize id0 = hive.Insert(42u);
    const usize id1 = hive.Insert(43u);

    hive.Remove(id1);

    REQUIRE(hive.GetSize() == 1);
    REQUIRE(hive.Contains(id0));
    REQUIRE_FALSE(hive.Contains(id1));
    REQUIRE(hive[id0] == 42u);
}

template <template <typename> typename A, typename... Args> void TestHiveRemoveFirst(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};

    const usize id0 = hive.Insert(10u);
    const usize id1 = hive.Insert(20u);
    const usize id2 = hive.Insert(30u);

    hive.Remove(id0);

    REQUIRE(hive.GetSize() == 2);
    REQUIRE_FALSE(hive.Contains(id0));
    REQUIRE(hive.Contains(id1));
    REQUIRE(hive.Contains(id2));
    REQUIRE(hive[id1] == 20u);
    REQUIRE(hive[id2] == 30u);
}

template <template <typename> typename A, typename... Args> void TestHiveIdReuse(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};

    const usize id0 = hive.Insert(1u);
    const usize id1 = hive.Insert(2u);
    const usize id2 = hive.Insert(3u);

    hive.Remove(id1);
    REQUIRE_FALSE(hive.Contains(id1));

    const usize id3 = hive.Insert(99u);
    REQUIRE(id3 == id1);
    REQUIRE(hive.Contains(id3));
    REQUIRE(hive[id3] == 99u);

    REQUIRE(hive.GetSize() == 3);
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id2));
}

template <template <typename> typename A, typename... Args> void TestHiveMultipleRemoves(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};

    const usize id0 = hive.Insert(10u);
    const usize id1 = hive.Insert(20u);
    const usize id2 = hive.Insert(30u);
    const usize id3 = hive.Insert(40u);

    hive.Remove(id1);
    hive.Remove(id2);
    REQUIRE(hive.GetSize() == 2);

    const usize nid0 = hive.Insert(55u);
    const usize nid1 = hive.Insert(66u);

    REQUIRE((nid0 == id1 || nid0 == id2));
    REQUIRE((nid1 == id1 || nid1 == id2));
    REQUIRE(nid0 != nid1);

    REQUIRE(hive.GetSize() == 4);
    REQUIRE(hive.Contains(id0));
    REQUIRE(hive.Contains(id3));
    REQUIRE(hive.Contains(nid0));
    REQUIRE(hive.Contains(nid1));
}

template <template <typename> typename A, typename... Args> void TestHiveDenseIteration(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};

    hive.Insert(1u);
    hive.Insert(2u);
    const usize mid = hive.Insert(3u);
    hive.Insert(4u);
    hive.Insert(5u);

    hive.Remove(mid);

    u32 sum = 0;
    u32 count = 0;
    for (const u32 x : hive)
    {
        sum += x;
        ++count;
    }
    REQUIRE(count == 4);
    REQUIRE(sum == 12u);
}

template <template <typename> typename A, typename... Args> void TestHiveReserve(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};
    hive.Reserve(8);
    REQUIRE(hive.GetSize() == 0);
    REQUIRE(hive.IsEmpty());
    REQUIRE(hive.GetCapacity() >= 8);
}

template <template <typename> typename A, typename... Args> void TestHiveCopyMove(Args... args)
{
    A<u32> st1{args...};
    A<u32> st2{args...};
    A<u32> st3{args...};
    Hive<u32, A<u32>> hive{std::move(st1), std::move(st2), std::move(st3)};
    const usize id0 = hive.Insert(7u);
    const usize id1 = hive.Insert(8u);

    Hive<u32, A<u32>> copy = hive;
    REQUIRE(copy.GetSize() == 2);
    REQUIRE(copy[id0] == 7u);
    REQUIRE(copy[id1] == 8u);

    copy[id0] = 77u;
    REQUIRE(hive[id0] == 7u);

    Hive<u32, A<u32>> moved = std::move(copy);
    REQUIRE(moved.GetSize() == 2);
    REQUIRE(moved[id0] == 77u);
    REQUIRE(moved[id1] == 8u);
}

template <template <typename> typename A, typename... Args> void TestHiveStringOps(Args... args)
{
    A<std::string> st1{args...};
    A<std::string> st2{args...};
    A<std::string> st3{args...};
    Hive<std::string, A<std::string>> hive{std::move(st1), std::move(st2), std::move(st3)};

    const usize id0 = hive.Insert("alpha");
    const usize id1 = hive.Insert("beta");
    const usize id2 = hive.Insert("gamma");

    REQUIRE(hive[id0] == "alpha");
    REQUIRE(hive[id1] == "beta");
    REQUIRE(hive[id2] == "gamma");

    hive.Remove(id1);
    REQUIRE_FALSE(hive.Contains(id1));
    REQUIRE(hive.GetSize() == 2);

    REQUIRE(hive[id0] == "alpha");
    REQUIRE(hive[id2] == "gamma");

    const usize id3 = hive.Insert("delta");
    REQUIRE(id3 == id1);
    REQUIRE(hive[id3] == "delta");
    REQUIRE(hive.GetSize() == 3);

    std::string concat;
    for (const std::string &s : hive)
        concat += s;
    REQUIRE(concat.size() == 5 + 5 + 5);
    REQUIRE(concat.find("alpha") != std::string::npos);
    REQUIRE(concat.find("gamma") != std::string::npos);
    REQUIRE(concat.find("delta") != std::string::npos);
}

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

TEST_CASE("Hive: basic capacity/size queries", "[Hive]")
{
    TestHiveBasic<ArenaAllocation>(&s_Arena, usize(4));
    TestHiveBasic<DynamicAllocation>(usize(4));
    TestHiveBasic<StackAllocation>(&s_Stack, usize(4));
    TestHiveBasic<StaticAlloc4>();
    TestHiveBasic<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Hive: Insert and lookup", "[Hive]")
{
    TestHiveInsertLookup<ArenaAllocation>(&s_Arena, usize(8));
    TestHiveInsertLookup<DynamicAllocation>(usize(8));
    TestHiveInsertLookup<StackAllocation>(&s_Stack, usize(8));
    TestHiveInsertLookup<StaticAlloc8>();
    TestHiveInsertLookup<TierAllocation>(&s_Tier, usize(8));
}

TEST_CASE("Hive: Remove invalidates id and compacts dense array", "[Hive]")
{
    TestHiveRemoveCompact<ArenaAllocation>(&s_Arena, usize(8));
    TestHiveRemoveCompact<DynamicAllocation>(usize(8));
    TestHiveRemoveCompact<StackAllocation>(&s_Stack, usize(8));
    TestHiveRemoveCompact<StaticAlloc8>();
    TestHiveRemoveCompact<TierAllocation>(&s_Tier, usize(8));
}

TEST_CASE("Hive: Remove last element", "[Hive]")
{
    TestHiveRemoveLast<ArenaAllocation>(&s_Arena, usize(4));
    TestHiveRemoveLast<DynamicAllocation>(usize(4));
    TestHiveRemoveLast<StackAllocation>(&s_Stack, usize(4));
    TestHiveRemoveLast<StaticAlloc4>();
    TestHiveRemoveLast<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Hive: Remove first element", "[Hive]")
{
    TestHiveRemoveFirst<ArenaAllocation>(&s_Arena, usize(4));
    TestHiveRemoveFirst<DynamicAllocation>(usize(4));
    TestHiveRemoveFirst<StackAllocation>(&s_Stack, usize(4));
    TestHiveRemoveFirst<StaticAlloc4>();
    TestHiveRemoveFirst<TierAllocation>(&s_Tier, usize(4));
}

TEST_CASE("Hive: id reuse after Remove", "[Hive]")
{
    TestHiveIdReuse<ArenaAllocation>(&s_Arena, usize(8));
    TestHiveIdReuse<DynamicAllocation>(usize(8));
    TestHiveIdReuse<StackAllocation>(&s_Stack, usize(8));
    TestHiveIdReuse<StaticAlloc8>();
    TestHiveIdReuse<TierAllocation>(&s_Tier, usize(8));
}

TEST_CASE("Hive: multiple Removes then Inserts reuse ids in order", "[Hive]")
{
    TestHiveMultipleRemoves<ArenaAllocation>(&s_Arena, usize(8));
    TestHiveMultipleRemoves<DynamicAllocation>(usize(8));
    TestHiveMultipleRemoves<StackAllocation>(&s_Stack, usize(8));
    TestHiveMultipleRemoves<StaticAlloc8>();
    TestHiveMultipleRemoves<TierAllocation>(&s_Tier, usize(8));
}

TEST_CASE("Hive: dense iteration is contiguous and order-independent", "[Hive]")
{
    TestHiveDenseIteration<ArenaAllocation>(&s_Arena, usize(8));
    TestHiveDenseIteration<DynamicAllocation>(usize(8));
    TestHiveDenseIteration<StackAllocation>(&s_Stack, usize(8));
    TestHiveDenseIteration<StaticAlloc8>();
    TestHiveDenseIteration<TierAllocation>(&s_Tier, usize(8));
}

TEST_CASE("Hive: Reserve does not change size", "[Hive]")
{
    TestHiveReserve<ArenaAllocation>(&s_Arena, usize(0));
    TestHiveReserve<DynamicAllocation>(usize(0));
    TestHiveReserve<StackAllocation>(&s_Stack, usize(0));
    // TestHiveReserve<StaticAlloc8>();
    TestHiveReserve<TierAllocation>(&s_Tier, usize(0));
}

TEST_CASE("Hive: copy and move", "[Hive]")
{
    TestHiveCopyMove<ArenaAllocation>(&s_Arena, usize(8));
    TestHiveCopyMove<DynamicAllocation>(usize(8));
    TestHiveCopyMove<StackAllocation>(&s_Stack, usize(8));
    TestHiveCopyMove<StaticAlloc8>();
    TestHiveCopyMove<TierAllocation>(&s_Tier, usize(8));
}

TEST_CASE("Hive<std::string>: Insert, Remove and id reuse", "[Hive][string]")
{
    TestHiveStringOps<ArenaAllocation>(&s_Arena, usize(8));
    TestHiveStringOps<DynamicAllocation>(usize(8));
    TestHiveStringOps<StackAllocation>(&s_Stack, usize(8));
    TestHiveStringOps<StaticAlloc8>();
    TestHiveStringOps<TierAllocation>(&s_Tier, usize(8));
}
