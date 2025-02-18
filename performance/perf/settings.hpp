#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/reflection/reflect.hpp"
#include <string>

namespace TKit
{
inline std::string g_Root = TKIT_ROOT_PATH;

struct AllocationSettings
{
    TKIT_REFLECT(AllocationSettings)
    usize MinPasses = 100;
    usize MaxPasses = 10000;
    usize PassIncrement = 100;
};

using ContainerSettings = AllocationSettings;

struct ThreadPoolSumSettings
{
    TKIT_REFLECT(ThreadPoolSumSettings)
    usize MaxThreads = 8;
    usize SumCount = 1000000;
};

struct Settings
{
    TKIT_REFLECT(Settings)
    AllocationSettings Allocation{};
    ThreadPoolSumSettings ThreadPoolSum{};
    ContainerSettings Container{};
};

Settings ReadOrWriteSettingsFile();
} // namespace TKit