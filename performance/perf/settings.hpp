#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/serialization/yaml/serialize.hpp"
#include <string>

namespace TKit::Perf
{
inline std::string g_Root = TKIT_ROOT_PATH;

struct AllocationSettings
{
    TKIT_SERIALIZE_DECLARE(AllocationSettings)
    usize MinPasses = 100;
    usize MaxPasses = 10000;
    usize PassIncrement = 100;
};

using ContainerSettings = AllocationSettings;

struct ThreadPoolSumSettings
{
    TKIT_SERIALIZE_DECLARE(ThreadPoolSumSettings)
    usize MaxThreads = 8;
    usize SumCount = 1000000;
};

struct Settings
{
    TKIT_SERIALIZE_DECLARE(Settings)
    AllocationSettings Allocation{};
    ThreadPoolSumSettings ThreadPoolSum{};
    ContainerSettings Container{};
};

Settings ReadOrWriteSettingsFile();
} // namespace TKit::Perf
