#include "kit/memory/stack_allocator.hpp"
#include "kit/core/literals.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

namespace KIT
{
using namespace Literals;

template <typename T> void RunBasicConstructDestructOperations()
{
    // Set the starting alignment...
    StackAllocator allocator(4_kb, alignof(T) < sizeof(void *) ? sizeof(void *) : alignof(T));
    allocator.Push<T>(10);

    // ...so that this is guaranteed to pass (in case I coded it right lol)
    REQUIRE(allocator.GetAllocated() == 10 * sizeof(T));
    allocator.Pop();

    allocator.Push<u32>();
    REQUIRE(allocator.GetAllocated() == sizeof(u32));
    T *ptr = allocator.Create<T>();
    REQUIRE(allocator.Top<T>() == ptr);

    usize factor = alignof(T) > alignof(u32) ? alignof(T) - sizeof(u32) % alignof(T) : 0;
    REQUIRE(allocator.GetAllocated() == sizeof(u32) + sizeof(T) + factor); // Because of the alignment

    REQUIRE(reinterpret_cast<const uptr>(ptr) % alignof(T) == 0);
    allocator.Destroy(ptr);
    allocator.Pop();

    allocator.Push<u8>();
    ptr = allocator.Create<T>();
    REQUIRE(allocator.Top<T>() == ptr);

    factor = alignof(T) > alignof(u8) ? alignof(T) - sizeof(u8) % alignof(T) : 0;
    REQUIRE(allocator.GetAllocated() == sizeof(u8) + sizeof(T) + factor); // Because of the alignment

    REQUIRE(reinterpret_cast<const uptr>(ptr) % alignof(T) == 0);
    allocator.Destroy(ptr);
    allocator.Pop();

    T *ptr1 = allocator.Create<T>();
    T *ptr2 = allocator.Create<T>();

    REQUIRE(ptr1 + 1 == ptr2);
    REQUIRE(allocator.GetAllocated() == 2 * sizeof(T));
    allocator.Destroy(ptr2);

    T *ptr3 = allocator.Create<T>();
    REQUIRE(ptr2 == ptr3);

    T *ptr4 = allocator.NConstruct<T>(10);

    REQUIRE(allocator.GetAllocated() == 12 * sizeof(T));
    T *ptr5 = allocator.Create<T>();
    REQUIRE(ptr4 + 10 == ptr5);
    allocator.Destroy(ptr5);

    allocator.Destroy(ptr4);
    allocator.Destroy(ptr3);
    allocator.Destroy(ptr1);
    REQUIRE(allocator.GetAllocated() == 0);
}

TEST_CASE("Stack allocator basic operations", "[memory][stack_allocator][basic]")
{
    StackAllocator allocator(1_kb);
    REQUIRE(allocator.GetSize() == 1_kb);
    REQUIRE(allocator.GetAllocated() == 0);
    REQUIRE(allocator.IsEmpty());

    allocator.Push(1_kb);
    REQUIRE(allocator.GetAllocated() == 1_kb);
    REQUIRE(allocator.IsFull());
    allocator.Pop();
    REQUIRE(allocator.GetAllocated() == 0);
    REQUIRE(allocator.IsEmpty());

    SECTION("Push and pop")
    {
        allocator.Push(128_b);
        REQUIRE(allocator.GetAllocated() == 128_b);

        allocator.Push(256_b);
        REQUIRE(allocator.GetAllocated() == 384_b);

        allocator.Pop();
        REQUIRE(allocator.GetAllocated() == 128_b);

        allocator.Pop();
        REQUIRE(allocator.GetAllocated() == 0);
    }

    SECTION("Create and destroy (std::byte)")
    {
        RunBasicConstructDestructOperations<std::byte>();
    }
    SECTION("Create and destroy (std::string)")
    {
        RunBasicConstructDestructOperations<std::string>();
    }
    SECTION("Create and destroy (NonTrivialData)")
    {
        RunBasicConstructDestructOperations<NonTrivialData>();
    }
    SECTION("Create and destroy (SmallData)")
    {
        RunBasicConstructDestructOperations<SmallData>();
    }
    SECTION("Create and destroy (BigData)")
    {
        RunBasicConstructDestructOperations<BigData>();
    }
}

TEST_CASE("Stack allocator complex data operations", "[memory][stack_allocator][complex]")
{
    StackAllocator allocator(5_kb);
    SECTION("Create and destruct with aligned data")
    {
        RunBasicConstructDestructOperations<AlignedData>();
    }

    SECTION("Fill allocator")
    {
        while (allocator.Push<AlignedData>())
            ;
        while (!allocator.IsEmpty())
            allocator.Pop();
    }
}
} // namespace KIT