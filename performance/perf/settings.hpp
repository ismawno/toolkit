#pragma once

#include "tkit/core/alias.hpp"
#include <string>

namespace TKit
{
inline std::string g_Root = TKIT_ROOT_PATH;

struct AllocationSettings
{
    usize MinPasses = 100;
    usize MaxPasses = 10000;
    usize PassIncrement = 100;
};

using ContainerSettings = AllocationSettings;

struct ThreadPoolSumSettings
{
    usize SumCount = 1000000000;
};

struct Settings
{
    usize MaxThreads = 8;
    AllocationSettings Allocation{};
    ThreadPoolSumSettings ThreadPoolSum{};
    ContainerSettings Container{};
};

Settings ReadOrWriteSettingsFile();
} // namespace TKit