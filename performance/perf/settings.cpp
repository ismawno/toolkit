#include "perf/settings.hpp"
#include "tkit/serialization/yaml.hpp"
#include <filesystem>
#include <fstream>

namespace TKit
{
Settings ReadOrWriteSettingsFile()
{
    Settings settings;
    if (!std::filesystem::exists(g_Root + "/performance/perf-settings.yaml"))
    {
        Yaml::Node node;
        Yaml::Node memory = node["Memory"];
        settings.Allocation.ForEachFieldIntegers(
            [&memory](const char *p_Name, const usize p_Field) { memory[p_Name] = p_Field; });

        Yaml::Node threadPoolSum = node["ThreadPoolSum"];
        settings.ThreadPoolSum.ForEachFieldIntegers(
            [&threadPoolSum](const char *p_Name, const usize p_Field) { threadPoolSum[p_Name] = p_Field; });

        Yaml::Node container = node["Container"];
        settings.Container.ForEachFieldIntegers(
            [&container](const char *p_Name, const usize p_Field) { container[p_Name] = p_Field; });

        std::ofstream file(g_Root + "/performance/perf-settings.yaml");
        file << node;
    }
    else
    {
        const Yaml::Node node = Yaml::LoadFromFile(g_Root + "/performance/perf-settings.yaml");

        const Yaml::Node memory = node["Memory"];
        settings.Allocation.ForEachFieldIntegers(
            [&memory](const char *p_Name, usize &p_Field) { p_Field = memory[p_Name].as<usize>(); });

        const Yaml::Node threadPoolSum = node["ThreadPoolSum"];
        settings.ThreadPoolSum.ForEachFieldIntegers(
            [&threadPoolSum](const char *p_Name, usize &p_Field) { p_Field = threadPoolSum[p_Name].as<usize>(); });

        const Yaml::Node container = node["Container"];
        settings.Container.ForEachFieldIntegers(
            [&container](const char *p_Name, usize &p_Field) { p_Field = container[p_Name].as<usize>(); });
    }
    std::filesystem::create_directories(g_Root + "/performance/results");
    return settings;
}
} // namespace TKit