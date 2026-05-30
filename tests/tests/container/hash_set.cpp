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

    const u32 &v0 = set.Insert(10u);
    const u32 &v1 = set.Insert(20u);
    const u32 &v2 = set.Insert(30u);
    REQUIRE(v0 == 10u);
    REQUIRE(v1 == 20u);
    REQUIRE(v2 == 30u);
    REQUIRE(set.GetSize() == 3);

    REQUIRE(set.Contains(10u));
    REQUIRE(set.Contains(20u));
    REQUIRE(set.Contains(30u));
    REQUIRE_FALSE(set.Contains(99u));

    // Find returns iterator that dereferences to const K&
    auto it = set.Find(10u);
    REQUIRE(*it == 10u);
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
    REQUIRE_FALSE(set.Contains(999u));
}

template <template <typename> typename A, typename... Args> void TestSetInsertDuplicate(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(1u);
    set.Insert(1u);

    // Insert doesn't deduplicate — each call adds
    REQUIRE(set.GetSize() == 3);
    REQUIRE(set.Contains(1u));
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
    REQUIRE(set.GetSize() == 2);
    REQUIRE_FALSE(set.Contains(2u));
    REQUIRE(set.Contains(1u));
    REQUIRE(set.Contains(3u));

    REQUIRE_FALSE(set.Remove(99u));
    REQUIRE(set.GetSize() == 2);
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

    REQUIRE(set.GetSize() == 1);
    REQUIRE_FALSE(set.Contains(5u));
    REQUIRE(set.Contains(6u));
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

    // re-insert after clear
    set.Insert(4u);
    REQUIRE(set.GetSize() == 1);
    REQUIRE(set.Contains(4u));
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

template <template <typename> typename A, typename... Args> void TestSetIteratorConst(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(2u);

    // both const and non-const iterators dereference to const K&
    const auto &cset = set;
    for (const u32 &key : cset)
        REQUIRE((key == 1u || key == 2u));

    for (const u32 &key : set)
        REQUIRE((key == 1u || key == 2u));
}

template <template <typename> typename A, typename... Args> void TestSetInsertAfterRemove(Args... args)
{
    using Node = SetNode<u32>;
    HashSet<u32, A<Node>> set{initialSize, A<Node>{args...}};

    set.Insert(1u);
    set.Insert(2u);
    set.Insert(3u);
    set.Remove(2u);
    REQUIRE(set.GetSize() == 2);

    // insert into tombstone slot
    set.Insert(4u);
    REQUIRE(set.Contains(4u));
    REQUIRE(set.Contains(1u));
    REQUIRE(set.Contains(3u));
    REQUIRE_FALSE(set.Contains(2u));
    REQUIRE(set.GetSize() == 3);

    // remove all, re-insert
    set.Remove(1u);
    set.Remove(3u);
    set.Remove(4u);
    REQUIRE(set.GetSize() == 0);

    set.Insert(100u);
    REQUIRE(set.GetSize() == 1);
    REQUIRE(set.Contains(100u));
}

template <template <typename> typename A, typename... Args> void TestSetStringKeys(Args... args)
{
    using Node = SetNode<std::string>;
    HashSet<std::string, A<Node>> set{initialSize, A<Node>{args...}};

    const std::string &r0 = set.Insert(std::string("alpha"));
    const std::string &r1 = set.Insert(std::string("beta"));
    const std::string &r2 = set.Insert(std::string("gamma"));
    REQUIRE(r0 == "alpha");
    REQUIRE(r1 == "beta");
    REQUIRE(r2 == "gamma");
    REQUIRE(set.GetSize() == 3);

    REQUIRE(set.Contains(std::string("alpha")));
    REQUIRE(set.Contains(std::string("beta")));
    REQUIRE(set.Contains(std::string("gamma")));
    REQUIRE_FALSE(set.Contains(std::string("delta")));

    // Find dereferences to const string
    REQUIRE(*set.Find(std::string("alpha")) == "alpha");

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
    const f32 lf1 = set.GetLoadFactor();
    REQUIRE(lf1 > 0.0f);
    REQUIRE(lf1 <= 1.0f);

    set.Insert(2u);
    REQUIRE(set.GetLoadFactor() > lf1);
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
TEST_CASE("HashSet: Insert does not deduplicate", "[HashSet]")
{
    SET_TEST_ALL(TestSetInsertDuplicate, 16);
}
TEST_CASE("HashSet: Remove by key", "[HashSet]")
{
    SET_TEST_ALL(TestSetRemoveByKey, 16);
}
TEST_CASE("HashSet: Remove by iterator", "[HashSet]")
{
    SET_TEST_ALL(TestSetRemoveByIterator, 16);
}
TEST_CASE("HashSet: Clear and re-insert", "[HashSet]")
{
    SET_TEST_ALL(TestSetClear, 16);
}
TEST_CASE("HashSet: Iteration", "[HashSet]")
{
    SET_TEST_ALL(TestSetIteration, 16);
}
TEST_CASE("HashSet: iterator yields const K&", "[HashSet]")
{
    SET_TEST_ALL(TestSetIteratorConst, 16);
}
TEST_CASE("HashSet: Insert after Remove (tombstone reuse)", "[HashSet]")
{
    SET_TEST_ALL(TestSetInsertAfterRemove, 16);
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
