#include "tkit/container/hive.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Alias;

static ArenaAllocator s_Arena{1_mib};
static StackAllocator s_Stack{1_mib};
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
} // namespace

template <typename T> using StaticAlloc4 = StaticAllocation<T, 4>;
template <typename T> using StaticAlloc8 = StaticAllocation<T, 8>;

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestHiveBasic([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);
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

template <template <typename> typename A, typename... Args>
void TestHiveInsertLookup([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);

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

template <template <typename> typename A, typename... Args>
void TestHiveRemoveCompact([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);

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

template <template <typename> typename A, typename... Args> void TestHiveRemoveLast([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);

    const usize id0 = hive.Insert(42u);
    const usize id1 = hive.Insert(43u);

    hive.Remove(id1);

    REQUIRE(hive.GetSize() == 1);
    REQUIRE(hive.Contains(id0));
    REQUIRE_FALSE(hive.Contains(id1));
    REQUIRE(hive[id0] == 42u);
}

template <template <typename> typename A, typename... Args>
void TestHiveRemoveFirst([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);

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

template <template <typename> typename A, typename... Args> void TestHiveIdReuse([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);

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

template <template <typename> typename A, typename... Args>
void TestHiveMultipleRemoves([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);

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

template <template <typename> typename A, typename... Args>
void TestHiveDenseIteration([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);

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

template <template <typename> typename A, typename... Args> void TestHiveReserve([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);
    hive.Reserve(8);
    REQUIRE(hive.GetSize() == 0);
    REQUIRE(hive.IsEmpty());
    REQUIRE(hive.GetCapacity() >= 8);
}

template <template <typename> typename A, typename... Args> void TestHiveCopyMove([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<u32, A<u32>>>(args...);
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

template <template <typename> typename A, typename... Args> void TestHiveStringOps([[maybe_unused]] const Args... args)
{
    auto hive = Reserved<Hive<std::string, A<std::string>>>(args...);

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
    const PushedAllocators pushed{};
    TestHiveBasic<ArenaAllocation>(usize(4));
    TestHiveBasic<DynamicAllocation>(usize(4));
    TestHiveBasic<StackAllocation>(usize(4));
    TestHiveBasic<StaticAlloc4>();
    TestHiveBasic<TierAllocation>(usize(4));
}

TEST_CASE("Hive: Insert and lookup", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveInsertLookup<ArenaAllocation>(usize(8));
    TestHiveInsertLookup<DynamicAllocation>(usize(8));
    TestHiveInsertLookup<StackAllocation>(usize(8));
    TestHiveInsertLookup<StaticAlloc8>();
    TestHiveInsertLookup<TierAllocation>(usize(8));
}

TEST_CASE("Hive: Remove invalidates id and compacts dense array", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveRemoveCompact<ArenaAllocation>(usize(8));
    TestHiveRemoveCompact<DynamicAllocation>(usize(8));
    TestHiveRemoveCompact<StackAllocation>(usize(8));
    TestHiveRemoveCompact<StaticAlloc8>();
    TestHiveRemoveCompact<TierAllocation>(usize(8));
}

TEST_CASE("Hive: Remove last element", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveRemoveLast<ArenaAllocation>(usize(4));
    TestHiveRemoveLast<DynamicAllocation>(usize(4));
    TestHiveRemoveLast<StackAllocation>(usize(4));
    TestHiveRemoveLast<StaticAlloc4>();
    TestHiveRemoveLast<TierAllocation>(usize(4));
}

TEST_CASE("Hive: Remove first element", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveRemoveFirst<ArenaAllocation>(usize(4));
    TestHiveRemoveFirst<DynamicAllocation>(usize(4));
    TestHiveRemoveFirst<StackAllocation>(usize(4));
    TestHiveRemoveFirst<StaticAlloc4>();
    TestHiveRemoveFirst<TierAllocation>(usize(4));
}

TEST_CASE("Hive: id reuse after Remove", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveIdReuse<ArenaAllocation>(usize(8));
    TestHiveIdReuse<DynamicAllocation>(usize(8));
    TestHiveIdReuse<StackAllocation>(usize(8));
    TestHiveIdReuse<StaticAlloc8>();
    TestHiveIdReuse<TierAllocation>(usize(8));
}

TEST_CASE("Hive: multiple Removes then Inserts reuse ids in order", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveMultipleRemoves<ArenaAllocation>(usize(8));
    TestHiveMultipleRemoves<DynamicAllocation>(usize(8));
    TestHiveMultipleRemoves<StackAllocation>(usize(8));
    TestHiveMultipleRemoves<StaticAlloc8>();
    TestHiveMultipleRemoves<TierAllocation>(usize(8));
}

TEST_CASE("Hive: dense iteration is contiguous and order-independent", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveDenseIteration<ArenaAllocation>(usize(8));
    TestHiveDenseIteration<DynamicAllocation>(usize(8));
    TestHiveDenseIteration<StackAllocation>(usize(8));
    TestHiveDenseIteration<StaticAlloc8>();
    TestHiveDenseIteration<TierAllocation>(usize(8));
}

TEST_CASE("Hive: Reserve does not change size", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveReserve<ArenaAllocation>(usize(0));
    TestHiveReserve<DynamicAllocation>(usize(0));
    TestHiveReserve<StackAllocation>(usize(0));
    // TestHiveReserve<StaticAlloc8>();
    TestHiveReserve<TierAllocation>(usize(0));
}

TEST_CASE("Hive: copy and move", "[Hive]")
{
    const PushedAllocators pushed{};
    TestHiveCopyMove<ArenaAllocation>(usize(8));
    TestHiveCopyMove<DynamicAllocation>(usize(8));
    TestHiveCopyMove<StackAllocation>(usize(8));
    TestHiveCopyMove<StaticAlloc8>();
    TestHiveCopyMove<TierAllocation>(usize(8));
}

TEST_CASE("Hive<std::string>: Insert, Remove and id reuse", "[Hive][string]")
{
    const PushedAllocators pushed{};
    TestHiveStringOps<ArenaAllocation>(usize(8));
    TestHiveStringOps<DynamicAllocation>(usize(8));
    TestHiveStringOps<StackAllocation>(usize(8));
    TestHiveStringOps<StaticAlloc8>();
    TestHiveStringOps<TierAllocation>(usize(8));
}
