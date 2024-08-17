#include "kit/memory/stack_allocator.hpp"
#include "kit/core/literals.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

KIT_NAMESPACE_BEGIN

using namespace Literals;

template <typename T> void RunBasicConstructDestructOperations(StackAllocator &allocator)
{
    allocator.Push<T>(10);
    REQUIRE(allocator.Allocated() == 10 * sizeof(T));
    allocator.Pop();

    const T *ptr1 = allocator.Create<T>();
    const T *ptr2 = allocator.Create<T>();

    REQUIRE(ptr1 + 1 == ptr2);
    REQUIRE(allocator.Allocated() == 2 * sizeof(T));
    allocator.Destroy(ptr2);

    const T *ptr3 = allocator.Create<T>();
    REQUIRE(ptr2 == ptr3);

    const T *ptr4 = allocator.NConstruct<T>(10);

    REQUIRE(allocator.Allocated() == 12 * sizeof(T));
    const T *ptr5 = allocator.Create<T>();
    REQUIRE(ptr4 + 10 == ptr5);
    allocator.Destroy(ptr5);

    REQUIRE_THROWS(allocator.Destroy(ptr1));
}

TEST_CASE("Stack allocator basic operations", "[memory][stack_allocator][basic]")
{
    StackAllocator allocator(1_kb);
    REQUIRE(allocator.Size() == 1_kb);
    REQUIRE(allocator.Allocated() == 0);
    REQUIRE(allocator.Empty());

    allocator.Push(1_kb);
    REQUIRE(allocator.Allocated() == 1_kb);
    REQUIRE(allocator.Full());
    allocator.Pop();
    REQUIRE(allocator.Allocated() == 0);
    REQUIRE(allocator.Empty());

    REQUIRE_THROWS(allocator.Deallocate(nullptr));

    SECTION("Push and pop")
    {
        allocator.Push(128_b);
        REQUIRE(allocator.Allocated() == 128_b);

        allocator.Push(256_b);
        REQUIRE(allocator.Allocated() == 384_b);

        allocator.Pop();
        REQUIRE(allocator.Allocated() == 128_b);

        allocator.Pop();
        REQUIRE(allocator.Allocated() == 0);
    }

    SECTION("Create and destroy")
    {
        RunBasicConstructDestructOperations<std::byte>(allocator);
    }
}

TEST_CASE("Stack allocator complex data operations", "[memory][stack_allocator][complex]")
{
    StackAllocator allocator(1024 * 5);
    SECTION("Create and destruct with aligned data")
    {
        RunBasicConstructDestructOperations<AlignedData>(allocator);
    }

    SECTION("Fill allocator")
    {
        while (allocator.Fits(sizeof(AlignedData)))
            allocator.Create<AlignedData>();
        REQUIRE_THROWS(allocator.Create<AlignedData>());
        while (!allocator.Empty())
            allocator.Destroy(allocator.Top<AlignedData>());
    }
}

KIT_NAMESPACE_END