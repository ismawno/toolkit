#pragma once

#include "kit/core/core.hpp"
#include "kit/memory/block_allocator.hpp"

KIT_NAMESPACE_BEGIN

struct SmallDataTS
{
    KIT_BLOCK_ALLOCATED(TSafeBlockAllocator, SmallDataTS, 10);
    i32 x;
};

struct SmallDataTU
{
    KIT_BLOCK_ALLOCATED(TUnsafeBlockAllocator, SmallDataTU, 10);
    i32 x;
};

struct BigDataTS
{
    KIT_BLOCK_ALLOCATED(TSafeBlockAllocator, BigDataTS, 10);
    f64 x;
    f64 y;
    f64 z;
};

struct BigDataTU
{
    KIT_BLOCK_ALLOCATED(TUnsafeBlockAllocator, BigDataTU, 10);
    f64 x;
    f64 y;
    f64 z;
};

KIT_WARNING_IGNORE_PUSH
KIT_MSVC_WARNING_IGNORE(4324)

struct AlignedDataTS
{
    KIT_BLOCK_ALLOCATED(TSafeBlockAllocator, AlignedDataTS, 10);
    alignas(16) f64 x, y, z;
    alignas(32) f64 a, b, c;
};

struct AlignedDataTU
{
    KIT_BLOCK_ALLOCATED(TUnsafeBlockAllocator, AlignedDataTU, 10);
    alignas(16) f64 x, y, z;
    alignas(32) f64 a, b, c;
};
KIT_WARNING_IGNORE_POP

struct NonTrivialData
{
    i32 *x = nullptr;
    NonTrivialData() : x(new i32[25])
    {
        Instances.fetch_add(1, std::memory_order_relaxed);
    }

    NonTrivialData(const NonTrivialData &other) : x(new i32[25])
    {
        Instances.fetch_add(1, std::memory_order_relaxed);
        for (i32 i = 0; i < 25; ++i)
            x[i] = other.x[i];
    }

    NonTrivialData &operator=(const NonTrivialData &other)
    {
        if (this != &other)
        {
            for (i32 i = 0; i < 25; ++i)
                x[i] = other.x[i];
        }
        return *this;
    }

    NonTrivialData(NonTrivialData &&other) noexcept : x(other.x)
    {
        Instances.fetch_add(1, std::memory_order_relaxed);
        other.x = nullptr;
    }

    NonTrivialData &operator=(NonTrivialData &&other) noexcept
    {
        if (this != &other)
        {
            KIT_WARNING_IGNORE_PUSH
            KIT_GCC_WARNING_IGNORE("-Wmaybe-uninitialized")
            if (x)
                delete[] x;
            KIT_WARNING_IGNORE_POP
            x = other.x;
            other.x = nullptr;
        }
        return *this;
    }

    ~NonTrivialData()
    {
        if (x)
            delete[] x;
        x = nullptr;
        Instances.fetch_sub(1, std::memory_order_relaxed);
    }

    static inline std::atomic<i32> Instances = 0;
};

struct NonTrivialDataTS : NonTrivialData
{
    KIT_BLOCK_ALLOCATED(TSafeBlockAllocator, NonTrivialDataTS, 10);
};

struct NonTrivialDataTU : NonTrivialData
{
    KIT_BLOCK_ALLOCATED(TUnsafeBlockAllocator, NonTrivialDataTU, 10);
};

// The following is quite annoying because of the two block allocator variants
struct VirtualBaseTS
{
    KIT_BLOCK_ALLOCATED(TSafeBlockAllocator, VirtualBaseTS, 10);
    VirtualBaseTS()
    {
        BaseInstances.fetch_add(1, std::memory_order_relaxed);
    }

    virtual ~VirtualBaseTS()
    {
        BaseInstances.fetch_sub(1, std::memory_order_relaxed);
    }

    static inline std::atomic<i32> BaseInstances = 0;

    i32 x;
    f64 y;
    std::string str[12];
};

struct VirtualBaseTU
{
    KIT_BLOCK_ALLOCATED(TUnsafeBlockAllocator, VirtualBaseTU, 10);
    VirtualBaseTU()
    {
        ++BaseInstances;
    }

    virtual ~VirtualBaseTU()
    {
        --BaseInstances;
    }

    static inline i32 BaseInstances = 0;

    i32 x;
    f64 y;
    std::string str[12];
};

struct VirtualDerivedTS : VirtualBaseTS
{
    KIT_BLOCK_ALLOCATED(TSafeBlockAllocator, VirtualDerivedTS, 10);
    VirtualDerivedTS()
    {
        DerivedInstances.fetch_add(1, std::memory_order_relaxed);
    }

    ~VirtualDerivedTS() override
    {
        DerivedInstances.fetch_sub(1, std::memory_order_relaxed);
    }

    static inline std::atomic<i32> DerivedInstances = 0;

    f64 z;
    std::string str2[12];
};

struct VirtualDerivedTU : VirtualBaseTU
{
    KIT_BLOCK_ALLOCATED(TUnsafeBlockAllocator, VirtualDerivedTU, 10);
    VirtualDerivedTU()
    {
        ++DerivedInstances;
    }

    ~VirtualDerivedTU() override
    {
        --DerivedInstances;
    }

    static inline i32 DerivedInstances = 0;

    f64 z;
    std::string str2[12];
};

struct BadVirtualDerivedTS : VirtualBaseTS
{
    // Remove the KIT_BLOCK_ALLOCATED macro to trigger the assert in the block allocator
    // KIT_BLOCK_ALLOCATED(TSafeBlockAllocator, BadVirtualDerivedTS, 10);
    BadVirtualDerivedTS()
    {
        DerivedInstances.fetch_add(1, std::memory_order_relaxed);
    }

    ~BadVirtualDerivedTS() override
    {
        DerivedInstances.fetch_sub(1, std::memory_order_relaxed);
    }

    static inline std::atomic<i32> DerivedInstances = 0;

    f64 z;
    std::string str2[12];
};

struct BadVirtualDerivedTU : VirtualBaseTU
{
    // Remove the KIT_BLOCK_ALLOCATED macro to trigger the assert in the block allocator
    // KIT_BLOCK_ALLOCATED(TUnsafeBlockAllocator, BadVirtualDerivedTU, 10);
    BadVirtualDerivedTU()
    {
        ++DerivedInstances;
    }

    ~BadVirtualDerivedTU() override
    {
        --DerivedInstances;
    }

    static inline i32 DerivedInstances = 0;

    f64 z;
    std::string str2[12];
};

KIT_NAMESPACE_END
