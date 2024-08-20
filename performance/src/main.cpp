#include "perf/memory.hpp"
#include "kit/memory/block_allocator.hpp"
#include <filesystem>
#include <fstream>

KIT_WARNING_IGNORE_PUSH
KIT_GCC_WARNING_IGNORE("-Wunused-parameter")
KIT_CLANG_WARNING_IGNORE("-Wunused-parameter")
KIT_MSVC_WARNING_IGNORE(4100)
#include <yaml-cpp/yaml.h>
KIT_WARNING_IGNORE_POP

using namespace KIT_NAMESPACE_NAME;

int main()
{
    AllocationSettings settings;
    usize maxThreads = 8;
    if (!std::filesystem::exists(g_Root + "/performance/perf-settings.yaml"))
    {
        YAML::Emitter emitter;
        YAML::Node node;
        node["MT"]["MaxThreads"] = 8;
        YAML::Node memory = node["Memory"];
        memory["MinPasses"] = settings.MinPasses;
        memory["MaxPasses"] = settings.MaxPasses;
        memory["PassIncrement"] = settings.PassIncrement;

        emitter << node;
        std::ofstream file(g_Root + "/performance/perf-settings.yml");
        file << emitter.c_str();
    }
    else
    {
        const YAML::Node node = YAML::LoadFile(g_Root + "/performance/perf-settings.yaml");
        maxThreads = node["MT"]["MaxThreads"].as<usize>();

        const YAML::Node memory = node["Memory"];
        settings.MinPasses = memory["MinPasses"].as<usize>();
        settings.MaxPasses = memory["MaxPasses"].as<usize>();
        settings.PassIncrement = memory["PassIncrement"].as<usize>();
    }
    std::filesystem::create_directories(g_Root + "/performance/results");

    RecordMallocFreeST(settings);
    RecordBlockAllocatorST(settings);

    RecordMallocFreeMT(settings, maxThreads);
    RecordBlockAllocatorMT(settings, maxThreads);

    RecordStackAllocator(settings);
}