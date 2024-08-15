#include "kit/memory/stack_allocator.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

KIT_NAMESPACE_BEGIN

template <typename T> void RunBasicConstructDestructOperations(StackAllocator &allocator)
{
    const T *ptr1 = allocator.Construct<T>();
    const T *ptr2 = allocator.Construct<T>();

    REQUIRE(ptr1 + 1 == ptr2);
    REQUIRE(allocator.Allocated() == 2 * sizeof(T));
    allocator.Destruct(ptr2);

    const T *ptr3 = allocator.Construct<T>();
    REQUIRE(ptr2 == ptr3);

    const T *ptr4 = allocator.NConstruct<T>(10);

    REQUIRE(allocator.Allocated() == 12 * sizeof(T));
    const T *ptr5 = allocator.Construct<T>();
    REQUIRE(ptr4 + 10 == ptr5);
    allocator.Destruct(ptr5);

    REQUIRE_THROWS(allocator.Destruct(ptr1));
}

TEST_CASE("Stack allocator basic operations", "[memory][stack_allocator][basic]")
{
    StackAllocator allocator(1024);
    REQUIRE(allocator.Size() == 1024);
    REQUIRE(allocator.Allocated() == 0);
    REQUIRE(allocator.Empty());

    allocator.Push(1024);
    REQUIRE(allocator.Allocated() == 1024);
    REQUIRE(allocator.Full());
    allocator.Pop();
    REQUIRE(allocator.Allocated() == 0);
    REQUIRE(allocator.Empty());

    REQUIRE_THROWS(allocator.Deallocate(nullptr));

    SECTION("Push and pop")
    {
        allocator.Push(128);
        REQUIRE(allocator.Allocated() == 128);

        allocator.Push(256);
        REQUIRE(allocator.Allocated() == 384);

        allocator.Pop();
        REQUIRE(allocator.Allocated() == 128);

        allocator.Pop();
        REQUIRE(allocator.Allocated() == 0);
    }

    SECTION("Construct and destroy")
    {
        RunBasicConstructDestructOperations<std::byte>(allocator);
    }
}

TEST_CASE("Stack allocator complex data operations", "[memory][stack_allocator][complex]")
{
    StackAllocator allocator(1024 * 5);
    SECTION("Construct and destruct with aligned data")
    {
        RunBasicConstructDestructOperations<AlignedData>(allocator);
    }

    SECTION("Fill allocator")
    {
        while (allocator.Fits(sizeof(AlignedData)))
            allocator.Construct<AlignedData>();
        REQUIRE_THROWS(allocator.Construct<AlignedData>());
        while (!allocator.Empty())
            allocator.Destruct(allocator.Top<AlignedData>());
    }
}

KIT_NAMESPACE_END