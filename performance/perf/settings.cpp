#include "perf/settings.hpp"
#include "tkit/reflection/perf/settings.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include <filesystem>
#include <fstream>

namespace TKit::Perf
{
Settings ReadOrWriteSettingsFile()
{
    Settings settings;
    if (!std::filesystem::exists(g_Root + "/performance/perf-settings.yaml"))
    {
        Yaml::Node node{settings};
        std::ofstream file(g_Root + "/performance/perf-settings.yaml");
        file << node;
    }
    else
    {
        const Yaml::Node node = Yaml::LoadFromFile(g_Root + "/performance/perf-settings.yaml");
        settings = node.as<Settings>();
    }
    std::filesystem::create_directories(g_Root + "/performance/results");
    return settings;
}
} // namespace TKit::Perf
