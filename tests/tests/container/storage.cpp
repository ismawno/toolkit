#include "tkit/container/storage.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using namespace TKit;
using namespace TKit::Alias;

static u32 g_CtorCount = 0;
static u32 g_DtorCount = 0;

TEST_CASE("RawStorage: trivial type construct and destruct", "[RawStorage]")
{
    RawStorage<sizeof(u32), alignof(u32)> storage;
    const u32 *pValue = storage.Construct<u32>(123);
    REQUIRE(*pValue == 123);
    storage.Destruct<u32>();
}

struct NT
{
    static inline u32 CtorCount = 0;
    static inline u32 DtorCount = 0;
    u32 value;
    NT(const u32 value) : value(value)
    {
        ++CtorCount;
    }
    ~NT()
    {
        ++DtorCount;
    }
};

TEST_CASE("RawStorage: non-trivial type construct and destruct", "[RawStorage]")
{
    RawStorage<sizeof(NT), alignof(NT)> storage;
    const NT *pObj = storage.Construct<NT>(77);
    REQUIRE(pObj->value == 77);
    REQUIRE(NT::CtorCount == 1);
    storage.Destruct<NT>();
    REQUIRE(NT::DtorCount == 1);
}

TEST_CASE("RawStorage: alignment correctness", "[RawStorage]")
{
    struct alignas(16) A16
    {
        char buf[16];
    };
    static_assert(alignof(A16) == 16, "alignment mismatch");

    RawStorage<sizeof(A16), 16> storage;
    const A16 *pA = storage.Construct<A16>();
    const uptr addr = reinterpret_cast<uintptr_t>(pA);
    REQUIRE((addr % alignof(A16)) == 0);
    storage.Destruct<A16>();
}

TEST_CASE("Storage: trivial type via constructor, destruct, reconstruct", "[Storage]")
{
    Storage<u32> s1(5);
    REQUIRE(*s1 == 5);

    s1.Destruct();
    *s1 = 9; // placement new left memory valid, assignment works
    REQUIRE(*s1 == 9);

    const u32 *pNew = s1.Construct(42);
    REQUIRE(*pNew == 42);
    s1.Destruct();
}

TEST_CASE("Storage: type without default constructor", "[Storage]")
{
    struct NoDef
    {
        u32 x;
        NoDef() = delete;
        NoDef(u32 v) : x(v)
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
    static inline u32 CtorCount = 0;
    static inline u32 CopyCtorCount = 0;
    static inline u32 MoveCtorCount = 0;
    u32 Val;
    STrack(const u32 value) : Val(value)
    {
        ++CtorCount;
    }
    STrack(const STrack &other) : Val(other.Val)
    {
        ++CopyCtorCount;
    }
    STrack(STrack &&other) : Val(other.Val)
    {
        ++MoveCtorCount;
    }
};

TEST_CASE("Storage: copy and move semantics", "[Storage]")
{
    // construct original
    g_CtorCount = g_DtorCount = 0;
    STrack::CtorCount = STrack::CopyCtorCount = STrack::MoveCtorCount = 0;
    Storage<STrack> orig(55);
    REQUIRE(STrack::CtorCount == 1);
    REQUIRE(orig->Val == 55);

    // copy-construct
    const Storage<STrack> copy = orig;
    REQUIRE(STrack::CopyCtorCount == 1);
    REQUIRE((*copy).Val == 55);

    // move-construct
    const Storage<STrack> moved = std::move(orig);
    REQUIRE(STrack::MoveCtorCount == 1);
    REQUIRE((*moved).Val == 55);
}
