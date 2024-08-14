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

int main()
{
    KIT::AllocationSettings settings;
    if (!std::filesystem::exists(KIT::g_Root + "/performance/perf-settings.yaml"))
    {
        YAML::Emitter emitter;
        YAML::Node node;
        YAML::Node memory = node["Memory"];
        memory["MinPasses"] = settings.MinPasses;
        memory["MaxPasses"] = settings.MaxPasses;
        memory["PassIncrement"] = settings.PassIncrement;

        emitter << node;
        std::ofstream file(KIT::g_Root + "/performance/perf-settings.yml");
        file << emitter.c_str();
    }
    else
    {
        const YAML::Node node = YAML::LoadFile(KIT::g_Root + "/performance/perf-settings.yaml");
        const YAML::Node memory = node["Memory"];
        settings.MinPasses = memory["MinPasses"].as<KIT::usz>();
        settings.MaxPasses = memory["MaxPasses"].as<KIT::usz>();
        settings.PassIncrement = memory["PassIncrement"].as<KIT::usz>();
    }
    std::filesystem::create_directories(KIT::g_Root + "/performance/results");

    KIT::RecordMallocFreeSingleThreaded(settings);
    KIT::RecordBlockAllocatorSingleThreaded<KIT::TSafeBlockAllocator>(settings);
    KIT::RecordBlockAllocatorSingleThreaded<KIT::TUnsafeBlockAllocator>(settings);
}