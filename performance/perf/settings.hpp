#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/preprocessor/reflection.hpp"
#include <string>

namespace TKit
{
inline std::string g_Root = TKIT_ROOT_PATH;

struct AllocationSettings
{
    usize MinPasses = 100;
    usize MaxPasses = 10000;
    usize PassIncrement = 100;

    TKIT_ENUMERATE_FIELDS(AllocationSettings, MinPasses, MaxPasses, PassIncrement)
};

using ContainerSettings = AllocationSettings;

struct ThreadPoolSumSettings
{
    usize MaxThreads = 8;
    usize SumCount = 1000000;

    TKIT_ENUMERATE_FIELDS(ThreadPoolSumSettings, MaxThreads, SumCount)
};

struct Settings
{
    AllocationSettings Allocation{};
    ThreadPoolSumSettings ThreadPoolSum{};
    ContainerSettings Container{};
};

Settings ReadOrWriteSettingsFile();
} // namespace TKit