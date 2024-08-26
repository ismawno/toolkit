#pragma once

#include "kit/core/alias.hpp"
#include <string>

namespace KIT
{
inline std::string g_Root = KIT_ROOT_PATH;

struct AllocationSettings
{
    usize MinPasses = 100;
    usize MaxPasses = 10000;
    usize PassIncrement = 100;
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
} // namespace KIT