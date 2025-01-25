#include "perf/settings.hpp"
#include <filesystem>
#include <fstream>

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wunused-parameter")
TKIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
TKIT_MSVC_WARNING_IGNORE(4100)
#include <yaml-cpp/yaml.h>
TKIT_COMPILER_WARNING_IGNORE_POP()

namespace TKit
{
Settings ReadOrWriteSettingsFile()
{
    Settings settings;
    if (!std::filesystem::exists(g_Root + "/performance/perf-settings.yaml"))
    {
        YAML::Emitter emitter;
        YAML::Node node;
        node["MT"]["MaxThreads"] = settings.MaxThreads;

        YAML::Node memory = node["Memory"];
        memory["MinPasses"] = settings.Allocation.MinPasses;
        memory["MaxPasses"] = settings.Allocation.MaxPasses;
        memory["PassIncrement"] = settings.Allocation.PassIncrement;

        YAML::Node threadPoolSum = node["ThreadPoolSum"];
        threadPoolSum["SumCount"] = settings.ThreadPoolSum.SumCount;

        YAML::Node container = node["Container"];
        container["MinPasses"] = settings.Container.MinPasses;
        container["MaxPasses"] = settings.Container.MaxPasses;
        container["PassIncrement"] = settings.Container.PassIncrement;

        emitter << node;
        std::ofstream file(g_Root + "/performance/perf-settings.yaml");
        file << emitter.c_str();
    }
    else
    {
        const YAML::Node node = YAML::LoadFile(g_Root + "/performance/perf-settings.yaml");
        settings.MaxThreads = node["MT"]["MaxThreads"].as<usize>();

        const YAML::Node memory = node["Memory"];
        settings.Allocation.MinPasses = memory["MinPasses"].as<usize>();
        settings.Allocation.MaxPasses = memory["MaxPasses"].as<usize>();
        settings.Allocation.PassIncrement = memory["PassIncrement"].as<usize>();

        const YAML::Node threadPoolSum = node["ThreadPoolSum"];
        settings.ThreadPoolSum.SumCount = threadPoolSum["SumCount"].as<usize>();

        const YAML::Node container = node["Container"];
        settings.Container.MinPasses = container["MinPasses"].as<usize>();
        settings.Container.MaxPasses = container["MaxPasses"].as<usize>();
        settings.Container.PassIncrement = container["PassIncrement"].as<usize>();
    }
    std::filesystem::create_directories(g_Root + "/performance/results");
    return settings;
}
} // namespace TKit