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
        ForEachField<AllocationSettings>([&memory, &settings](const char *p_Name, const auto p_Field) {
            memory[p_Name] = settings.Allocation.*p_Field;
        });

        YAML::Node threadPoolSum = node["ThreadPoolSum"];
        ForEachField<ThreadPoolSumSettings>([&threadPoolSum, &settings](const char *p_Name, const auto p_Field) {
            threadPoolSum[p_Name] = settings.ThreadPoolSum.*p_Field;
        });

        YAML::Node container = node["Container"];
        ForEachField<ContainerSettings>([&container, &settings](const char *p_Name, const auto p_Field) {
            container[p_Name] = settings.Container.*p_Field;
        });

        emitter << node;
        std::ofstream file(g_Root + "/performance/perf-settings.yaml");
        file << emitter.c_str();
    }
    else
    {
        const YAML::Node node = YAML::LoadFile(g_Root + "/performance/perf-settings.yaml");

        const YAML::Node memory = node["Memory"];
        ForEachField<AllocationSettings>([&memory, &settings](const char *p_Name, auto p_Field) {
            using FT = FieldType<decltype(p_Field)>;
            settings.Allocation.*p_Field = memory[p_Name].as<FT>();
        });

        const YAML::Node threadPoolSum = node["ThreadPoolSum"];
        ForEachField<ThreadPoolSumSettings>([&threadPoolSum, &settings](const char *p_Name, auto p_Field) {
            using FT = FieldType<decltype(p_Field)>;
            settings.ThreadPoolSum.*p_Field = threadPoolSum[p_Name].as<FT>();
        });

        const YAML::Node container = node["Container"];
        ForEachField<ContainerSettings>([&container, &settings](const char *p_Name, auto p_Field) {
            using FT = FieldType<decltype(p_Field)>;
            settings.Container.*p_Field = container[p_Name].as<FT>();
        });
    }
    std::filesystem::create_directories(g_Root + "/performance/results");
    return settings;
}
} // namespace TKit