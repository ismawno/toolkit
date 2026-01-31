#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/serialization/yaml/serialize.hpp"
#include "tkit/reflection/reflect.hpp"
#include <string>

namespace TKit
{
inline std::string g_Root = TKIT_ROOT_PATH;

struct AllocationSettings
{
    TKIT_YAML_SERIALIZE_DECLARE(AllocationSettings)
    TKIT_REFLECT_DECLARE(AllocationSettings)
    usize MinPasses = 100;
    usize MaxPasses = 10000;
    usize PassIncrement = 100;
};

using ContainerSettings = AllocationSettings;

struct ThreadPoolSettings
{
    TKIT_YAML_SERIALIZE_DECLARE(ThreadPoolSettings)
    TKIT_REFLECT_DECLARE(ThreadPoolSettings)
    usize MaxThreads = 8;
    usize SumCount = 1000000;
};

struct Settings
{
    TKIT_YAML_SERIALIZE_DECLARE(Settings)
    TKIT_REFLECT_DECLARE(Settings)
    AllocationSettings Allocation{};
    ThreadPoolSettings ThreadPoolSum{};
    ContainerSettings Container{};
};

#ifdef TKIT_ENABLE_INFO_LOGS
void LogSettings(const Settings &settings);
#endif
Settings CreateSettings(int argc, char **argv);
} // namespace TKit
