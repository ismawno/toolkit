#include "kit/memory/block_allocator.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <thread>
#include <iostream>

KIT_NAMESPACE_BEGIN

template <typename T, template <typename> typename Allocator> static void RunRawAllocationTest()
{
    REQUIRE(sizeof(T) % alignof(T) == 0);
    Allocator<T> allocator(10);

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
        allocator.Destroy(data);
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
    auto &allocator = T::s_Allocator;
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
    auto &allocator = Derived::s_Allocator;
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

template <typename T> static void RunMultithreadedAllocatorTests()
{
    TSafeBlockAllocator<T> &allocator = T::s_Allocator;
    REQUIRE(allocator.Empty());
    allocator.Reset();
    DYNAMIC_SECTION("Multithreaded allocations" << (int)typeid(T).hash_code())
    {
        struct Data
        {
            T *data;
            std::byte padding[64 - sizeof(T *)];
        };
        constexpr usz amount = 1000;
        constexpr usz threadCount = 8;
        static std::mutex mutex;

        std::array<std::array<Data, amount>, threadCount> data;

        const auto allocateBulk = [&data](const usz tindex) {
            const TSafeBlockAllocator<T> &alloc = T::s_Allocator; // msvc yells if i dont do this
            for (usz i = 0; i < amount; ++i)
            {
                T *ptr = new T;
                ptr->ToEdit = tindex * amount + i;

                data[tindex][i].data = ptr;
                const bool owned = alloc.Owns(ptr);

                std::scoped_lock lock(mutex);
                REQUIRE(ptr != nullptr);
                REQUIRE(owned);
            }
        };

        const auto deallocateBulk = [&data](const usz tindex) {
            for (usz i = 0; i < amount; ++i)
            {
                T *ptr = data[tindex][i].data;
                const usz edit = ptr->ToEdit;
                const bool equal = edit == tindex * amount + i;
                delete ptr;
                std::scoped_lock lock(mutex);
                REQUIRE(equal);
            }
        };

        const auto allocateDeallocate = [](const usz tindex) {
            const TSafeBlockAllocator<T> &alloc = T::s_Allocator; // msvc yells if i dont do this
            for (usz i = 0; i < amount; ++i)
            {
                // this scoped code causes a data race :( (benign one if u ask me)
                T *ptr = new T;
                ptr->ToEdit = tindex * amount + i;

                const bool owned = alloc.Owns(ptr);
                const bool notnull = ptr != nullptr;
                const usz edit = ptr->ToEdit;
                const bool equal = edit == tindex * amount + i;
                delete ptr;

                std::scoped_lock lock(mutex);
                REQUIRE(notnull);
                REQUIRE(owned);
                REQUIRE(equal);
            }
        };

        std::array<std::thread, threadCount> threads;
        for (usz i = 0; i < threadCount; ++i)
            threads[i] = std::thread(allocateBulk, i);
        for (usz i = 0; i < threadCount; ++i)
            threads[i].join();

        HashSet<T *> allocated;
        for (usz i = 0; i < threadCount; ++i)
            for (usz j = 0; j < amount; ++j)
            {
                auto it = allocated.insert(data[i][j].data);
                REQUIRE(it.second);
            }

        for (usz i = 0; i < threadCount; ++i)
            threads[i] = std::thread(deallocateBulk, i);
        for (usz i = 0; i < threadCount; ++i)
            threads[i].join();

        for (usz i = 0; i < threadCount; ++i)
            threads[i] = std::thread(allocateDeallocate, i);
        for (usz i = 0; i < threadCount; ++i)
            threads[i].join();
    }

    REQUIRE(allocator.Empty());
}

TEST_CASE("Block allocator deals with small data", "[block_allocator][small]")
{
    RunRawAllocationTest<SmallDataTS, TSafeBlockAllocator>();
    RunRawAllocationTest<SmallDataTU, TUnsafeBlockAllocator>();
    RunNewDeleteTest<SmallDataTS>();
    RunNewDeleteTest<SmallDataTU>();
    RunMultithreadedAllocatorTests<SmallDataTS>();
}
TEST_CASE("Block allocator deals with big data", "[block_allocator][big]")
{
    RunRawAllocationTest<BigDataTS, TSafeBlockAllocator>();
    RunRawAllocationTest<BigDataTU, TUnsafeBlockAllocator>();
    RunNewDeleteTest<BigDataTS>();
    RunNewDeleteTest<BigDataTU>();
    RunMultithreadedAllocatorTests<BigDataTS>();
}
TEST_CASE("Block allocator deals with aligned data", "[block_allocator][aligned]")
{
    RunRawAllocationTest<AlignedDataTS, TSafeBlockAllocator>();
    RunRawAllocationTest<AlignedDataTU, TUnsafeBlockAllocator>();
    RunNewDeleteTest<AlignedDataTS>();
    RunNewDeleteTest<AlignedDataTU>();
    RunMultithreadedAllocatorTests<AlignedDataTS>();
}
TEST_CASE("Block allocator deals with non trivial data", "[block_allocator][nont trivial]")
{
    RunRawAllocationTest<NonTrivialDataTS, TSafeBlockAllocator>();
    RunRawAllocationTest<NonTrivialDataTU, TUnsafeBlockAllocator>();
    RunNewDeleteTest<NonTrivialDataTS>();
    RunNewDeleteTest<NonTrivialDataTU>();
    // This test is disabled because NonTrivialData is not thread safe, even though the name may suggest otherwise
    // RunMultithreadedAllocatorTests<NonTrivialDataTS>();
    REQUIRE(NonTrivialDataTS::Instances == 0);
    REQUIRE(NonTrivialDataTU::Instances == 0);
}
TEST_CASE("Block allocator deals with derived data", "[block_allocator][derived]")
{
    RunRawAllocationTest<VirtualDerivedTS, TSafeBlockAllocator>();
    RunRawAllocationTest<VirtualDerivedTU, TUnsafeBlockAllocator>();
    RunNewDeleteTest<VirtualDerivedTS>();
    RunNewDeleteTest<VirtualDerivedTU>();

    // Construction/destruction of virtual objects is not thread safe in c++!! yay. turns out i didnt know that, so i
    // wasted quite a lot of time on this.
    // RunMultithreadedAllocatorTests<VirtualDerivedTS>();

    REQUIRE(VirtualBaseTS::BaseInstances == 0);
    REQUIRE(VirtualDerivedTS::DerivedInstances == 0);

    REQUIRE(VirtualBaseTU::BaseInstances == 0);
    REQUIRE(VirtualDerivedTU::DerivedInstances == 0);
}
TEST_CASE("Block allocator deals with virtual data", "[block_allocator][virtual]")
{
    RunVirtualAllocatorTests<VirtualBaseTS, VirtualDerivedTS>();
    RunVirtualAllocatorTests<VirtualBaseTU, VirtualDerivedTU>();

    REQUIRE(VirtualBaseTS::BaseInstances == 0);
    REQUIRE(VirtualDerivedTS::DerivedInstances == 0);

    REQUIRE(VirtualBaseTU::BaseInstances == 0);
    REQUIRE(VirtualDerivedTU::DerivedInstances == 0);
}
TEST_CASE("Block allocator deals with invalid virtual data", "[block_allocator][virtual][invalid]")
{
    REQUIRE_THROWS(new BadVirtualDerivedTS);
    REQUIRE_THROWS(new BadVirtualDerivedTU);
}

KIT_NAMESPACE_END