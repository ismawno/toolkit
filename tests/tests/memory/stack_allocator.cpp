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
    const StackAllocator stack(size);

    REQUIRE(stack.IsEmpty());
    REQUIRE(!stack.IsFull());
    REQUIRE(stack.GetCapacity() == size);
    REQUIRE(stack.GetAllocatedBytes() == 0);
    REQUIRE(stack.GetRemainingBytes() == size);
    // no allocations yet → no pointer belongs
    u32 dummy = 0;
    REQUIRE(!stack.Belongs(&dummy));
}

TEST_CASE("Allocate blocks and invariants", "[StackAllocator]")
{
    constexpr usize size = 64;
    StackAllocator stack(size);

    // Allocate two small blocks
    const void *p1 = stack.Allocate(16);
    REQUIRE(p1);
    REQUIRE(stack.Belongs(p1));
    REQUIRE(stack.GetAllocatedBytes() + stack.GetRemainingBytes() == stack.GetCapacity());
    const void *p2 = stack.Allocate(8);
    REQUIRE(p2);
    REQUIRE(stack.Belongs(p2));
    REQUIRE(!stack.IsEmpty());
    // Deallocate in LIFO order
    stack.Deallocate(p2, 8);
    REQUIRE(stack.GetRemainingBytes() == stack.GetCapacity() - 16);
    stack.Deallocate(p1, 16);
    REQUIRE(stack.IsEmpty());
}

TEST_CASE("Allocate<T> template overload", "[StackAllocator]")
{
    StackAllocator stack(128);
    const auto pi = stack.Allocate<u32>(4); // alloc 4×sizeof(u32)
    REQUIRE(pi);
    for (u32 i = 0; i < 4; ++i)
        pi[i] = i * 5;
    for (u32 i = 0; i < 4; ++i)
        REQUIRE(pi[i] == i * 5);
    stack.Deallocate<u32>(pi, 4);
    REQUIRE(stack.IsEmpty());
}

TEST_CASE("Alignment behavior", "[StackAllocator]")
{
    constexpr usize size = 64;
    constexpr usize align = 32;
    StackAllocator stack(size, align);

    const void *p1 = stack.Allocate(1);
    const void *p2 = stack.Allocate(1);
    REQUIRE(p1);
    REQUIRE(p2);
    REQUIRE(reinterpret_cast<uptr>(p1) % align == 0);
    REQUIRE(reinterpret_cast<uptr>(p2) % align == 0);
    stack.Deallocate(p2, 1);
    stack.Deallocate(p1, 1);
    REQUIRE(stack.IsEmpty());
}

TEST_CASE("Create<T> and NCreate<T> with NDestroy<T>", "[StackAllocator]")
{
    StackAllocator stack(256);

    // single Create
    NonTrivialSA::CtorCount = 0;
    NonTrivialSA::DtorCount = 0;
    const NonTrivialSA *p = stack.Create<NonTrivialSA>(42);
    REQUIRE(p);
    REQUIRE(NonTrivialSA::CtorCount == 1);
    REQUIRE(p->value == 42);

    stack.Destroy(p);
    REQUIRE(NonTrivialSA::DtorCount == 1);

    // array NCreate
    NonTrivialSA::CtorCount = 0;
    NonTrivialSA::DtorCount = 0;
    const NonTrivialSA *arr = stack.NCreate<NonTrivialSA>(3, 7);
    REQUIRE(arr);
    REQUIRE(NonTrivialSA::CtorCount == 3);
    for (u32 i = 0; i < 3; ++i)
        REQUIRE(arr[i].value == 7);

    // Destroy all (LIFO)
    stack.NDestroy(arr, 3);
    REQUIRE(NonTrivialSA::DtorCount == 3);
    REQUIRE(stack.IsEmpty());
}

TEST_CASE("Allocate until full and LIFO deallocate", "[StackAllocator]")
{
    constexpr usize blockSize = 16;
    constexpr usize capacity = 128 / blockSize;
    StackAllocator stack(capacity * blockSize);

    std::vector<const void *> ptrs;
    for (usize i = 0; i < capacity; ++i)
    {
        const void *p = stack.Allocate(blockSize);
        REQUIRE(p);
        ptrs.push_back(p);
    }
    REQUIRE(stack.IsFull());
    REQUIRE(stack.GetRemainingBytes() == 0);

    // deallocate all in reverse
    for (usize i = static_cast<usize>(ptrs.size()); i > 0; --i)
        stack.Deallocate(ptrs[i - 1], blockSize);
    REQUIRE(stack.IsEmpty());
}

TEST_CASE("Move constructor and move assignment", "[StackAllocator]")
{
    StackAllocator a1(128);
    a1.Allocate(32);
    const auto rem = a1.GetRemainingBytes();

    StackAllocator a2(std::move(a1));
    REQUIRE(a2.GetRemainingBytes() == rem);
    REQUIRE(a1.GetCapacity() == 0);

    StackAllocator a3(64);
    a3 = std::move(a2);
    REQUIRE(a3.GetRemainingBytes() == rem);
    REQUIRE(a2.GetCapacity() == 0);
}

TEST_CASE("User-provided buffer constructor", "[StackAllocator]")
{
    constexpr usize size = 200;
    alignas(std::max_align_t) std::byte buffer[size];
    StackAllocator stack(buffer, size);

    REQUIRE(stack.IsEmpty());
    REQUIRE(stack.GetCapacity() == size);
    const void *p = stack.Allocate(32);
    REQUIRE(p);
    REQUIRE(stack.Belongs(p));
    stack.Deallocate(p, 32);
    REQUIRE(stack.IsEmpty());
}
