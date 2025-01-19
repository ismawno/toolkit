#include "tkit/memory/arena_allocator.hpp"
#include "tkit/core/literals.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

namespace TKit
{
using namespace Literals;

TEST_CASE("Arena allocator basic operations", "[memory][arena_allocator][basic]")
{
    ArenaAllocator allocator(1_kb);
    REQUIRE(allocator.GetAllocated() == 0);
    REQUIRE(allocator.GetRemaining() == 1_kb);
    REQUIRE(allocator.IsEmpty());
    REQUIRE(!allocator.IsFull());

    SECTION("Allocate")
    {
        REQUIRE(allocator.Allocate(128_b) != nullptr);
        REQUIRE(allocator.GetAllocated() == 128_b);
        REQUIRE(allocator.GetRemaining() == 1_kb - 128_b);
        REQUIRE(!allocator.IsEmpty());
        REQUIRE(!allocator.IsFull());

        REQUIRE(allocator.Allocate(256_b) != nullptr);
        REQUIRE(allocator.GetAllocated() == 384_b);
        REQUIRE(allocator.GetRemaining() == 1_kb - 384_b);
        REQUIRE(!allocator.IsEmpty());
        REQUIRE(!allocator.IsFull());

        allocator.Reset();
        REQUIRE(allocator.GetAllocated() == 0);
        REQUIRE(allocator.GetRemaining() == 1_kb);
        REQUIRE(allocator.IsEmpty());
        REQUIRE(!allocator.IsFull());
    }

    SECTION("Reset")
    {
        allocator.Allocate(128_b);
        allocator.Reset();
        REQUIRE(allocator.GetAllocated() == 0);
        REQUIRE(allocator.GetRemaining() == 1_kb);
        REQUIRE(allocator.IsEmpty());
        REQUIRE(!allocator.IsFull());
    }

    SECTION("Belongs")
    {
        void *ptr = allocator.Allocate(128_b);
        REQUIRE(allocator.Belongs(ptr));
        REQUIRE(!allocator.Belongs(reinterpret_cast<void *>(0x12345678)));

        allocator.Reset();
    }
}

} // namespace TKit