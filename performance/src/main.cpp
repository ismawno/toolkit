#include "perf/memory/block_allocator.hpp"

int main()
{
    KIT::BenchmarkSettings settings;
    KIT::RunBlockAllocatorBenchmarks(settings);
}