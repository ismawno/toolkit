#include "kit/memory/block_allocator.hpp"
#include "kit/multiprocessing/thread_pool.hpp"
#include "kit/multiprocessing/for_each.hpp"
#include "tests/data_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>

namespace KIT
{
template <typename T> static void RunRawAllocationTest()
{
    REQUIRE(sizeof(T) % alignof(T) == 0);
    BlockAllocator<T> allocator(10);
    REQUIRE(allocator.IsEmpty());
    REQUIRE(allocator.GetBlockCount() == 0);

    DYNAMIC_SECTION("Allocate and deallocate (raw call)" << (int)typeid(T).hash_code())
    {
        T *data = allocator.AllocateSerial();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        allocator.DeallocateSerial(data);
        REQUIRE(allocator.IsEmpty());
    }

    DYNAMIC_SECTION("Create and destroy (raw call)" << (int)typeid(T).hash_code())
    {
        T *data = allocator.CreateSerial();
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        allocator.DestroySerial(data);
        REQUIRE(allocator.IsEmpty());
    }

    DYNAMIC_SECTION("Allocate and deallocate multiple (raw call)" << (int)typeid(T).hash_code())
    {
        constexpr u32 amount = 1000;
        for (u32 j = 0; j < 2; ++j)
        {
            HashSet<T *> allocated;
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = allocator.AllocateSerial();
                REQUIRE(ptr != nullptr);
                REQUIRE(allocated.insert(ptr).second);
                REQUIRE(allocator.Owns(ptr));
            }
            REQUIRE(allocator.GetAllocations() == amount);

            for (T *ptr : allocated)
                allocator.DeallocateSerial(ptr);

            // Reuse the same chunk over and over again
            for (u32 i = 0; i < amount; ++i)
            {
                T *ptr = allocator.AllocateSerial();
                REQUIRE(ptr != nullptr);
                REQUIRE(allocator.Owns(ptr));
                allocator.DeallocateSerial(ptr);
            }
            REQUIRE(allocator.GetBlockCount() == amount / 10);
        }
        REQUIRE(allocator.IsEmpty());
    }

    DYNAMIC_SECTION("Assert contiguous (raw call)" << (int)typeid(T).hash_code())
    {
        constexpr u32 amount = 10;
        std::array<T *, amount> data;
        const usize chunkSize = allocator.GetChunkSize();
        for (u32 i = 0; i < amount; ++i)
        {
            data[i] = allocator.AllocateSerial();
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
            allocator.DeallocateSerial(data[i]);
        REQUIRE(allocator.IsEmpty());
    }
}

template <typename T> static void RunNewDeleteTest()
{
    BlockAllocator<T> &allocator = GlobalBlockAllocatorInstance<T, 10>();
    REQUIRE(allocator.IsEmpty());
    allocator.Reset();

    DYNAMIC_SECTION("Allocate and deallocate (new/delete)" << (int)typeid(T).hash_code())
    {
        T *data = new T;
        REQUIRE(data != nullptr);
        REQUIRE(allocator.Owns(data));
        delete data;
        REQUIRE(allocator.IsEmpty());
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
            REQUIRE(allocator.GetAllocations() == amount);
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

            REQUIRE(allocator.GetBlockCount() == amount / 10);
        }
        REQUIRE(allocator.IsEmpty());
    }

    DYNAMIC_SECTION("Assert contiguous (new/delete)" << (int)typeid(T).hash_code())
    {
        constexpr u32 amount = 10;
        std::array<T *, amount> data;
        constexpr usize chunkSize = BlockAllocator<T>::GetChunkSize();
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
        REQUIRE(allocator.IsEmpty());
    }
}

template <typename T> static void RunMultithreadedAllocationsTest()
{
    struct Data
    {
        KIT_BLOCK_ALLOCATED_CONCURRENT(Data, 125);
        T Custom;
        u32 Value1 = 0;
        u32 Value2 = 0;
        u64 Result = 0;
    };
    struct PaddedData
    {
        Data *Ptr;
        std::byte Padding[KIT_CACHE_LINE_SIZE - sizeof(T *)];
    };
    constexpr usize amount = 1000;
    constexpr usize threadCount = 8;
    ThreadPool<std::mutex> pool(threadCount);
    std::array<PaddedData, amount> data;
    std::array<Ref<Task<bool>>, threadCount> tasks;

    ForEach(pool, data.begin(), data.end(), tasks.begin(), threadCount,
            [](auto p_It1, auto p_It2, const usize p_ThreadIndex) {
                bool valid = true;
                for (auto it = p_It1; it != p_It2; ++it)
                {
                    it->Ptr = new Data;
                    it->Ptr->Value1 = static_cast<u32>(p_ThreadIndex);
                    it->Ptr->Value2 = static_cast<u32>(p_ThreadIndex) * 10;
                    it->Ptr->Result = it->Ptr->Value1 + it->Ptr->Value2;
                }

                for (auto it = p_It1; it != p_It2; ++it)
                {
                    valid = valid && (it->Ptr->Value1 == p_ThreadIndex);
                    valid = valid && (it->Ptr->Value2 == p_ThreadIndex * 10);
                    valid = valid && (it->Ptr->Result == it->Ptr->Value1 + it->Ptr->Value2);
                    delete it->Ptr;
                }
                return valid;
            });
    for (auto &task : tasks)
        REQUIRE(task->WaitForResult());
}

template <typename Base, typename Derived> void RunVirtualAllocatorTests()
{
    BlockAllocator<Derived> &allocator = GlobalBlockAllocatorInstance<Derived, 10>();
    REQUIRE(allocator.IsEmpty());
    allocator.Reset();

    DYNAMIC_SECTION("Virtual deallocations" << (int)typeid(Derived).hash_code())
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
                REQUIRE(allocator.Owns(vd));
            }
            REQUIRE(allocator.GetAllocations() == amount);
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
                REQUIRE(allocator.Owns(vd));
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
    RunMultithreadedAllocationsTest<SmallData>();
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
} // namespace KIT