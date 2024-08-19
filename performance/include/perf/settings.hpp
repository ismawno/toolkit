#pragma once

#include "kit/core/alias.hpp"
#include <string>

KIT_NAMESPACE_BEGIN

inline std::string g_Root = KIT_ROOT_PATH;

struct AllocationSettings
{
    usz MinPasses = 10;
    usz MaxPasses = 10000;
    usz PassIncrement = 1;
};

struct ExampleData
{
    f64 Values[16];
    void SetValues()
    {
        for (usz i = 0; i < 16; ++i)
            Values[i] = static_cast<f64>(i);
    }
};

struct PaddedData
{
    ExampleData *Data;
    std::byte Padding[KIT_CACHE_LINE_SIZE - sizeof(ExampleData *)];
};

KIT_NAMESPACE_END