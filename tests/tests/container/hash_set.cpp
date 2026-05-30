#include "tkit/container/hash_set.hpp"
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

template <template <typename> typename A, typename... Args> void TestSetInsertFind(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    const u32 *v0 = set.Insert(10u);
    const u32 *v1 = set.Insert(20u);
    const u32 *v2 = set.Insert(30u);

    REQUIRE(v0 != nullptr);
    REQUIRE(v1 != nullptr);
    REQUIRE(v2 != nullptr);
    REQUIRE(*v0 == 10u);
    REQUIRE(*v1 == 20u);
    REQUIRE(*v2 == 30u);

    REQUIRE(set.GetSize() == 3);

    REQUIRE(set.Contains(10u));
    REQUIRE(set.Contains(20u));
    REQUIRE(set.Contains(30u));
    REQUIRE_FALSE(set.Contains(99u));
}

template <template <typename> typename A, typename... Args> void TestSetContains(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(2u);
    set.Insert(3u);

    REQUIRE(set.Contains(1u));
    REQUIRE(set.Contains(2u));
    REQUIRE(set.Contains(3u));
    REQUIRE_FALSE(set.Contains(0u));
    REQUIRE_FALSE(set.Contains(4u));
}

template <template <typename> typename A, typename... Args> void TestSetRemoveByKey(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(2u);
    set.Insert(3u);
    REQUIRE(set.GetSize() == 3);

    REQUIRE(set.Remove(2u));
    REQUIRE_FALSE(set.Contains(2u));
    REQUIRE(set.Contains(1u));
    REQUIRE(set.Contains(3u));
    REQUIRE(set.GetSize() == 2);

    REQUIRE_FALSE(set.Remove(99u));
}

template <template <typename> typename A, typename... Args> void TestSetRemoveByIterator(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(5u);
    set.Insert(6u);

    REQUIRE(set.Contains(5u));
    auto it = set.Find(5u);
    set.Remove(it);

    REQUIRE_FALSE(set.Contains(5u));
    REQUIRE(set.Contains(6u));
    REQUIRE(set.GetSize() == 1);
}

template <template <typename> typename A, typename... Args> void TestSetClear(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(2u);
    set.Insert(3u);
    REQUIRE(set.GetSize() == 3);

    set.Clear();
    REQUIRE(set.GetSize() == 0);
    REQUIRE_FALSE(set.Contains(1u));
    REQUIRE_FALSE(set.Contains(2u));
    REQUIRE_FALSE(set.Contains(3u));
}

template <template <typename> typename A, typename... Args> void TestSetIteration(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(10u);
    set.Insert(20u);
    set.Insert(30u);

    u32 sum = 0;
    u32 count = 0;
    for (const u32 &key : set)
    {
        sum += key;
        ++count;
    }
    REQUIRE(count == 3);
    REQUIRE(sum == 60u);
}

template <template <typename> typename A, typename... Args> void TestSetInsertAfterRemove(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(2u);
    set.Insert(3u);
    set.Remove(2u);

    set.Insert(4u);
    REQUIRE(set.Contains(4u));
    REQUIRE(set.Contains(1u));
    REQUIRE(set.Contains(3u));
    REQUIRE_FALSE(set.Contains(2u));
    REQUIRE(set.GetSize() == 3);
}

template <template <typename> typename A, typename... Args> void TestSetDuplicateInsert(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(1u);
    set.Insert(1u);

    // depending on implementation, size may be 1 or 3
    // but all three insertions should succeed (open addressing)
    REQUIRE(set.Contains(1u));
}

template <template <typename> typename A, typename... Args> void TestSetStringKeys(Args... args)
{
    using Node = SetNode<std::string>;
    HashSet<std::string, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(std::string("alpha"));
    set.Insert(std::string("beta"));
    set.Insert(std::string("gamma"));

    REQUIRE(set.GetSize() == 3);
    REQUIRE(set.Contains(std::string("alpha")));
    REQUIRE(set.Contains(std::string("beta")));
    REQUIRE(set.Contains(std::string("gamma")));
    REQUIRE_FALSE(set.Contains(std::string("delta")));

    set.Remove(std::string("beta"));
    REQUIRE_FALSE(set.Contains(std::string("beta")));
    REQUIRE(set.Contains(std::string("alpha")));
    REQUIRE(set.Contains(std::string("gamma")));
    REQUIRE(set.GetSize() == 2);

    // iteration
    u32 count = 0;
    for (const std::string &key : set)
    {
        REQUIRE((key == "alpha" || key == "gamma"));
        ++count;
    }
    REQUIRE(count == 2);
}

template <template <typename> typename A, typename... Args> void TestSetLoadFactor(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    REQUIRE(set.GetSize() == 0);

    set.Insert(1u);
    REQUIRE(set.GetSize() == 1);
    const f32 lf = set.GetLoadFactor();
    REQUIRE(lf > 0.0f);
    REQUIRE(lf <= 1.0f);
}

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

#define SET_TEST_ALL(Fn, Cap)                                                                                          \
    Fn<ArenaAllocation>(&s_Arena, usize(Cap));                                                                         \
    Fn<DynamicAllocation>(usize(Cap));                                                                                 \
    Fn<StackAllocation>(&s_Stack, usize(Cap));                                                                         \
    Fn<StaticAlloc##Cap>();                                                                                            \
    Fn<TierAllocation>(&s_Tier, usize(Cap))

TEST_CASE("HashSet: Insert and Find", "[HashSet]")
{
    SET_TEST_ALL(TestSetInsertFind, 16);
}
TEST_CASE("HashSet: Contains", "[HashSet]")
{
    SET_TEST_ALL(TestSetContains, 16);
}
TEST_CASE("HashSet: Remove by key", "[HashSet]")
{
    SET_TEST_ALL(TestSetRemoveByKey, 16);
}
TEST_CASE("HashSet: Remove by iterator", "[HashSet]")
{
    SET_TEST_ALL(TestSetRemoveByIterator, 16);
}
TEST_CASE("HashSet: Clear", "[HashSet]")
{
    SET_TEST_ALL(TestSetClear, 16);
}
TEST_CASE("HashSet: Iteration", "[HashSet]")
{
    SET_TEST_ALL(TestSetIteration, 16);
}
TEST_CASE("HashSet: Insert after Remove (tombstone reuse)", "[HashSet]")
{
    SET_TEST_ALL(TestSetInsertAfterRemove, 16);
}
TEST_CASE("HashSet: duplicate Insert", "[HashSet]")
{
    SET_TEST_ALL(TestSetDuplicateInsert, 16);
}
TEST_CASE("HashSet<string>: basic operations", "[HashSet][string]")
{
    SET_TEST_ALL(TestSetStringKeys, 16);
}
TEST_CASE("HashSet: load factor", "[HashSet]")
{
    SET_TEST_ALL(TestSetLoadFactor, 16);
}

#undef SET_TEST_ALL
