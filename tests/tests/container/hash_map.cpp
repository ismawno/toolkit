#include "tkit/container/hash_map.hpp"
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

static constexpr usize initialSize = 4;

template <typename T> using StaticAlloc16 = StaticAllocation<T, 16>;
template <typename T> using StaticAlloc32 = StaticAllocation<T, 32>;

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestMapInsertFind([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    const u32 v0 = map.Insert(1u, 100u);
    const u32 v1 = map.Insert(2u, 200u);
    const u32 v2 = map.Insert(3u, 300u);
    REQUIRE(v0 == 100u);
    REQUIRE(v1 == 200u);
    REQUIRE(v2 == 300u);
    REQUIRE(map.GetSize() == 3);

    // operator[] on existing key mutates in place
    REQUIRE(map.Find(3u)->Value == 300u);
    map[3u] = 250u;
    REQUIRE(map.Find(3u)->Value == 250u);
    REQUIRE(map.GetSize() == 3); // no new entry

    // operator[] on new key inserts via TryInsert
    map[50u] = 500u;
    REQUIRE(map.Find(50u)->Value == 500u);
    REQUIRE(map.GetSize() == 4);

    // Find returns correct values for all keys
    REQUIRE(map.Find(1u)->Value == 100u);
    REQUIRE(map.Find(2u)->Value == 200u);
    REQUIRE(map.Find(3u)->Value == 250u);
    REQUIRE(map.Find(50u)->Value == 500u);

    REQUIRE_FALSE(map.Contains(99u));
}

template <template <typename> typename A, typename... Args> void TestMapAt([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(1u, 10u);
    map.Insert(2u, 20u);

    // const At
    const auto &cmap = map;
    REQUIRE(cmap.At(1u) == 10u);
    REQUIRE(cmap.At(2u) == 20u);

    // mutable At
    map.At(1u) = 99u;
    REQUIRE(map.At(1u) == 99u);
    REQUIRE(cmap[2u] == 20u);
}

template <template <typename> typename A, typename... Args> void TestMapTryInsert([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    // first TryInsert creates the entry
    u32 &v0 = map.TryInsert(1u, 100u);
    REQUIRE(v0 == 100u);
    REQUIRE(map.GetSize() == 1);

    // second TryInsert on same bucket returns existing, does not overwrite
    u32 &v1 = map.TryInsert(1u, 999u);
    REQUIRE(v1 == 100u);
    REQUIRE(map.GetSize() == 1);

    // operator[] uses TryInsert — existing key returns current value
    map[1u] = 42u;
    REQUIRE(map.At(1u) == 42u);

    // operator[] on fresh key default-constructs
    u32 &fresh = map[77u];
    fresh = 77u;
    REQUIRE(map.At(77u) == 77u);
}

template <template <typename> typename A, typename... Args> void TestMapContains([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(10u, 1u);
    map.Insert(20u, 2u);

    REQUIRE(map.Contains(10u));
    REQUIRE(map.Contains(20u));
    REQUIRE_FALSE(map.Contains(30u));
    REQUIRE_FALSE(map.Contains(0u));
}

template <template <typename> typename A, typename... Args> void TestMapRemoveByKey([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(1u, 10u);
    map.Insert(2u, 20u);
    map.Insert(3u, 30u);
    REQUIRE(map.GetSize() == 3);

    REQUIRE(map.TryRemove(2u));
    REQUIRE(map.GetSize() == 2);
    REQUIRE_FALSE(map.Contains(2u));
    REQUIRE(map.Contains(1u));
    REQUIRE(map.Contains(3u));

    REQUIRE_FALSE(map.TryRemove(99u));
    REQUIRE(map.GetSize() == 2);

    REQUIRE(map.Find(1u)->Value == 10u);
    REQUIRE(map.Find(3u)->Value == 30u);
}

template <template <typename> typename A, typename... Args> void TestMapRemoveByIterator([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(5u, 50u);
    map.Insert(6u, 60u);

    REQUIRE(map.Contains(5u));
    auto it = map.Find(5u);
    map.Remove(it);

    REQUIRE(map.GetSize() == 1);
    REQUIRE_FALSE(map.Contains(5u));
    REQUIRE(map.Contains(6u));
    REQUIRE(map.Find(6u)->Value == 60u);
}

template <template <typename> typename A, typename... Args> void TestMapClear([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(1u, 1u);
    map.Insert(2u, 2u);
    map.Insert(3u, 3u);
    REQUIRE(map.GetSize() == 3);

    map.Clear();
    REQUIRE(map.GetSize() == 0);
    REQUIRE_FALSE(map.Contains(1u));
    REQUIRE_FALSE(map.Contains(2u));
    REQUIRE_FALSE(map.Contains(3u));

    // re-insert after clear
    map.Insert(4u, 40u);
    REQUIRE(map.GetSize() == 1);
    REQUIRE(map.Find(4u)->Value == 40u);
}

template <template <typename> typename A, typename... Args> void TestMapIteration([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(1u, 10u);
    map.Insert(2u, 20u);
    map.Insert(3u, 30u);

    u32 keySum = 0;
    u32 valSum = 0;
    u32 count = 0;
    for (const auto &entry : map)
    {
        keySum += entry.Key;
        valSum += entry.Value;
        ++count;
    }
    REQUIRE(count == 3);
    REQUIRE(keySum == 6u);
    REQUIRE(valSum == 60u);

    // mutation through non-const iteration
    for (auto &entry : map)
        entry.Value *= 2;

    REQUIRE(map.Find(1u)->Value == 20u);
    REQUIRE(map.Find(2u)->Value == 40u);
    REQUIRE(map.Find(3u)->Value == 60u);
}

template <template <typename> typename A, typename... Args> void TestMapMutation([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(1u, 100u);

    // mutate through Find->
    map.Find(1u)->Value = 999u;
    REQUIRE(map.At(1u) == 999u);

    // mutate through At
    map.At(1u) = 50u;
    REQUIRE(map.Find(1u)->Value == 50u);

    // mutate through operator[]
    map[1u] = 42u;
    REQUIRE(map.At(1u) == 42u);

    // Insert return ref
    u32 &ref = map.Insert(2u, 0u);
    ref = 77u;
    REQUIRE(map.At(2u) == 77u);
}

template <template <typename> typename A, typename... Args> void TestMapInsertAfterRemove([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    map.Insert(1u, 10u);
    map.Insert(2u, 20u);
    map.Insert(3u, 30u);
    map.Remove(2u);
    REQUIRE(map.GetSize() == 2);

    // insert into tombstone slot
    map.Insert(4u, 40u);
    REQUIRE(map.Contains(4u));
    REQUIRE(map.Find(4u)->Value == 40u);
    REQUIRE(map.Contains(1u));
    REQUIRE(map.Contains(3u));
    REQUIRE_FALSE(map.Contains(2u));
    REQUIRE(map.GetSize() == 3);

    // remove all, re-insert
    map.Remove(1u);
    map.Remove(3u);
    map.Remove(4u);
    REQUIRE(map.GetSize() == 0);

    map.Insert(100u, 1000u);
    REQUIRE(map.GetSize() == 1);
    REQUIRE(map.At(100u) == 1000u);
}

template <template <typename> typename A, typename... Args> void TestMapStringKV([[maybe_unused]] const Args... args)
{
    using Node = MapNode<std::string, std::string>;
    HashMap<std::string, std::string, A<Node>> map(initialSize);

    map.Insert(std::string("name"), std::string("Alice"));
    map.Insert(std::string("city"), std::string("Madrid"));
    map.Insert(std::string("lang"), std::string("C++"));

    REQUIRE(map.GetSize() == 3);
    REQUIRE(map.At(std::string("name")) == "Alice");
    REQUIRE(map.At(std::string("city")) == "Madrid");
    REQUIRE(map.At(std::string("lang")) == "C++");

    // mutate via operator[]
    map[std::string("city")] = "London";
    REQUIRE(map.At(std::string("city")) == "London");
    REQUIRE(map.GetSize() == 3);

    map.Remove(std::string("city"));
    REQUIRE_FALSE(map.Contains(std::string("city")));
    REQUIRE(map.At(std::string("name")) == "Alice");
    REQUIRE(map.At(std::string("lang")) == "C++");
    REQUIRE(map.GetSize() == 2);
}

template <template <typename> typename A, typename... Args> void TestMapMixedKV([[maybe_unused]] const Args... args)
{
    using Node = MapNode<std::string, u32>;
    HashMap<std::string, u32, A<Node>> map(initialSize);

    map.Insert(std::string("alpha"), 1u);
    map.Insert(std::string("beta"), 2u);
    map.Insert(std::string("gamma"), 3u);

    REQUIRE(map.GetSize() == 3);

    u32 sum = 0;
    for (const auto &entry : map)
        sum += entry.Value;
    REQUIRE(sum == 6u);

    // operator[] default-constructs then assign
    map[std::string("delta")] = 4u;
    REQUIRE(map.GetSize() == 4);
    REQUIRE(map.At(std::string("delta")) == 4u);

    map.Remove(std::string("beta"));
    REQUIRE_FALSE(map.Contains(std::string("beta")));

    map.Clear();
    REQUIRE(map.GetSize() == 0);
}

template <template <typename> typename A, typename... Args> void TestMapLoadFactor([[maybe_unused]] const Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map(initialSize);

    REQUIRE(map.GetSize() == 0);
    map.Insert(1u, 1u);
    const f32 lf = map.GetLoadFactor();
    REQUIRE(lf > 0.0f);
    REQUIRE(lf <= 1.0f);

    map.Insert(2u, 2u);
    REQUIRE(map.GetLoadFactor() > lf);
}

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

#define MAP_TEST_ALL(Fn, Cap)                                                                                          \
    Fn<ArenaAllocation>(usize(Cap));                                                                         \
    Fn<DynamicAllocation>(usize(Cap));                                                                                 \
    Fn<StackAllocation>(usize(Cap));                                                                         \
    Fn<StaticAlloc##Cap>();                                                                                            \
    Fn<TierAllocation>(usize(Cap))

TEST_CASE("HashMap: Insert and Find", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapInsertFind, 16);
}
TEST_CASE("HashMap: At accessor", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapAt, 16);
}
TEST_CASE("HashMap: TryInsert semantics", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapTryInsert, 16);
}
TEST_CASE("HashMap: Contains", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapContains, 16);
}
TEST_CASE("HashMap: Remove by key", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapRemoveByKey, 16);
}
TEST_CASE("HashMap: Remove by iterator", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapRemoveByIterator, 16);
}
TEST_CASE("HashMap: Clear and re-insert", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapClear, 16);
}
TEST_CASE("HashMap: Iteration and mutation", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapIteration, 16);
}
TEST_CASE("HashMap: Mutation through Find, At, operator[], Insert ref", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapMutation, 16);
}
TEST_CASE("HashMap: Insert after Remove (tombstone reuse)", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapInsertAfterRemove, 16);
}
TEST_CASE("HashMap<string, string>: basic operations", "[HashMap][string]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapStringKV, 16);
}
TEST_CASE("HashMap<string, u32>: mixed key-value types", "[HashMap][string]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapMixedKV, 16);
}
TEST_CASE("HashMap: load factor", "[HashMap]")
{
    const PushedAllocators pushed{};
    MAP_TEST_ALL(TestMapLoadFactor, 16);
}

#undef MAP_TEST_ALL
