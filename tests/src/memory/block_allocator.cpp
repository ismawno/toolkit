#include "kit/memory/block_allocator.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <thread>

KIT_NAMESPACE_BEGIN

template <typename T> void RunBasicAllocatorTest(BlockAllocator<T> &allocator)
{
    const usz chunkSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
    const usz blockSize = chunkSize * 10;

    REQUIRE(allocator.BlockSize() == blockSize);
    REQUIRE(allocator.BlockCount() == 0);
    REQUIRE(allocator.ChunksPerBlock() == 10);
    REQUIRE(allocator.ChunkSize() == chunkSize);
    REQUIRE(allocator.Empty());

    T *null = nullptr;
    REQUIRE_THROWS(allocator.Deallocate(null));
    REQUIRE_THROWS(Deallocate(null));
}

template <typename T> void RunRawAllocationTest()
{
    REQUIRE(sizeof(T) % alignof(T) == 0);
    BlockAllocator<T> allocator(10);
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
        constexpr u32 amount = 10000;
        std::array<T *, amount> data;
        for (u32 j = 0; j < 2; ++j)
        {
            for (u32 i = 0; i < amount; ++i)
            {
                data[i] = allocator.Allocate();
                REQUIRE(data[i] != nullptr);
                REQUIRE(allocator.Owns(data[i]));
            }
            for (u32 i = 0; i < amount; ++i)
                allocator.Deallocate(data[i]);
            REQUIRE(allocator.BlockCount() == 1000);
        }
        REQUIRE(allocator.Empty());
    }

    SECTION("Assert contiguous (raw call)")
    {
        constexpr u32 amount = 10;
        std::array<T *, amount> data;
        const usz chunkSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
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

template <typename T> void RunNewDeleteTest()
{
    const BlockAllocator<T> &allocator = T::s_Allocator;

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
        constexpr u32 amount = 20;
        std::array<T *, amount> data;
        for (u32 j = 0; j < 2; ++j)
        {
            for (u32 i = 0; i < amount; ++i)
            {
                data[i] = new T;
                REQUIRE(data[i] != nullptr);
                REQUIRE(allocator.Owns(data[i]));
            }
            for (u32 i = 0; i < amount; ++i)
                delete data[i];
            REQUIRE(allocator.BlockCount() == 2);
        }
        REQUIRE(allocator.Empty());
    }

    SECTION("Assert contiguous (new/delete)")
    {
        constexpr u32 amount = 10;
        std::array<T *, amount> data;
        const usz chunkSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
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
    const auto &allocator = Derived::s_Allocator;
    REQUIRE(allocator.Empty());

    SECTION("Virtual deallocations")
    {
        constexpr u32 amount = 20;
        std::array<Base *, amount> data;
        for (u32 j = 0; j < 2; ++j)
        {
            for (u32 i = 0; i < amount; ++i)
            {
                Derived *vd = new Derived;
                data[i] = vd;
                REQUIRE(data[i] != nullptr);
                REQUIRE(allocator.Owns(vd));
            }
            REQUIRE(allocator.Allocations() == amount);
            for (u32 i = 0; i < amount; ++i)
                delete data[i];
            REQUIRE(allocator.BlockCount() == 2);
            REQUIRE(allocator.Empty());
        }
        REQUIRE(allocator.Empty());
    }
}

template <typename T> void RunMultithreadedAllocatorTests()
{
    SECTION("Multithreaded allocations")
    {
        const auto allocate = []() {
            constexpr usz amount = 1000;
            const BlockAllocator<T> &allocator = T::s_Allocator;
            thread_local std::array<T *, amount> data{};
            for (usz i = 0; i < amount; ++i)
            {
                data[i] = new T;
                REQUIRE(data[i] != nullptr);
                REQUIRE(allocator.Owns(data[i]));
            }
            for (usz i = 0; i < amount; ++i)
                delete data[i];
        };

        constexpr usz threadCount = 8;
        std::array<std::thread, threadCount> threads;
        for (std::thread &t : threads)
            t = std::thread(allocate);
        for (std::thread &t : threads)
            t.join();
    }
}

TEST_CASE("Block allocator deals with small data", "[block_allocator][small]")
{
    RunRawAllocationTest<SmallData>();
    RunNewDeleteTest<SmallData>();
    RunMultithreadedAllocatorTests<SmallData>();
}
TEST_CASE("Block allocator deals with big data", "[block_allocator][big]")
{
    RunRawAllocationTest<BigData>();
    RunNewDeleteTest<BigData>();
    RunMultithreadedAllocatorTests<BigData>();
}
TEST_CASE("Block allocator deals with aligned data", "[block_allocator][aligned]")
{
    RunRawAllocationTest<AlignedData>();
    RunNewDeleteTest<AlignedData>();
    RunMultithreadedAllocatorTests<AlignedData>();
}
TEST_CASE("Block allocator deals with non trivial data", "[block_allocator][nont trivial]")
{
    RunRawAllocationTest<NonTrivialData>();
    RunNewDeleteTest<NonTrivialData>();
    // I skip non trivial data as its constructor and destructor are not thread safe
    // RunMultithreadedAllocatorTests<NonTrivialData>();
    REQUIRE(NonTrivialData::Instances == 0);
}
TEST_CASE("Block allocator deals with derived data", "[block_allocator][derived]")
{
    RunRawAllocationTest<VirtualDerived>();
    RunNewDeleteTest<VirtualDerived>();
    // I skip non virtual derived as its constructor and destructor are not thread safe
    // RunMultithreadedAllocatorTests<VirtualDerived>();
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