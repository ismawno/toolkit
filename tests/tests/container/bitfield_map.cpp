#include "tkit/container/bitfield_map.hpp"
#include "tkit/container/arena_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/utils/literals.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <type_traits>

using namespace TKit;
using namespace TKit::Alias;

// Internal linkage: other test files define helpers with the same names (Test_Tracker, TestRemove...)
namespace
{
u32 g_Constructions = 0;
u32 g_Destructions = 0;
ArenaAllocator s_Arena{1_mib};
StackAllocator s_Stack{1_mib};
TierAllocator s_Tier{{.MaxAllocation = 16_kib}};

struct Test_Tracker
{
    u32 Value;
    Test_Tracker(const u32 value = 0) : Value(value)
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
    Test_Tracker &operator=(const Test_Tracker &other) = default;
    Test_Tracker &operator=(Test_Tracker &&other) = default;
};

template <typename T> using StaticAlloc16 = StaticAllocation<T, 16>;

// 4 bits: [0] Flat, [1] BackCull, [2..3] Blend (2-bit field, 4 values)
enum Flag : u8
{
    Flag_Flat = 1 << 0,
    Flag_BackCull = 1 << 1,
};
constexpr u8 BlendShift = 2;

struct FlatDesc
{
    static constexpr usize BitCount = 4;
    // Back culling is meaningless for flat geometry, so it is folded away
    static constexpr u8 Canonicalize(const u8 k)
    {
        return (k & Flag_Flat) ? u8(k & ~Flag_BackCull) : k;
    }
};

using Desc4 = IdentityFieldDesc<4>;
using Desc12 = IdentityFieldDesc<12>;

template <template <typename> typename A, typename T, typename Desc> using MapOf = BitFieldMap<T, Desc, A<T>>;

static_assert(std::is_same_v<MapOf<DynamicAllocation, u32, IdentityFieldDesc<1>>::Key, u8>);
static_assert(std::is_same_v<MapOf<DynamicAllocation, u32, Desc4>::Key, u8>);
static_assert(std::is_same_v<MapOf<DynamicAllocation, u32, IdentityFieldDesc<8>>::Key, u8>);
static_assert(std::is_same_v<MapOf<DynamicAllocation, u32, IdentityFieldDesc<9>>::Key, u16>);
static_assert(std::is_same_v<MapOf<DynamicAllocation, u32, Desc12>::Key, u16>);
static_assert(std::is_same_v<MapOf<DynamicAllocation, u32, IdentityFieldDesc<16>>::Key, u16>);

// Every key the sparse array claims is in the dense arrays and vice versa, keys and values stay in lockstep, and At()
// agrees with iteration
template <typename Map> void RequireConsistent(const Map &map)
{
    using Key = typename Map::Key;
    REQUIRE(map.GetKeys().GetSize() == map.GetSize());
    REQUIRE(usize(map.end() - map.begin()) == map.GetSize());

    usize found = 0;
    for (u32 k = 0; k < (1u << Map::BitCount); ++k)
    {
        bool inDense = false;
        for (const Key key : map.GetKeys())
            if (key == k)
            {
                REQUIRE_FALSE(inDense); // no duplicated keys in the dense array
                inDense = true;
            }
        // Contains() canonicalizes, so only check keys as they are stored
        if (inDense)
        {
            REQUIRE(map.Contains(Key(k)));
            ++found;
        }
    }
    REQUIRE(found == map.GetSize());
    for (usize i = 0; i < map.GetSize(); ++i)
    {
        REQUIRE(map.GetKey(i) == map.GetKeys()[i]);
        REQUIRE(&map.At(map.GetKey(i)) == map.GetData() + i);
    }
}

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

template <template <typename> typename A, typename... Args> void TestInsertAt(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    Map map{args...};

    REQUIRE(map.IsEmpty());
    REQUIRE(map.GetSize() == 0);

    REQUIRE(map.Insert(3, 30u) == 30u);
    REQUIRE(map.Insert(0, 0u) == 0u);
    map.Insert(15, 150u) = 155u;
    map.Insert(7, 70u);

    REQUIRE(map.GetSize() == 4);
    REQUIRE_FALSE(map.IsEmpty());
    REQUIRE(map.At(3) == 30u);
    REQUIRE(map.At(0) == 0u);
    REQUIRE(map.At(15) == 155u);
    REQUIRE(map[7] == 70u);

    map[3] = 33u;
    REQUIRE(map.At(3) == 33u);
    REQUIRE(map.GetSize() == 4);

    for (u8 k = 0; k < 16; ++k)
        REQUIRE(map.Contains(k) == (k == 0 || k == 3 || k == 7 || k == 15));

    // Dense storage keeps insertion order
    const u8 expectedKeys[] = {3, 0, 15, 7};
    const u32 expectedValues[] = {33, 0, 155, 70};
    usize i = 0;
    for (const u32 value : map)
    {
        REQUIRE(map.GetKey(i) == expectedKeys[i]);
        REQUIRE(value == expectedValues[i]);
        ++i;
    }
    REQUIRE(i == 4);

    RequireConsistent(map);
}

template <template <typename> typename A, typename... Args> void TestTryInsert(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    Map map{args...};

    bool inserted = false;
    REQUIRE(map.TryInsert(&inserted, 5, 50u) == 50u);
    REQUIRE(inserted);
    REQUIRE(map.GetSize() == 1);

    // Existing entry is returned untouched, no new entry created
    u32 &ref = map.TryInsert(&inserted, 5, 999u);
    REQUIRE_FALSE(inserted);
    REQUIRE(ref == 50u);
    REQUIRE(map.GetSize() == 1);

    ref = 51u;
    REQUIRE(map.At(5) == 51u);

    REQUIRE(map.TryInsert(9, 90u) == 90u);
    REQUIRE(map.TryInsert(9, 0u) == 90u);
    REQUIRE(map.GetSize() == 2);

    RequireConsistent(map);
}

template <template <typename> typename A, typename... Args> void TestRemove(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    Map map{args...};

    for (u8 k = 0; k < 8; ++k)
        map.Insert(k, u32(k) * 10u);

    // Middle: the back element is swapped into the hole, its sparse entry must follow it
    map.Remove(2);
    REQUIRE(map.GetSize() == 7);
    REQUIRE_FALSE(map.Contains(2));
    REQUIRE(map.At(7) == 70u);
    RequireConsistent(map);

    // Front
    map.Remove(0);
    REQUIRE_FALSE(map.Contains(0));
    REQUIRE(map.At(6) == 60u);
    RequireConsistent(map);

    // Back of the dense array
    const u8 back = map.GetKey(map.GetSize() - 1);
    map.Remove(back);
    REQUIRE_FALSE(map.Contains(back));
    RequireConsistent(map);

    for (u8 k = 0; k < 8; ++k)
        if (map.Contains(k))
            REQUIRE(map.At(k) == u32(k) * 10u);

    // Remove everything, then reinsert into the freed slots
    for (u8 k = 0; k < 8; ++k)
        map.TryRemove(k);
    REQUIRE(map.IsEmpty());
    for (u8 k = 0; k < 16; ++k)
        REQUIRE_FALSE(map.Contains(k));

    map.Insert(2, 222u);
    REQUIRE(map.At(2) == 222u);
    REQUIRE(map.GetSize() == 1);
    RequireConsistent(map);
}

template <template <typename> typename A, typename... Args> void TestTryRemove(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    Map map{args...};

    REQUIRE_FALSE(map.TryRemove(4));
    map.Insert(4, 40u);
    REQUIRE(map.TryRemove(4));
    REQUIRE_FALSE(map.TryRemove(4));
    REQUIRE(map.IsEmpty());
}

template <template <typename> typename A, typename... Args> void TestClear(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    Map map{args...};

    for (u8 k = 0; k < 16; k += 3)
        map.Insert(k, u32(k));
    map.Clear();

    REQUIRE(map.IsEmpty());
    for (u8 k = 0; k < 16; ++k)
        REQUIRE_FALSE(map.Contains(k));

    // Sparse entries were reset, so plain Insert (which asserts on duplicates) works again
    for (u8 k = 0; k < 16; k += 3)
        map.Insert(k, u32(k) + 1u);
    for (u8 k = 0; k < 16; k += 3)
        REQUIRE(map.At(k) == u32(k) + 1u);
    RequireConsistent(map);
}

template <template <typename> typename A, typename... Args> void TestOutOfRangeContains(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    Map map{args...};

    map.Insert(15, 1u);
    // Representable by the u8 key type but outside the 4 bit range
    REQUIRE_FALSE(map.Contains(16));
    REQUIRE_FALSE(map.Contains(0xFF));
}

template <template <typename> typename A, typename... Args> void TestWideKeys(Args... args)
{
    using Map = MapOf<A, u32, Desc12>;
    Map map{args...};

    const u16 keys[] = {0x000, 0x0FF, 0x100, 0x800, 0xFFF};
    for (const u16 k : keys)
        map.Insert(k, u32(k));

    for (const u16 k : keys)
        REQUIRE(map.At(k) == u32(k));

    REQUIRE_FALSE(map.Contains(0x7FF));
    REQUIRE_FALSE(map.Contains(0x1000));

    map.Remove(0x0FF);
    REQUIRE_FALSE(map.Contains(0x0FF));
    REQUIRE(map.At(0xFFF) == 0xFFFu);
    REQUIRE(map.GetSize() == 4);
}

template <template <typename> typename A, typename... Args> void TestCanonicalize(Args... args)
{
    using Map = MapOf<A, u32, FlatDesc>;
    Map map{args...};

    const u8 blend = u8(2 << BlendShift);
    const u8 flatCulled = u8(Flag_Flat | Flag_BackCull | blend);
    const u8 flat = u8(Flag_Flat | blend);

    // Inserting the non-canonical key stores the canonical one
    map.Insert(flatCulled, 1u);
    REQUIRE(map.GetSize() == 1);
    REQUIRE(map.GetKey(0) == flat);

    // Both spellings reach the same entry
    REQUIRE(map.Contains(flat));
    REQUIRE(map.Contains(flatCulled));
    REQUIRE(&map.At(flat) == &map.At(flatCulled));

    bool inserted = true;
    REQUIRE(map.TryInsert(&inserted, flat, 2u) == 1u);
    REQUIRE_FALSE(inserted);
    REQUIRE(map.GetSize() == 1);

    // Non-flat keys keep the culling bit, so these are two distinct entries
    const u8 culled = u8(Flag_BackCull | blend);
    map.Insert(culled, 3u);
    map.Insert(blend, 4u);
    REQUIRE(map.GetSize() == 3);
    REQUIRE(map.At(culled) == 3u);
    REQUIRE(map.At(blend) == 4u);

    // Removing through the non-canonical spelling removes the canonical entry
    map.Remove(flatCulled);
    REQUIRE_FALSE(map.Contains(flat));
    REQUIRE_FALSE(map.Contains(flatCulled));
    REQUIRE(map.GetSize() == 2);

    REQUIRE(map.TryRemove(culled));
    REQUIRE(map.GetSize() == 1);

    for (const u8 key : map.GetKeys())
        REQUIRE(FlatDesc::Canonicalize(key) == key);
}

template <template <typename> typename A, typename... Args> void TestCopy(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    Map src{args...};
    for (u8 k = 1; k < 16; k += 2)
        src.Insert(k, u32(k));

    Map copy{src};
    REQUIRE(copy.GetSize() == src.GetSize());
    for (u8 k = 1; k < 16; k += 2)
        REQUIRE(copy.At(k) == u32(k));
    RequireConsistent(copy);

    // Copies are independent
    copy.At(1) = 100u;
    copy.Remove(3);
    REQUIRE(src.At(1) == 1u);
    REQUIRE(src.Contains(3));
    RequireConsistent(src);
    RequireConsistent(copy);

    Map assigned{};
    assigned = src;
    REQUIRE(assigned.GetSize() == src.GetSize());
    for (u8 k = 1; k < 16; k += 2)
        REQUIRE(assigned.At(k) == u32(k));
    assigned.Remove(5);
    REQUIRE(src.Contains(5));
    RequireConsistent(assigned);
}

template <typename Map> void RequireMovedFrom(const Map &src, const usize sizeBefore)
{
    RequireConsistent(src);
    if constexpr (Map::Type == Array_Static)
        REQUIRE(src.GetSize() == sizeBefore); // static storage element-moves and keeps its size
    else
    {
        REQUIRE(src.IsEmpty());
        for (u8 k = 0; k < 16; ++k)
            REQUIRE_FALSE(src.Contains(k));
    }
}

template <template <typename> typename A, typename... Args> void TestMove(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;

    {
        Map src{args...};
        for (u8 k = 0; k < 16; k += 2)
            src.Insert(k, u32(k));
        const usize size = src.GetSize();

        Map dst{std::move(src)};
        REQUIRE(dst.GetSize() == size);
        for (u8 k = 0; k < 16; k += 2)
            REQUIRE(dst.At(k) == u32(k));
        RequireConsistent(dst);
        RequireMovedFrom(src, size);
    }
    {
        Map src{args...};
        for (u8 k = 1; k < 16; k += 4)
            src.Insert(k, u32(k));
        const usize size = src.GetSize();

        Map dst{};
        dst = std::move(src);
        REQUIRE(dst.GetSize() == size);
        for (u8 k = 1; k < 16; k += 4)
            REQUIRE(dst.At(k) == u32(k));
        RequireConsistent(dst);
        RequireMovedFrom(src, size);
    }
    {
        // Self move assignment leaves the map intact
        Map map{args...};
        map.Insert(6, 60u);
        Map &alias = map;
        map = std::move(alias);
        REQUIRE(map.GetSize() == 1);
        REQUIRE(map.At(6) == 60u);
        RequireConsistent(map);
    }
}

template <template <typename> typename A, typename... Args> void TestMovedFromIsReusable(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    if constexpr (Map::Type != Array_Static)
    {
        Map src{args...};
        src.Insert(3, 30u);
        src.Insert(9, 90u);
        {
            const Map dst{std::move(src)};
            REQUIRE(dst.At(3) == 30u);
            RequireConsistent(dst);
        }

        // Moved-from heap storage has no allocation left, give it a fresh one (scoped above to keep stack order)
        src = Map{args...};

        // Would trip the duplicate assert if stale sparse entries survived the move
        src.Insert(3, 31u);
        src.Insert(9, 91u);
        REQUIRE(src.At(3) == 31u);
        REQUIRE(src.At(9) == 91u);
        RequireConsistent(src);
    }
}

template <template <typename> typename A, typename... Args> void TestConvertingCopyMove(Args... args)
{
    using Map = MapOf<A, u32, Desc4>;
    using DynMap = MapOf<DynamicAllocation, u32, Desc4>;
    using StaticMap = MapOf<StaticAlloc16, u32, Desc4>;

    Map src{args...};
    for (u8 k = 0; k < 16; k += 5)
        src.Insert(k, u32(k));
    const usize size = src.GetSize();

    const DynMap dyn{src};
    const StaticMap stat{src};
    REQUIRE(dyn.GetSize() == size);
    REQUIRE(stat.GetSize() == size);
    for (u8 k = 0; k < 16; k += 5)
    {
        REQUIRE(dyn.At(k) == u32(k));
        REQUIRE(stat.At(k) == u32(k));
    }
    RequireConsistent(dyn);
    RequireConsistent(stat);

    DynMap assigned{};
    assigned = src;
    REQUIRE(assigned.GetSize() == size);
    RequireConsistent(assigned);

    // There is no cross allocator move for the dense array, so a converting move copies and the source must stay
    // intact and consistent. With A = DynamicAllocation this is a plain same type move, which empties the source
    DynMap moved{std::move(src)};
    REQUIRE(moved.GetSize() == size);
    RequireConsistent(moved);
    RequireConsistent(src);
    if constexpr (std::is_same_v<Map, DynMap>)
        REQUIRE(src.IsEmpty());
    else
        REQUIRE(src.GetSize() == size);

    DynMap moveAssigned{};
    moveAssigned = std::move(src);
    if constexpr (!std::is_same_v<Map, DynMap>)
        REQUIRE(moveAssigned.GetSize() == size);
    RequireConsistent(moveAssigned);
    RequireConsistent(src);
}

template <template <typename> typename A, typename... Args> void TestQuery(Args... args)
{
    // Same layout as FlatDesc: [0] Flat, [1] BackCull, [2..3] Blend
    constexpr u8 flatMask = Flag_Flat;
    constexpr u8 cullMask = Flag_BackCull;
    constexpr u8 blendMask = u8(0b11 << BlendShift);
    constexpr u8 fullMask = u8(flatMask | cullMask | blendMask);
    const auto blend = [](const u32 b) { return u8(b << BlendShift); };

    using Map = MapOf<A, u32, Desc4>;
    Map map{args...};
    for (u8 k = 0; k < 16; ++k)
        map.Insert(k, u32(k) * 10u);

    // Counts the matches, checking each one actually satisfies (key & mask) == partial
    const auto count = [&](const u8 partial, const u8 mask) {
        usize n = 0;
        map.Query(partial, mask, [&](const u8 key, const u32 value) {
            REQUIRE((key & mask) == partial);
            REQUIRE(value == u32(key) * 10u);
            ++n;
        });
        return n;
    };

    // A single flag set, and the same flag queried for zero
    REQUIRE(count(Flag_Flat, flatMask) == 8);
    REQUIRE(count(0, flatMask) == 8);
    REQUIRE(count(0, cullMask) == 8);

    // Every value of the 2 bit field, zero included. Blend 2 (0b10) must not match blend 3 (0b11), which a plain
    // "bits are set" test would
    for (u32 b = 0; b < 4; ++b)
    {
        usize n = 0;
        map.Query(blend(b), blendMask, [&](const u8 key, const u32) {
            REQUIRE(u32(key >> BlendShift) == b);
            ++n;
        });
        REQUIRE(n == 4);
    }

    // Several fields at once, with the remaining one ignored: blend 1 and flat, any culling -> 0101, 0111
    REQUIRE(count(u8(blend(1) | Flag_Flat), u8(blendMask | flatMask)) == 2);
    // A zero field combined with a set one: blend 0 and culled, any flat -> 0010, 0011
    REQUIRE(count(Flag_BackCull, u8(blendMask | cullMask)) == 2);
    // Everything zero, every field constrained: exactly key 0
    REQUIRE(count(0, fullMask) == 1);

    // The full mask is an exact lookup
    for (u8 k = 0; k < 16; ++k)
        REQUIRE(count(k, fullMask) == 1);

    // An empty mask matches everything
    REQUIRE(count(0, 0) == 16);

    // mask == partial is the old "these bits are set" query
    REQUIRE(count(0b0101, 0b0101) == 4); // 0101, 0111, 1101, 1111

    // Value only callback
    u32 sum = 0;
    map.Query(blend(3), blendMask, [&](const u32 value) { sum += value; });
    REQUIRE(sum == (12u + 13u + 14u + 15u) * 10u);

    // Mutation through the non const overload: bump every blend 2 entry
    map.Query(blend(2), blendMask, [](u32 &value) { value += 1u; });
    for (u8 k = 0; k < 16; ++k)
        REQUIRE(map.At(k) == u32(k) * 10u + ((k >> BlendShift) == 2 ? 1u : 0u));
    map.Query(blend(2), blendMask, [](const u8 key, u32 &value) { value = u32(key) * 10u; });

    // Returning bool stops early, and Query reports whether it ran to completion
    usize visited = 0;
    REQUIRE_FALSE(map.Query(0, flatMask, [&](const u32) { return ++visited < 3; }));
    REQUIRE(visited == 3);

    visited = 0;
    REQUIRE(map.Query(0, flatMask, [&](const u8, const u32) { return ++visited != 0; }));
    REQUIRE(visited == 8);

    // Removed keys are not visited, and a query with no matches completes
    map.Remove(15);
    visited = 0;
    REQUIRE(map.Query(15, fullMask, [&](const u32) { return ++visited != 0; }));
    REQUIRE(visited == 0);

    // Const overload
    const Map &cmap = map;
    visited = 0;
    cmap.Query(blend(3), blendMask, [&](const u32 &) { ++visited; });
    REQUIRE(visited == 3);
    cmap.Query(u8(blend(3) | Flag_Flat), u8(blendMask | flatMask), [&](const u8 key, const u32 &) {
        REQUIRE(key == 0b1101);
        ++visited;
    });
    REQUIRE(visited == 4);

    // Queries see canonical keys: flat entries had their culling bit folded away, so querying culling only finds the
    // non flat ones
    using FlatMap = MapOf<A, u32, FlatDesc>;
    FlatMap fmap{args...};
    for (u8 k = 0; k < 16; ++k)
        fmap.TryInsert(k, u32(k));
    REQUIRE(fmap.GetSize() == 12);

    usize culled = 0;
    fmap.Query(Flag_BackCull, cullMask, [&](const u8 key, const u32) {
        REQUIRE((key & Flag_Flat) == 0);
        ++culled;
    });
    REQUIRE(culled == 4); // 0010, 0110, 1010, 1110

    usize flatCulled = 0;
    fmap.Query(u8(Flag_Flat | Flag_BackCull), u8(flatMask | cullMask), [&](const u32) { ++flatCulled; });
    REQUIRE(flatCulled == 0);
}

template <template <typename> typename A, typename... Args> void TestLifetimes(Args... args)
{
    using Map = MapOf<A, Test_Tracker, Desc4>;
    g_Constructions = g_Destructions = 0;
    {
        Map map{args...};
        for (u8 k = 0; k < 8; ++k)
            map.Insert(k, u32(k));
        map.TryInsert(3, 999u); // existing, must not construct a value
        map.Remove(1);
        map.Remove(7);
        REQUIRE(map.At(6).Value == 6u);
        REQUIRE(map.GetSize() == 6);

        Map copy{map};
        REQUIRE(copy.At(6).Value == 6u);
        map.Clear();
        REQUIRE(map.IsEmpty());
        REQUIRE(copy.GetSize() == 6);
    }
    REQUIRE(g_Constructions == g_Destructions);
}

} // namespace

// ---------------------------------------------------------------------------
// TEST_CASEs
// ---------------------------------------------------------------------------

// Non-static allocations pick up the pushed allocators when the capacity constructor reserves
#define BFM_TEST_ALL(Fn)                                                                                               \
    PushArena(&s_Arena);                                                                                               \
    PushStack(&s_Stack);                                                                                               \
    PushTier(&s_Tier);                                                                                                 \
    Fn<ArenaAllocation>(usize(16));                                                                                    \
    Fn<DynamicAllocation>(usize(16));                                                                                  \
    Fn<StackAllocation>(usize(16));                                                                                    \
    Fn<StaticAlloc16>();                                                                                               \
    Fn<TierAllocation>(usize(16));                                                                                     \
    PopTier();                                                                                                         \
    PopStack();                                                                                                        \
    PopArena()

TEST_CASE("BitFieldMap: Insert and At", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestInsertAt);
}
TEST_CASE("BitFieldMap: TryInsert semantics", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestTryInsert);
}
TEST_CASE("BitFieldMap: Remove keeps sparse and dense in sync", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestRemove);
}
TEST_CASE("BitFieldMap: TryRemove", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestTryRemove);
}
TEST_CASE("BitFieldMap: Clear and re-insert", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestClear);
}
TEST_CASE("BitFieldMap: Contains with out of range keys", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestOutOfRangeContains);
}
TEST_CASE("BitFieldMap: 12 bit keys", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestWideKeys);
}
TEST_CASE("BitFieldMap: Canonicalize folds equivalent keys", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestCanonicalize);
}
TEST_CASE("BitFieldMap: Copy construction and assignment", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestCopy);
}
TEST_CASE("BitFieldMap: Move construction and assignment", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestMove);
}
TEST_CASE("BitFieldMap: Moved-from map is reusable", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestMovedFromIsReusable);
}
TEST_CASE("BitFieldMap: Converting copy and move", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestConvertingCopyMove);
}
TEST_CASE("BitFieldMap: Query by partial key", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestQuery);
}
TEST_CASE("BitFieldMap: Element lifetimes", "[BitFieldMap]")
{
    BFM_TEST_ALL(TestLifetimes);
}
