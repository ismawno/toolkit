#include "tkit/memory/arena_allocator.hpp"
#include "tkit/container/dynamic_array.hpp"
#include <catch2/catch_all.hpp>
#include <cstddef>

using namespace TKit;

struct WarningShutter : Catch::EventListenerBase
{
    using EventListenerBase::EventListenerBase;

    void testCaseStarting(const Catch::TestCaseInfo &) override
    {
        TKIT_IGNORE_WARNING_LOGS_PUSH();
    }

    void testCaseEnded(const Catch::TestCaseStats &) override
    {
        TKIT_IGNORE_WARNING_LOGS_POP();
    }
};

CATCH_REGISTER_LISTENER(WarningShutter)

// A helper non-trivial type to test Create/NCreate
struct NonTrivialAA
{
    static inline u32 CtorCount = 0;
    static inline u32 DtorCount = 0;
    u32 Value;

    NonTrivialAA(const u32 p_Value) : Value(p_Value)
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
    REQUIRE(arena.GetSize() == size);
    REQUIRE(arena.GetAllocated() == 0);
    REQUIRE(arena.GetRemaining() == size);
    // nothing has been allocated → no pointer should belong
    u32 dummy;
    REQUIRE(!arena.Belongs(&dummy));
}

TEST_CASE("Allocate blocks and invariants", "[ArenaAllocator]")
{
    ArenaAllocator arena(256);
    const usize beforeRem = arena.GetRemaining();

    // allocate 64 bytes
    const void *p = arena.Allocate(64);
    REQUIRE(p);
    REQUIRE(arena.Belongs(p));
    // allocated + remaining == total
    REQUIRE(arena.GetAllocated() + arena.GetRemaining() == arena.GetSize());
    REQUIRE(arena.GetRemaining() < beforeRem);

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
    constexpr usize size = 128;
    ArenaAllocator arena(size);

    // ask for 1 byte but 32-byte alignment
    constexpr usize align = 32;
    const void *p = arena.Allocate(1, align);
    REQUIRE(p);
    REQUIRE(arena.Belongs(p));
    REQUIRE(reinterpret_cast<uptr>(p) % align == 0);
}

TEST_CASE("Allocate until full and Reset", "[ArenaAllocator]")
{
    ArenaAllocator arena(64);

    // consume all with 1-byte allocs, aligned to 1 byte
    DynamicArray<void *> ptrs;
    while (true)
    {
        void *p = arena.Allocate(1, 1);
        if (!p)
            break;
        ptrs.Append(p);
    }

    REQUIRE(arena.IsFull());
    REQUIRE(arena.GetRemaining() == 0);
    REQUIRE(arena.Allocate(1) == nullptr);
    REQUIRE(arena.GetAllocated() == ptrs.GetSize());

    // Reset and allocate again
    arena.Reset();
    REQUIRE(arena.IsEmpty());
    REQUIRE(arena.GetRemaining() == arena.GetSize());
    const void *p2 = arena.Allocate(8);
    REQUIRE(p2 != nullptr);
    REQUIRE(!arena.IsEmpty());
}

TEST_CASE("Move constructor and move assignment", "[ArenaAllocator]")
{
    ArenaAllocator a1(128);
    a1.Allocate(16);
    const auto rem1 = a1.GetRemaining();

    // move-construct
    ArenaAllocator a2(std::move(a1));
    REQUIRE(a2.GetSize() == 128);
    REQUIRE(a2.GetRemaining() == rem1);
    // a1 has been zeroed-out
    REQUIRE(a1.GetSize() == 0);
    REQUIRE(a1.GetRemaining() == 0);

    // move-assign
    ArenaAllocator a3(64);
    a3 = std::move(a2);
    REQUIRE(a3.GetSize() == 128);
    REQUIRE(a3.GetRemaining() == rem1);
    REQUIRE(a2.GetSize() == 0);
    REQUIRE(a2.GetRemaining() == 0);
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
}

TEST_CASE("User-provided buffer constructor", "[ArenaAllocator]")
{
    constexpr usize size = 256;
    alignas(std::max_align_t) std::byte buffer[size];
    ArenaAllocator arena(buffer, size);

    REQUIRE(arena.IsEmpty());
    REQUIRE(arena.GetSize() == size);

    const void *p = arena.Allocate(32);
    REQUIRE(p);
    REQUIRE(arena.Belongs(p));
}
