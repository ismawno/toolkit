#include "tkit/memory/stack_allocator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <vector>

using namespace TKit;

// A helper non-trivial type for Create/Destroy tests
struct NonTrivialSA
{
    static inline u32 CtorCount = 0;
    static inline u32 DtorCount = 0;
    u32 value;
    NonTrivialSA(const u32 p_Value) : value(p_Value)
    {
        ++CtorCount;
    }
    ~NonTrivialSA()
    {
        ++DtorCount;
    }
    NonTrivialSA(const NonTrivialSA &) = delete;
    NonTrivialSA &operator=(const NonTrivialSA &) = delete;
};

TEST_CASE("Constructor and initial state", "[StackAllocator]")
{
    constexpr usize size = 256;
    const StackAllocator arena(size);

    REQUIRE(arena.IsEmpty());
    REQUIRE(!arena.IsFull());
    REQUIRE(arena.GetSize() == size);
    REQUIRE(arena.GetAllocated() == 0);
    REQUIRE(arena.GetRemaining() == size);
    // no allocations yet → no pointer belongs
    u32 dummy;
    REQUIRE(!arena.Belongs(&dummy));
}

TEST_CASE("Allocate blocks and invariants", "[StackAllocator]")
{
    constexpr usize size = 64;
    StackAllocator arena(size);

    // Allocate two small blocks
    const void *p1 = arena.Allocate(16);
    REQUIRE(p1);
    REQUIRE(arena.Belongs(p1));
    REQUIRE(arena.GetAllocated() + arena.GetRemaining() == arena.GetSize());
    const void *p2 = arena.Allocate(8);
    REQUIRE(p2);
    REQUIRE(arena.Belongs(p2));
    REQUIRE(!arena.IsEmpty());
    // Deallocate in LIFO order
    arena.Deallocate(p2);
    REQUIRE(arena.GetRemaining() == arena.GetSize() - 16);
    arena.Deallocate(p1);
    REQUIRE(arena.IsEmpty());
}

TEST_CASE("Allocate<T> template overload", "[StackAllocator]")
{
    StackAllocator arena(128);
    const auto pi = arena.Allocate<u32>(4); // alloc 4×sizeof(u32)
    REQUIRE(pi);
    for (u32 i = 0; i < 4; ++i)
        pi[i] = i * 5;
    for (u32 i = 0; i < 4; ++i)
        REQUIRE(pi[i] == i * 5);
    arena.Deallocate(pi);
    REQUIRE(arena.IsEmpty());
}

TEST_CASE("Alignment behavior", "[StackAllocator]")
{
    constexpr usize size = 64;
    StackAllocator arena(size);

    constexpr usize align = 32;
    const void *p = arena.Allocate(1, align);
    REQUIRE(p);
    REQUIRE(reinterpret_cast<uptr>(p) % align == 0);
    arena.Deallocate(p);
    REQUIRE(arena.IsEmpty());
}

TEST_CASE("Create<T> and NCreate<T> with Destroy<T>", "[StackAllocator]")
{
    StackAllocator arena(256);

    // single Create
    NonTrivialSA::CtorCount = 0;
    NonTrivialSA::DtorCount = 0;
    const auto p = arena.Create<NonTrivialSA>(42);
    REQUIRE(p);
    REQUIRE(NonTrivialSA::CtorCount == 1);
    REQUIRE(p->value == 42);

    // array NCreate
    NonTrivialSA::CtorCount = 0;
    NonTrivialSA::DtorCount = 0;
    const auto arr = arena.NCreate<NonTrivialSA>(3, 7);
    REQUIRE(arr);
    REQUIRE(NonTrivialSA::CtorCount == 3);
    for (u32 i = 0; i < 3; ++i)
        REQUIRE(arr[i].value == 7);

    // Destroy all (LIFO)
    arena.Destroy(arr);
    arena.Destroy(p);
    REQUIRE(NonTrivialSA::DtorCount == 4);
    REQUIRE(arena.IsEmpty());
}

TEST_CASE("Top() entry reflects last allocation", "[StackAllocator]")
{
    StackAllocator arena(64);
    const auto p1 = arena.Allocate(8);
    const auto p2 = arena.Allocate(16);
    const auto &e = arena.Top();
    REQUIRE(e.Ptr == p2);
    // REQUIRE(e.Size == 16);
    arena.Deallocate(p2);
    arena.Deallocate(p1);
}

TEST_CASE("Allocate until full and LIFO deallocate", "[StackAllocator]")
{
    constexpr usize blockSize = 16;
    constexpr usize capacity = 128 / blockSize; // TKIT_STACK_ALLOCATOR_MAX_ENTRIES default ≥ capacity
    StackAllocator arena(capacity * blockSize);

    std::vector<const void *> ptrs;
    for (usize i = 0; i < capacity; ++i)
    {
        const void *p = arena.Allocate(blockSize, 1);
        REQUIRE(p);
        ptrs.push_back(p);
    }
    REQUIRE(arena.IsFull());
    REQUIRE(arena.GetRemaining() == 0);

    // deallocate all in reverse
    for (usize i = static_cast<usize>(ptrs.size()); i > 0; --i)
        arena.Deallocate(ptrs[i - 1]);
    REQUIRE(arena.IsEmpty());
}

TEST_CASE("Move constructor and move assignment", "[StackAllocator]")
{
    StackAllocator a1(128);
    a1.Allocate(32);
    const auto rem = a1.GetRemaining();

    StackAllocator a2(std::move(a1));
    REQUIRE(a2.GetRemaining() == rem);
    REQUIRE(a1.GetSize() == 0);

    StackAllocator a3(64);
    a3 = std::move(a2);
    REQUIRE(a3.GetRemaining() == rem);
    REQUIRE(a2.GetSize() == 0);
}

TEST_CASE("User-provided buffer constructor", "[StackAllocator]")
{
    constexpr usize size = 200;
    alignas(std::max_align_t) std::byte buffer[size];
    StackAllocator arena(buffer, size);

    REQUIRE(arena.IsEmpty());
    REQUIRE(arena.GetSize() == size);
    const void *p = arena.Allocate(32);
    REQUIRE(p);
    REQUIRE(arena.Belongs(p));
    arena.Deallocate(p);
    REQUIRE(arena.IsEmpty());
}
