#pragma once

#include "kit/core/core.hpp"
#include "kit/memory/block_allocator.hpp"

struct SmallData
{
    KIT_BLOCK_ALLOCATED(SmallData, 10);
    int x;
};

struct BigData
{
    KIT_BLOCK_ALLOCATED(BigData, 10);
    double x;
    double y;
    double z;
};

KIT_WARNING_IGNORE_PUSH
KIT_MSVC_WARNING_IGNORE(4324)
struct AlignedData
{
    KIT_BLOCK_ALLOCATED(AlignedData, 10);
    alignas(16) double x, y, z;
    alignas(32) double a, b, c;
};
KIT_WARNING_IGNORE_POP

struct NonTrivialData
{
    KIT_BLOCK_ALLOCATED(NonTrivialData, 10);
    int *x = nullptr;
    NonTrivialData() : x(new int[25])
    {
        ++Instances;
    }

    NonTrivialData(const NonTrivialData &other) : x(new int[25])
    {
        ++Instances;
        for (int i = 0; i < 25; ++i)
            x[i] = other.x[i];
    }

    NonTrivialData &operator=(const NonTrivialData &other)
    {
        if (this != &other)
        {
            for (int i = 0; i < 25; ++i)
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

    static inline int Instances = 0;
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

    static inline int BaseInstances = 0;

    int x;
    double y;
    std::string str[12];
};

struct VirtualDerived final : VirtualBase
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

    static inline int DerivedInstances = 0;

    double z;
    std::string str2[12];
};

struct BadVirtualDerived : VirtualBase
{
    // Remove the KIT_BLOCK_ALLOCATED macro to trigger the assert in the block allocator
    // KIT_BLOCK_ALLOCATED(BadVirtualDerived);
    BadVirtualDerived()
    {
        ++DerivedInstances;
    }

    ~BadVirtualDerived() override
    {
        --DerivedInstances;
    }

    static inline int DerivedInstances = 0;

    double z;
    std::string str2[12];
};
