#include "kit/memory/block_allocator.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>

KIT_NAMESPACE_BEGIN

template <typename T> static void RunRawAllocationTest()
{
    REQUIRE(sizeof(T) % alignof(T) == 0);
    BlockAllocator<T> allocator(10);
    REQUIRE(allocator.Empty());
    REQUIRE(allocator.BlockCount() == 0);

    DYNAMIC_SECTION("Allocate and deallocate (raw call)" << (int)typeid(T).hash_code())
    {
        T *data = allocator.Allocate();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        allocator.Deallocate(data);
        REQUIRE(allocator.Empty());
    }

    DYNAMIC_SECTION("Construct and destroy (raw call)" << (int)typeid(T).hash_code())
    {
        T *data = allocator.Construct();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        allocator.Destruct(data);
        REQUIRE(allocator.Empty());
    }

    DYNAMIC_SECTION("Allocate and deallocate multiple (raw call)" << (int)typeid(T).hash_code())
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
                REQUIRE(allocator.Owns(ptr));
            }
            REQUIRE(allocator.Allocations() == amount);

            for (T *ptr : allocated)
                allocator.Deallocate(ptr);

            // Reuse the same chunk over and over again
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = allocator.Allocate();
                REQUIRE(ptr != nullptr);
                REQUIRE(allocator.Owns(ptr));
                allocator.Deallocate(ptr);
            }
            REQUIRE(allocator.BlockCount() == amount / 10);
        }
        REQUIRE(allocator.Empty());
    }

    DYNAMIC_SECTION("Assert contiguous (raw call)" << (int)typeid(T).hash_code())
    {
        constexpr u32 amount = 10;
        std::array<T *, amount> data;
        const usz chunkSize = allocator.ChunkSize();
        for (u32 i = 0; i < amount; ++i)
        {
            data[i] = allocator.Allocate();
            REQUIRE(data[i] != nullptr);
            REQUIRE(allocator.Owns(data[i]));
            if (i != 0)
            {
                std::byte *b1 = reinterpret_cast<std::byte *>(data[i - 1]);
                std::byte *b2 = reinterpret_cast<std::byte *>(data[i]);
                REQUIRE(b2 == b1 + chunkSize);
            }
        }
        for (u32 i = 0; i < amount; ++i)
            allocator.Deallocate(data[i]);
        REQUIRE(allocator.Empty());
    }
}

template <typename T> static void RunNewDeleteTest()
{
    BlockAllocator<T> &allocator = T::s_Allocator;
    REQUIRE(allocator.Empty());
    allocator.Reset();

    DYNAMIC_SECTION("Allocate and deallocate (new/delete)" << (int)typeid(T).hash_code())
    {
        T *data = new T;
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        delete data;
        REQUIRE(allocator.Empty());
    }

    DYNAMIC_SECTION("Allocate and deallocate multiple (new/delete)" << (int)typeid(T).hash_code())
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
                REQUIRE(allocator.Owns(ptr));
            }
            REQUIRE(allocator.Allocations() == amount);
            for (T *ptr : allocated)
                delete ptr;

            // Reuse the same chunk over and over again
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = new T;
                REQUIRE(ptr != nullptr);
                REQUIRE(allocator.Owns(ptr));
                delete ptr;
            }

            REQUIRE(allocator.BlockCount() == amount / 10);
        }
        REQUIRE(allocator.Empty());
    }

    DYNAMIC_SECTION("Assert contiguous (new/delete)" << (int)typeid(T).hash_code())
    {
        constexpr u32 amount = 10;
        std::array<T *, amount> data;
        const usz chunkSize = allocator.ChunkSize();
        for (u32 i = 0; i < amount; ++i)
        {
            data[i] = new T;
            REQUIRE(data[i] != nullptr);
            REQUIRE(allocator.Owns(data[i]));
            if (i != 0)
            {
                std::byte *b1 = reinterpret_cast<std::byte *>(data[i - 1]);
                std::byte *b2 = reinterpret_cast<std::byte *>(data[i]);
                REQUIRE(b2 == b1 + chunkSize);
            }
        }
        for (u32 i = 0; i < amount; ++i)
            delete data[i];
        REQUIRE(allocator.Empty());
    }
}

template <typename Base, typename Derived> void RunVirtualAllocatorTests()
{
    BlockAllocator<Derived> &allocator = Derived::s_Allocator;
    REQUIRE(allocator.Empty());
    allocator.Reset();

    DYNAMIC_SECTION("Virtual deallocations" << (int)typeid(Derived).hash_code())
    {
        constexpr usz amount = 1000;
        for (usz j = 0; j < 2; ++j)
        {
            HashSet<Base *> allocated;
            for (usz i = 0; i < amount; ++i)
            {
                Derived *vd = new Derived;
                REQUIRE(vd != nullptr);
                REQUIRE(allocated.insert(vd).second);
                REQUIRE(allocator.Owns(vd));
            }
            REQUIRE(allocator.Allocations() == amount);
            for (Base *vb : allocated)
                delete vb;

            // Reuse the same chunk over and over again
            for (usz i = 0; i < amount; ++i)
            {
                Derived *vd = new Derived;
                REQUIRE(vd != nullptr);
                REQUIRE(allocator.Owns(vd));
                delete vd;
            }
            REQUIRE(allocator.Empty());
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
TEST_CASE("Block allocator deals with invalid virtual data", "[block_allocator][virtual][invalid]")
{
    REQUIRE_THROWS(new BadVirtualDerived);
}

KIT_NAMESPACE_END