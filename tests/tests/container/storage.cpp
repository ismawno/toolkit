#include "tkit/container/storage.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <type_traits>

using namespace TKit;
using namespace TKit::Alias;

static u32 g_CtorCount = 0;
static u32 g_DtorCount = 0;

TEST_CASE("RawStorage: trivial type construct and destruct", "[RawStorage]")
{
    RawStorage<sizeof(int), alignof(int)> storage;
    int *pValue = storage.Construct<int>(123);
    REQUIRE(*pValue == 123);
    storage.Destruct<int>();
}

struct NT
{
    static inline u32 s_CtorCount = 0;
    static inline u32 dtorCount = 0;
    u32 value;
    NT(u32 v) : value(v)
    {
        ++s_CtorCount;
    }
    ~NT()
    {
        ++dtorCount;
    }
};

TEST_CASE("RawStorage: non-trivial type construct and destruct", "[RawStorage]")
{
    RawStorage<sizeof(NT), alignof(NT)> storage;
    NT *pObj = storage.Construct<NT>(77);
    REQUIRE(pObj->value == 77);
    REQUIRE(NT::s_CtorCount == 1);
    storage.Destruct<NT>();
    REQUIRE(NT::dtorCount == 1);
}

TEST_CASE("RawStorage: alignment correctness", "[RawStorage]")
{
    struct alignas(16) A16
    {
        char buf[16];
    };
    static_assert(alignof(A16) == 16, "alignment mismatch");

    RawStorage<sizeof(A16), 16> storage;
    A16 *pA = storage.Construct<A16>();
    uintptr_t addr = reinterpret_cast<uintptr_t>(pA);
    REQUIRE((addr % alignof(A16)) == 0);
    storage.Destruct<A16>();
}

TEST_CASE("Storage: trivial type via constructor, destruct, reconstruct", "[Storage]")
{
    Storage<int> s1(5);
    REQUIRE(*s1 == 5);

    s1.Destruct();
    *s1 = 9; // placement new left memory valid, assignment works
    REQUIRE(*s1 == 9);

    int *pNew = s1.Construct(42);
    REQUIRE(*pNew == 42);
    s1.Destruct();
}

TEST_CASE("Storage: type without default constructor", "[Storage]")
{
    struct NoDef
    {
        u32 x;
        NoDef() = delete;
        explicit NoDef(u32 v) : x(v)
        {
        }
    };
    Storage<NoDef> s1(99);
    REQUIRE(s1->x == 99);

    s1.Destruct();
    s1.Construct(123);
    REQUIRE((*s1).x == 123);
    s1.Destruct();
}

struct STrack
{
    static inline u32 s_CtorCount = 0;
    static inline u32 s_CopyCtorCount = 0;
    static inline u32 s_MoveCtorCount = 0;
    u32 Val;
    STrack(u32 v) : Val(v)
    {
        ++s_CtorCount;
    }
    STrack(const STrack &o) : Val(o.Val)
    {
        ++s_CopyCtorCount;
    }
    STrack(STrack &&o) noexcept : Val(o.Val)
    {
        ++s_MoveCtorCount;
    }
};

TEST_CASE("Storage: copy and move semantics", "[Storage]")
{
    // construct original
    g_CtorCount = g_DtorCount = 0;
    STrack::s_CtorCount = STrack::s_CopyCtorCount = STrack::s_MoveCtorCount = 0;
    Storage<STrack> orig(55);
    REQUIRE(STrack::s_CtorCount == 1);
    REQUIRE(orig->Val == 55);

    // copy-construct
    Storage<STrack> copy = orig;
    REQUIRE(STrack::s_CopyCtorCount == 1);
    REQUIRE((*copy).Val == 55);

    // move-construct
    Storage<STrack> moved = std::move(orig);
    REQUIRE(STrack::s_MoveCtorCount == 1);
    REQUIRE((*moved).Val == 55);
}