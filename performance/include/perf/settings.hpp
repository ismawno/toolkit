#pragma once

#include "kit/core/alias.hpp"
#include <string>

KIT_NAMESPACE_BEGIN

inline std::string g_Root = KIT_ROOT_PATH;

struct AllocationSettings
{
    usize MinPasses = 10;
    usize MaxPasses = 10000;
    usize PassIncrement = 1;
};

struct ThreadPoolSumSettings
{
    usize SumCount = 1000000000;
};

struct Settings
{
    usize MaxThreads = 8;
    AllocationSettings Allocation{};
    ThreadPoolSumSettings ThreadPoolSum{};
};

Settings ReadOrWriteSettingsFile();

KIT_NAMESPACE_END