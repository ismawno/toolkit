#include "kit/memory/block_allocator.hpp"
#include "data_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>

template <typename T> void RunBasicAllocatorTest(KIT::BlockAllocator<T> &allocator)
{
    const size_t chunkSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
    const size_t blockSize = chunkSize * 10;

    REQUIRE(allocator.BlockSize() == blockSize);
    REQUIRE(allocator.BlockCount() == 0);
    REQUIRE(allocator.ChunksPerBlock() == 10);
    REQUIRE(allocator.ChunkSize() == chunkSize);
    REQUIRE(allocator.Empty());

    T *null = nullptr;
    REQUIRE_THROWS(allocator.Deallocate(null));
    REQUIRE_THROWS(KIT::Deallocate(null));
}

template <typename T> void RunRawAllocationTest()
{
    REQUIRE(sizeof(T) % alignof(T) == 0);
    KIT::BlockAllocator<T> allocator(10);
    RunBasicAllocatorTest(allocator);

    SECTION("Allocate and deallocate (raw call)")
    {
        T *data = allocator.Allocate();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        allocator.Deallocate(data);
        REQUIRE(allocator.Empty());
    }

    SECTION("Construct and destroy (raw call)")
    {
        T *data = allocator.Construct();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        allocator.Destroy(data);
        REQUIRE(allocator.Empty());
    }

    SECTION("Allocate and deallocate multiple (raw call)")
    {
        constexpr uint32_t amount = 20;
        std::array<T *, amount> data;
        for (uint32_t j = 0; j < 2; ++j)
        {
            for (uint32_t i = 0; i < amount; ++i)
            {
                data[i] = allocator.Allocate();
                REQUIRE(data[i] != nullptr);
                REQUIRE(allocator.Owns(data[i]));
            }
            for (uint32_t i = 0; i < amount; ++i)
                allocator.Deallocate(data[i]);
            REQUIRE(allocator.BlockCount() == 2);
        }
        REQUIRE(allocator.Empty());
    }

    SECTION("Assert contiguous (raw call)")
    {
        constexpr uint32_t amount = 10;
        std::array<T *, amount> data;
        const size_t chunkSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
        for (uint32_t i = 0; i < amount; ++i)
        {
            data[i] = allocator.Allocate();
            REQUIRE(data[i] != nullptr);
            REQUIRE(allocator.Owns(data[i]));
            if (i != 0)
            {
                KIT::Byte *b1 = reinterpret_cast<KIT::Byte *>(data[i - 1]);
                KIT::Byte *b2 = reinterpret_cast<KIT::Byte *>(data[i]);
                REQUIRE(b2 == b1 + chunkSize);
            }
        }
        for (uint32_t i = 0; i < amount; ++i)
            allocator.Deallocate(data[i]);
        REQUIRE(allocator.Empty());
    }
}

template <typename T> void RunNewDeleteTest()
{
    const KIT::BlockAllocator<T> &allocator = T::s_Allocator;

    SECTION("Allocate and deallocate (new/delete)")
    {
        T *data = new T;
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        delete data;
        REQUIRE(allocator.Empty());
    }

    SECTION("Allocate and deallocate multiple (new/delete)")
    {
        constexpr uint32_t amount = 20;
        std::array<T *, amount> data;
        for (uint32_t j = 0; j < 2; ++j)
        {
            for (uint32_t i = 0; i < amount; ++i)
            {
                data[i] = new T;
                REQUIRE(data[i] != nullptr);
                REQUIRE(allocator.Owns(data[i]));
            }
            for (uint32_t i = 0; i < amount; ++i)
                delete data[i];
            REQUIRE(allocator.BlockCount() == 2);
        }
        REQUIRE(allocator.Empty());
    }

    SECTION("Assert contiguous (new/delete)")
    {
        constexpr uint32_t amount = 10;
        std::array<T *, amount> data;
        const size_t chunkSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
        for (uint32_t i = 0; i < amount; ++i)
        {
            data[i] = new T;
            REQUIRE(data[i] != nullptr);
            REQUIRE(allocator.Owns(data[i]));
            if (i != 0)
            {
                KIT::Byte *b1 = reinterpret_cast<KIT::Byte *>(data[i - 1]);
                KIT::Byte *b2 = reinterpret_cast<KIT::Byte *>(data[i]);
                REQUIRE(b2 == b1 + chunkSize);
            }
        }
        for (uint32_t i = 0; i < amount; ++i)
            delete data[i];
        REQUIRE(allocator.Empty());
    }
}

template <typename Base, typename Derived> void RunVirtualAllocatorTests()
{
    const auto &allocator = Derived::s_Allocator;
    REQUIRE(allocator.Empty());

    SECTION("Virtual deallocations")
    {
        constexpr uint32_t amount = 20;
        std::array<Base *, amount> data;
        for (uint32_t j = 0; j < 2; ++j)
        {
            for (uint32_t i = 0; i < amount; ++i)
            {
                Derived *vd = new Derived;
                data[i] = vd;
                REQUIRE(data[i] != nullptr);
                REQUIRE(allocator.Owns(vd));
            }
            REQUIRE(allocator.Allocations() == amount);
            for (uint32_t i = 0; i < amount; ++i)
                delete data[i];
            REQUIRE(allocator.BlockCount() == 2);
            REQUIRE(allocator.Empty());
        }
        REQUIRE(allocator.Empty());
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
TEST_CASE("Block allocator deals with nont trivial data", "[block_allocator][nont trivial]")
{
    RunRawAllocationTest<NonTrivialData>();
    RunNewDeleteTest<NonTrivialData>();
    REQUIRE(NonTrivialData::Instances == 0);
}
TEST_CASE("Block allocator deals with derived data", "[block_allocator][derived]")
{
    RunRawAllocationTest<VirtualDerived>();
    RunNewDeleteTest<VirtualDerived>();
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