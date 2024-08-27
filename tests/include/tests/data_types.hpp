#pragma once

#include "kit/core/api.hpp"
#include "kit/memory/block_allocator.hpp"

namespace KIT
{
struct SmallData
{
    KIT_BLOCK_ALLOCATED(SmallData, 10);
    i32 x;
};

struct BigData
{
    KIT_BLOCK_ALLOCATED(BigData, 10);
    f64 x;
    f64 y;
    f64 z;
    std::string str[3];
};

KIT_WARNING_IGNORE_PUSH
KIT_MSVC_WARNING_IGNORE(4324)

struct AlignedData
{
    KIT_BLOCK_ALLOCATED(AlignedData, 10);
    alignas(16) f64 x, y, z;
    alignas(32) f64 a, b, c;
};
KIT_WARNING_IGNORE_POP

struct NonTrivialData
{
    KIT_BLOCK_ALLOCATED(NonTrivialData, 10);
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
        --Instances;
    }

    static inline i32 Instances = 0;
};

struct VirtualBase
{
    KIT_BLOCK_ALLOCATED(VirtualBase, 10);
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
    KIT_BLOCK_ALLOCATED(VirtualDerived, 10);
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
} // namespace KIT
