#include "tkit/memory/block_allocator.hpp"
#include "tkit/multiprocessing/thread_pool.hpp"
#include "tkit/multiprocessing/for_each.hpp"
#include "tkit/container/array.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>

namespace TKit
{
template <typename T> static void RunRawAllocationTest()
{
    BlockAllocator allocator = BlockAllocator::Create<T>(10);
    REQUIRE(allocator.IsEmpty());
    REQUIRE(allocator.GetBlockCount() == 0);

    SECTION("Allocate and deallocate (raw call)")
    {
        T *data = allocator.Allocate();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Belongs(data));
        allocator.Deallocate(data);
        REQUIRE(allocator.IsEmpty());
    }

    SECTION("Create and destroy (raw call)")
    {
        T *data = allocator.Create();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Belongs(data));
        allocator.Destroy(data);
        REQUIRE(allocator.IsEmpty());
    }

    SECTION("Allocate and deallocate multiple (raw call)")
    {
        constexpr u32 amount = 1000;
        for (u32 j = 0; j < 2; ++j)
        {
            HashSet<T *> allocated;
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = allocator.Allocate();
                REQUIRE(ptr != nullptr);
                REQUIRE(allocated.insert(ptr).second);
                REQUIRE(allocator.Belongs(ptr));
            }
            REQUIRE(allocator.GetAllocationCount() == amount);

            for (T *ptr : allocated)
                allocator.Deallocate(ptr);

            // Reuse the same chunk over and over again
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = allocator.Allocate();
                REQUIRE(ptr != nullptr);
                REQUIRE(allocator.Belongs(ptr));
                allocator.Deallocate(ptr);
            }
            REQUIRE(allocator.GetBlockCount() == amount / 10);
        }
        REQUIRE(allocator.IsEmpty());
    }

    SECTION("Assert contiguous (raw call)")
    {
        constexpr u32 amount = 10;
        Array<T *, amount> data;
        const usize chunkSize = allocator.GetChunkSize();
        for (u32 i = 0; i < amount; ++i)
        {
            data[i] = allocator.Allocate();
            REQUIRE(data[i] != nullptr);
            REQUIRE(allocator.Belongs(data[i]));
            if (i != 0)
            {
                std::byte *b1 = reinterpret_cast<std::byte *>(data[i - 1]);
                std::byte *b2 = reinterpret_cast<std::byte *>(data[i]);
                REQUIRE(b2 == b1 + chunkSize);
            }
        }
        for (u32 i = 0; i < amount; ++i)
            allocator.Deallocate(data[i]);
        REQUIRE(allocator.IsEmpty());
    }
}

template <typename T> static void RunNewDeleteTest()
{
    BlockAllocator &allocator = Detail::GetBlockAllocatorInstance<T, 10>();
    REQUIRE(allocator.IsEmpty());
    allocator.Reset();

    SECTION("Allocate and deallocate (new/delete)")
    {
        T *data = new T;
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Belongs(data));
        delete data;
        REQUIRE(allocator.IsEmpty());
    }

    SECTION("Allocate and deallocate multiple (new/delete)")
    {
        constexpr u32 amount = 1000;
        for (u32 j = 0; j < 2; ++j)
        {
            HashSet<T *> allocated;
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = new T;
                REQUIRE(ptr != nullptr);
                REQUIRE(allocated.insert(ptr).second);
                REQUIRE(allocator.Belongs(ptr));
            }
            REQUIRE(allocator.GetAllocationCount() == amount);
            for (T *ptr : allocated)
                delete ptr;

            // Reuse the same chunk over and over again
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = new T;
                REQUIRE(ptr != nullptr);
                REQUIRE(allocator.Belongs(ptr));
                delete ptr;
            }

            REQUIRE(allocator.GetBlockCount() == amount / 10);
        }
        REQUIRE(allocator.IsEmpty());
    }

    SECTION("Assert contiguous (new/delete)")
    {
        constexpr u32 amount = 10;
        Array<T *, amount> data;
        constexpr usize chunkSize = BlockAllocator<T>::GetChunkSize();
        for (u32 i = 0; i < amount; ++i)
        {
            data[i] = new T;
            REQUIRE(data[i] != nullptr);
            REQUIRE(allocator.Belongs(data[i]));
            if (i != 0)
            {
                std::byte *b1 = reinterpret_cast<std::byte *>(data[i - 1]);
                std::byte *b2 = reinterpret_cast<std::byte *>(data[i]);
                REQUIRE(b2 == b1 + chunkSize);
            }
        }
        for (u32 i = 0; i < amount; ++i)
            delete data[i];
        REQUIRE(allocator.IsEmpty());
    }
}

template <typename Base, typename Derived> void RunVirtualAllocatorTests()
{
    BlockAllocator &allocator = GetGlobalBlockAllocatorInstance<Derived, 10>();
    REQUIRE(allocator.IsEmpty());
    allocator.Reset();

    SECTION("Virtual deallocations")
    {
        constexpr usize amount = 1000;
        for (usize j = 0; j < 2; ++j)
        {
            HashSet<Base *> allocated;
            for (usize i = 0; i < amount; ++i)
            {
                Derived *vd = new Derived;
                Base *vb = vd;
                vb->SetValues();

                REQUIRE(vd->x == 10);
                REQUIRE(vd->y == 20.0);
                REQUIRE(vd->str[0] == "Hello");
                REQUIRE(vd->str[1] == "World");
                REQUIRE(vd->z == 30.0);
                REQUIRE(vd->str2[0] == "Goodbye");
                REQUIRE(vd->str2[1] == "Cruel World");

                REQUIRE(vd != nullptr);
                REQUIRE(allocated.insert(vd).second);
                REQUIRE(allocator.Belongs(vd));
            }
            REQUIRE(allocator.GetAllocationCount() == amount);
            for (Base *vb : allocated)
            {
                REQUIRE(vb->x == 10);
                REQUIRE(vb->y == 20.0);
                REQUIRE(vb->str[0] == "Hello");
                REQUIRE(vb->str[1] == "World");
                delete vb;
            }

            // Reuse the same chunk over and over again
            for (usize i = 0; i < amount; ++i)
            {
                Derived *vd = new Derived;
                REQUIRE(vd != nullptr);
                REQUIRE(allocator.Belongs(vd));
                delete vd;
            }
            REQUIRE(allocator.IsEmpty());
        }
    }
}

TEST_CASE("Block allocator deals with small data", "[block_allocator][small]")
{
    RunRawAllocationTest<SmallData>();
    RunNewDeleteTest<SmallData>();
}
TEST_CASE("Block allocator deals with big data", "[block_allocator][big]")
{
    RunRawAllocationTest<BigData>();
    RunNewDeleteTest<BigData>();
}
TEST_CASE("Block allocator deals with aligned data", "[block_allocator][aligned]")
{
    RunRawAllocationTest<AlignedData>();
    RunNewDeleteTest<AlignedData>();
}
TEST_CASE("Block allocator deals with non trivial data", "[block_allocator][nont trivial]")
{
    RunRawAllocationTest<NonTrivialData>();
    RunNewDeleteTest<NonTrivialData>();
    REQUIRE(NonTrivialData::Instances == 0);
}
TEST_CASE("Block allocator deals with derived data", "[block_allocator][derived]")
{
    RunRawAllocationTest<VirtualDerived>();
    RunNewDeleteTest<VirtualDerived>();

    REQUIRE(VirtualBase::BaseInstances == 0);
    REQUIRE(VirtualDerived::DerivedInstances == 0);
}
TEST_CASE("Block allocator deals with virtual data", "[block_allocator][virtual]")
{
    RunVirtualAllocatorTests<VirtualBase, VirtualDerived>();

    REQUIRE(VirtualBase::BaseInstances == 0);
    REQUIRE(VirtualDerived::DerivedInstances == 0);
}
} // namespace TKit