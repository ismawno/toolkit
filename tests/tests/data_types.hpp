#pragma once

#include "tkit/core/api.hpp"
#include "tkit/memory/block_allocator.hpp"

namespace TKit
{
struct SmallData
{
    TKIT_BLOCK_ALLOCATED_CONCURRENT(SmallData, 10);
    i32 x;
};

struct BigData
{
    TKIT_BLOCK_ALLOCATED_CONCURRENT(BigData, 10);
    f64 x;
    f64 y;
    f64 z;
    std::string str[3];
};

TKIT_WARNING_IGNORE_PUSH
TKIT_MSVC_WARNING_IGNORE(4324)

struct AlignedData
{
    TKIT_BLOCK_ALLOCATED_CONCURRENT(AlignedData, 10);
    alignas(16) f64 x, y, z;
    alignas(32) f64 a, b, c;
};
TKIT_WARNING_IGNORE_POP

struct NonTrivialData
{
    TKIT_BLOCK_ALLOCATED_CONCURRENT(NonTrivialData, 10);
    i32 *x = nullptr;
    NonTrivialData() : x(new i32[25])
    {
        ++Instances;
    }

    NonTrivialData(const NonTrivialData &other) : x(new i32[25])
    {
        ++Instances;
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
        ++Instances;
        other.x = nullptr;
    }

    NonTrivialData &operator=(NonTrivialData &&other) noexcept
    {
        if (this != &other)
        {
            TKIT_WARNING_IGNORE_PUSH
            TKIT_GCC_WARNING_IGNORE("-Wmaybe-uninitialized")
            if (x)
                delete[] x;
            TKIT_WARNING_IGNORE_POP
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
        --Instances;
    }

    static inline i32 Instances = 0;
};

struct VirtualBase
{
    TKIT_BLOCK_ALLOCATED_CONCURRENT(VirtualBase, 10);
    VirtualBase()
    {
        ++BaseInstances;
    }

    virtual ~VirtualBase()
    {
        --BaseInstances;
    }

    virtual void SetValues()
    {
        x = 10;
        y = 20.0;
        str[0] = "Hello";
        str[1] = "World";
    }

    static inline i32 BaseInstances = 0;

    i32 x;
    f64 y;
    std::string str[2];
};

struct VirtualDerived : VirtualBase
{
    TKIT_BLOCK_ALLOCATED_CONCURRENT(VirtualDerived, 10);
    VirtualDerived()
    {
        ++DerivedInstances;
    }

    ~VirtualDerived() override
    {
        --DerivedInstances;
    }

    void SetValues() override
    {
        VirtualBase::SetValues();
        z = 30.0;
        str2[0] = "Goodbye";
        str2[1] = "Cruel World";
    }

    static inline i32 DerivedInstances = 0;

    f64 z;
    std::string str2[2];
};
} // namespace TKit
