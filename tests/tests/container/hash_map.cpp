#include "tkit/container/hash_map.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace TKit;
using namespace TKit::Container;
using namespace TKit::Alias;

static ArenaAllocator s_Arena{1_mib};
static StackAllocator s_Stack{1_mib};
static TierAllocator s_Tier{{.Allocator = &s_Arena, .MaxAllocation = 16_kib}};

static constexpr usize initialSize = 4;

template <typename T> using StaticAlloc16 = StaticAllocation<T, 16>;
template <typename T> using StaticAlloc32 = StaticAllocation<T, 32>;

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestMapInsertFind(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

    u32 *v0 = map.Insert(1u, 100u);
    u32 *v1 = map.Insert(2u, 200u);
    u32 *v2 = map.Insert(3u, 300u);

    REQUIRE(v0 != nullptr);
    REQUIRE(v1 != nullptr);
    REQUIRE(v2 != nullptr);
    REQUIRE(*v0 == 100u);
    REQUIRE(*v1 == 200u);
    REQUIRE(*v2 == 300u);

    REQUIRE(map.GetSize() == 3);

    auto it0 = map.Find(1u);
    auto it1 = map.Find(2u);
    auto it2 = map.Find(3u);
    REQUIRE((*it0).Value == 100u);
    REQUIRE((*it1).Value == 200u);
    REQUIRE((*it2).Value == 300u);

    // non-existent key
    REQUIRE_FALSE(map.Contains(99u));
}

template <template <typename> typename A, typename... Args> void TestMapContains(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(10u, 1u);
    map.Insert(20u, 2u);

    REQUIRE(map.Contains(10u));
    REQUIRE(map.Contains(20u));
    REQUIRE_FALSE(map.Contains(30u));
    REQUIRE_FALSE(map.Contains(0u));
}

template <template <typename> typename A, typename... Args> void TestMapRemoveByKey(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(1u, 10u);
    map.Insert(2u, 20u);
    map.Insert(3u, 30u);
    REQUIRE(map.GetSize() == 3);

    REQUIRE(map.Remove(2u));
    REQUIRE_FALSE(map.Contains(2u));
    REQUIRE(map.Contains(1u));
    REQUIRE(map.Contains(3u));

    // removing non-existent key
    REQUIRE_FALSE(map.Remove(99u));

    // remaining values intact
    REQUIRE((*map.Find(1u)).Value == 10u);
    REQUIRE((*map.Find(3u)).Value == 30u);
}

template <template <typename> typename A, typename... Args> void TestMapRemoveByIterator(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(5u, 50u);
    map.Insert(6u, 60u);

    REQUIRE(map.Contains(5u));
    auto it = map.Find(5u);
    map.Remove(it);

    REQUIRE_FALSE(map.Contains(5u));
    REQUIRE(map.Contains(6u));
    REQUIRE((*map.Find(6u)).Value == 60u);
}

template <template <typename> typename A, typename... Args> void TestMapClear(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(1u, 1u);
    map.Insert(2u, 2u);
    map.Insert(3u, 3u);
    REQUIRE(map.GetSize() == 3);

    map.Clear();
    REQUIRE(map.GetSize() == 0);
    REQUIRE_FALSE(map.Contains(1u));
    REQUIRE_FALSE(map.Contains(2u));
    REQUIRE_FALSE(map.Contains(3u));
}

template <template <typename> typename A, typename... Args> void TestMapIteration(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

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
}

template <template <typename> typename A, typename... Args> void TestMapMutation(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(1u, 100u);
    REQUIRE((*map.Find(1u)).Value == 100u);

    // mutate through iterator
    (*map.Find(1u)).Value = 999u;
    REQUIRE((*map.Find(1u)).Value == 999u);

    // mutate through Insert return value
    u32 *v = map.Insert(2u, 0u);
    *v = 42u;
    REQUIRE((*map.Find(2u)).Value == 42u);
}

template <template <typename> typename A, typename... Args> void TestMapInsertAfterRemove(Args... args)
{
    using Node = MapNode<u32, u32>;
    HashMap<u32, u32, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(1u, 10u);
    map.Insert(2u, 20u);
    map.Insert(3u, 30u);
    map.Remove(2u);

    // insert new key — should find a free or tombstone slot
    map.Insert(4u, 40u);
    REQUIRE(map.Contains(4u));
    REQUIRE((*map.Find(4u)).Value == 40u);
    REQUIRE(map.Contains(1u));
    REQUIRE(map.Contains(3u));
    REQUIRE_FALSE(map.Contains(2u));
}

template <template <typename> typename A, typename... Args> void TestMapStringKV(Args... args)
{
    using Node = MapNode<std::string, std::string>;
    HashMap<std::string, std::string, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(std::string("name"), std::string("Alice"));
    map.Insert(std::string("city"), std::string("Madrid"));
    map.Insert(std::string("lang"), std::string("C++"));

    REQUIRE(map.GetSize() == 3);
    REQUIRE(map.Contains(std::string("name")));
    REQUIRE((*map.Find(std::string("name"))).Value == "Alice");
    REQUIRE((*map.Find(std::string("city"))).Value == "Madrid");
    REQUIRE((*map.Find(std::string("lang"))).Value == "C++");

    map.Remove(std::string("city"));
    REQUIRE_FALSE(map.Contains(std::string("city")));
    REQUIRE((*map.Find(std::string("name"))).Value == "Alice");
    REQUIRE((*map.Find(std::string("lang"))).Value == "C++");
}

template <template <typename> typename A, typename... Args> void TestMapMixedKV(Args... args)
{
    using Node = MapNode<std::string, u32>;
    HashMap<std::string, u32, A<Node>> map{initialSize, A<Node>{args...}};

    map.Insert(std::string("alpha"), 1u);
    map.Insert(std::string("beta"), 2u);
    map.Insert(std::string("gamma"), 3u);

    REQUIRE(map.GetSize() == 3);

    u32 sum = 0;
    for (const auto &entry : map)
        sum += entry.Value;
    REQUIRE(sum == 6u);

    map.Remove(std::string("beta"));
    REQUIRE_FALSE(map.Contains(std::string("beta")));

    map.Clear();
    REQUIRE(map.GetSize() == 0);
}

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

#define MAP_TEST_ALL(Fn, Cap)                                                                                          \
    Fn<ArenaAllocation>(&s_Arena, usize(Cap));                                                                         \
    Fn<DynamicAllocation>(usize(Cap));                                                                                 \
    Fn<StackAllocation>(&s_Stack, usize(Cap));                                                                         \
    Fn<StaticAlloc##Cap>();                                                                                            \
    Fn<TierAllocation>(&s_Tier, usize(Cap))

TEST_CASE("HashMap: Insert and Find", "[HashMap]")
{
    MAP_TEST_ALL(TestMapInsertFind, 16);
}
TEST_CASE("HashMap: Contains", "[HashMap]")
{
    MAP_TEST_ALL(TestMapContains, 16);
}
TEST_CASE("HashMap: Remove by key", "[HashMap]")
{
    MAP_TEST_ALL(TestMapRemoveByKey, 16);
}
TEST_CASE("HashMap: Remove by iterator", "[HashMap]")
{
    MAP_TEST_ALL(TestMapRemoveByIterator, 16);
}
TEST_CASE("HashMap: Clear", "[HashMap]")
{
    MAP_TEST_ALL(TestMapClear, 16);
}
TEST_CASE("HashMap: Iteration", "[HashMap]")
{
    MAP_TEST_ALL(TestMapIteration, 16);
}
TEST_CASE("HashMap: Mutation through Find and Insert", "[HashMap]")
{
    MAP_TEST_ALL(TestMapMutation, 16);
}
TEST_CASE("HashMap: Insert after Remove (tombstone reuse)", "[HashMap]")
{
    MAP_TEST_ALL(TestMapInsertAfterRemove, 16);
}
TEST_CASE("HashMap<string, string>: basic operations", "[HashMap][string]")
{
    MAP_TEST_ALL(TestMapStringKV, 16);
}
TEST_CASE("HashMap<string, u32>: mixed key-value types", "[HashMap][string]")
{
    MAP_TEST_ALL(TestMapMixedKV, 16);
}

#undef MAP_TEST_ALL
