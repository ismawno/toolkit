#include "tkit/memory/arena_allocator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <vector>

using namespace TKit;

// A helper non-trivial type to test Create/NCreate
struct NonTrivialAA
{
    static inline u32 CtorCount = 0;
    static inline u32 DtorCount = 0;
    u32 Value;

    NonTrivialAA(const u32 value) : Value(value)
    {
        ++CtorCount;
    }
    ~NonTrivialAA()
    {
        ++DtorCount;
    }

    // non-copyable to force Create path
    NonTrivialAA(const NonTrivialAA &) = delete;
    NonTrivialAA &operator=(const NonTrivialAA &) = delete;
};

TEST_CASE("Constructor and initial state", "[ArenaAllocator]")
{
    constexpr usize size = 1024;
    const ArenaAllocator arena(size);

    REQUIRE(arena.IsEmpty());
    REQUIRE(!arena.IsFull());
    REQUIRE(arena.GetCapacity() == size);
    REQUIRE(arena.GetAllocatedBytes() == 0);
    REQUIRE(arena.GetRemainingBytes() == size);
    // nothing has been allocated → no pointer should belong
    u32 dummy = 0;
    REQUIRE(!arena.Belongs(&dummy));
}

TEST_CASE("Allocate blocks and invariants", "[ArenaAllocator]")
{
    ArenaAllocator arena(256);
    const usize beforeRem = arena.GetRemainingBytes();

    // allocate 64 bytes
    const void *p = arena.Allocate(64);
    REQUIRE(p);
    REQUIRE(arena.Belongs(p));
    // allocated + remaining == total
    REQUIRE(arena.GetAllocatedBytes() + arena.GetRemainingBytes() == arena.GetCapacity());
    REQUIRE(arena.GetRemainingBytes() < beforeRem);

    // default Allocate<T>
    const auto pi = arena.Allocate<u32>(4);
    REQUIRE(pi != nullptr);
    // It's safe to write into it
    for (u32 i = 0; i < 4; ++i)
        pi[i] = i * 10;
    for (u32 i = 0; i < 4; ++i)
        REQUIRE(pi[i] == i * 10);
    REQUIRE(arena.Belongs(pi));
}

TEST_CASE("Alignment behavior", "[ArenaAllocator]")
{
    constexpr usize size = 64;
    constexpr usize align = 32;
    ArenaAllocator arena(size, align);

    const void *p1 = arena.Allocate(1);
    const void *p2 = arena.Allocate(1);
    REQUIRE(p1);
    REQUIRE(p2);
    REQUIRE(reinterpret_cast<uptr>(p1) % align == 0);
    REQUIRE(reinterpret_cast<uptr>(p2) % align == 0);
    arena.Reset();
}

TEST_CASE("Create<T> and NCreate<T>", "[ArenaAllocator]")
{
    ArenaAllocator arena(512);

    // Create a single u32
    const u32 *pi = arena.Create<u32>(42);
    REQUIRE(pi != nullptr);
    REQUIRE(*pi == 42);

    // Create an array of NonTrivialAA, track ctor/dtor
    NonTrivialAA::CtorCount = 0;
    NonTrivialAA::DtorCount = 0;
    const auto ptr = arena.NCreate<NonTrivialAA>(3, 7);
    REQUIRE(ptr);
    REQUIRE(NonTrivialAA::CtorCount == 3);
    for (u32 i = 0; i < 3; ++i)
        REQUIRE(ptr[i].Value == 7);

    // manually destroy them since ArenaAllocator won't call dtors
    for (u32 i = 0; i < 3; ++i)
        ptr[i].~NonTrivialAA();
    REQUIRE(NonTrivialAA::DtorCount == 3);
    arena.Reset();
}

TEST_CASE("Allocate until full and Reset", "[ArenaAllocator]")
{
    ArenaAllocator arena(64, 8);

    // consume all with 1-byte allocs, aligned to 1 byte
    std::vector<void *> ptrs;
    for (;;)
    {
        void *p = arena.Allocate(8);
        if (!p)
            break;
        ptrs.push_back(p);
    }

    REQUIRE(arena.IsFull());
    REQUIRE(arena.GetRemainingBytes() == 0);
    REQUIRE(arena.Allocate(1) == nullptr);
    REQUIRE(arena.GetAllocatedBytes() == static_cast<usize>(ptrs.size() * 8));

    // Reset and allocate again
    arena.Reset();
    REQUIRE(arena.IsEmpty());
    REQUIRE(arena.GetRemainingBytes() == arena.GetCapacity());
    const void *p2 = arena.Allocate(8);
    REQUIRE(p2 != nullptr);
    REQUIRE(!arena.IsEmpty());
    arena.Reset();
}

TEST_CASE("Move constructor and move assignment", "[ArenaAllocator]")
{
    ArenaAllocator a1(128);
    a1.Allocate(16);
    const auto rem1 = a1.GetRemainingBytes();

    // move-construct
    ArenaAllocator a2(std::move(a1));
    REQUIRE(a2.GetCapacity() == 128);
    REQUIRE(a2.GetRemainingBytes() == rem1);
    // a1 has been zeroed-out
    REQUIRE(a1.GetCapacity() == 0);
    REQUIRE(a1.GetRemainingBytes() == 0);

    // move-assign
    ArenaAllocator a3(64);
    a3 = std::move(a2);
    REQUIRE(a3.GetCapacity() == 128);
    REQUIRE(a3.GetRemainingBytes() == rem1);
    REQUIRE(a2.GetCapacity() == 0);
    REQUIRE(a2.GetRemainingBytes() == 0);
    a3.Reset();
}

TEST_CASE("User-provided buffer constructor", "[ArenaAllocator]")
{
    constexpr usize size = 256;
    alignas(std::max_align_t) std::byte buffer[size];
    ArenaAllocator arena(buffer, size);

    REQUIRE(arena.IsEmpty());
    REQUIRE(arena.GetCapacity() == size);

    const void *p = arena.Allocate(32);
    REQUIRE(p);
    REQUIRE(arena.Belongs(p));
    arena.Reset();
}
