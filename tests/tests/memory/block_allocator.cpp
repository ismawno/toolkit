#include "tkit/memory/block_allocator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <vector>

using namespace TKit;

// A helper non-trivial type to test Create<T> and Destroy<T>
struct NonTrivialBA
{
    static inline u32 CtorCount = 0;
    static inline u32 DtorCount = 0;
    u32 value;

    NonTrivialBA(const u32 value) : value(value)
    {
        ++CtorCount;
    }
    ~NonTrivialBA()
    {
        ++DtorCount;
    }

    NonTrivialBA(const NonTrivialBA &) = delete;
    NonTrivialBA &operator=(const NonTrivialBA &) = delete;
};

TEST_CASE("Constructor and initial state", "[BlockAllocator]")
{
    constexpr usize bufSize = 1024;
    constexpr usize allocSize = 64;
    const BlockAllocator alloc(bufSize, allocSize);

    REQUIRE(!alloc.IsFull());
    REQUIRE(alloc.GetBufferSize() == bufSize);
    REQUIRE(alloc.GetAllocationSize() == allocSize);
    REQUIRE(alloc.GetAllocationCapacityCount() == bufSize / allocSize);

    u32 dummy = 0;
    REQUIRE(!alloc.Belongs(&dummy));
}

TEST_CASE("CreateFromType factory", "[BlockAllocator]")
{
    constexpr usize count = 10;
    auto alloc = BlockAllocator::CreateFromType<u32>(count);

    REQUIRE(alloc.GetAllocationCapacityCount() == count);
    REQUIRE(alloc.GetAllocationSize() >= sizeof(u32));
    REQUIRE(alloc.Belongs(alloc.Allocate()));
}

TEST_CASE("Allocate and Deallocate blocks", "[BlockAllocator]")
{
    constexpr usize capacity = 4;
    auto alloc = BlockAllocator::CreateFromType<u32>(capacity);

    std::vector<void *> ptrs;
    for (usize i = 0; i < capacity; ++i)
    {
        void *p = alloc.Allocate();
        REQUIRE(p);
        REQUIRE(alloc.Belongs(p));
        ptrs.push_back(p);
    }
    REQUIRE(alloc.IsFull());

    // Deallocate in same order
    for (usize i = 0; i < capacity; ++i)
    {
        alloc.Deallocate(ptrs[i]);
    }
}

TEST_CASE("Reset after all deallocations", "[BlockAllocator]")
{
    constexpr usize capacity = 3;
    auto alloc = BlockAllocator::CreateFromType<u32>(capacity);

    // allocate and deallocate all
    void *a = alloc.Allocate();
    void *b = alloc.Allocate();
    void *c = alloc.Allocate();
    alloc.Deallocate(a);
    alloc.Deallocate(b);
    alloc.Deallocate(c);

    alloc.Reset();

    // can allocate again to full capacity
    for (usize i = 0; i < capacity; ++i)
        REQUIRE(alloc.Allocate());
    REQUIRE(alloc.IsFull());
}

TEST_CASE("Move constructor and move assignment", "[BlockAllocator]")
{
    auto a1 = BlockAllocator::CreateFromType<u32>(5);
    void *ptr = a1.Allocate();
    REQUIRE(a1.Belongs(ptr));

    // move-construct
    BlockAllocator a2(std::move(a1));
    REQUIRE(a1.GetBufferSize() == 0);
    REQUIRE(a2.Belongs(ptr));

    // move-assign
    BlockAllocator a3 = BlockAllocator::CreateFromType<u32>(2);
    a3 = std::move(a2);
    REQUIRE(a2.GetBufferSize() == 0);
    REQUIRE(a3.Belongs(ptr));
    a3.Deallocate(ptr);
}

TEST_CASE("Create<T> and Destroy<T>", "[BlockAllocator]")
{
    auto alloc = BlockAllocator::CreateFromType<NonTrivialBA>(3);

    NonTrivialBA::CtorCount = 0;
    NonTrivialBA::DtorCount = 0;

    // Create three NonTrivialBA instances
    NonTrivialBA *a = alloc.Create<NonTrivialBA>(7);
    NonTrivialBA *b = alloc.Create<NonTrivialBA>(8);
    NonTrivialBA *c = alloc.Create<NonTrivialBA>(9);
    REQUIRE(NonTrivialBA::CtorCount == 3);
    REQUIRE(a->value == 7);
    REQUIRE(b->value == 8);
    REQUIRE(c->value == 9);

    // Destroy them
    alloc.Destroy(a);
    alloc.Destroy(b);
    alloc.Destroy(c);
    REQUIRE(NonTrivialBA::DtorCount == 3);
}

TEST_CASE("User-provided buffer constructor", "[BlockAllocator]")
{
    constexpr usize bufSize = 512;
    constexpr usize allocSize = 32;
    alignas(std::max_align_t) std::byte buffer[bufSize];
    BlockAllocator alloc(buffer, bufSize, allocSize);

    REQUIRE(alloc.GetBufferSize() == bufSize);
    REQUIRE(alloc.GetAllocationSize() == allocSize);

    const void *p = alloc.Allocate();
    REQUIRE(p);
    REQUIRE(alloc.Belongs(p));
}
