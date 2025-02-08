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

        YAML::Node memory = node["Memory"];
        settings.Allocation.ForEachFieldIntegers(
            [&memory](const char *p_Name, const usize p_Field) { memory[p_Name] = p_Field; });

        YAML::Node threadPoolSum = node["ThreadPoolSum"];
        settings.ThreadPoolSum.ForEachFieldIntegers(
            [&threadPoolSum](const char *p_Name, const usize p_Field) { threadPoolSum[p_Name] = p_Field; });

        YAML::Node container = node["Container"];
        settings.Container.ForEachFieldIntegers(
            [&container](const char *p_Name, const usize p_Field) { container[p_Name] = p_Field; });

        emitter << node;
        std::ofstream file(g_Root + "/performance/perf-settings.yaml");
        file << emitter.c_str();
    }
    else
    {
        const YAML::Node node = YAML::LoadFile(g_Root + "/performance/perf-settings.yaml");

        const YAML::Node memory = node["Memory"];
        settings.Allocation.ForEachFieldIntegers(
            [&memory](const char *p_Name, usize &p_Field) { p_Field = memory[p_Name].as<usize>(); });

        const YAML::Node threadPoolSum = node["ThreadPoolSum"];
        settings.ThreadPoolSum.ForEachFieldIntegers(
            [&threadPoolSum](const char *p_Name, usize &p_Field) { p_Field = threadPoolSum[p_Name].as<usize>(); });

        const YAML::Node container = node["Container"];
        settings.Container.ForEachFieldIntegers(
            [&container](const char *p_Name, usize &p_Field) { p_Field = container[p_Name].as<usize>(); });
    }
    std::filesystem::create_directories(g_Root + "/performance/results");
    return settings;
}
} // namespace TKit