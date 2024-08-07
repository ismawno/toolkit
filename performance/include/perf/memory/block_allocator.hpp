#pragma once
#include "kit/core/alias.hpp"

KIT_NAMESPACE_BEGIN

struct BenchmarkSettings
{
    usz MinPasses = 10;
    usz MaxPasses = 10000;
    usz PassIncrement = 1;
    usz Containers = 10;
};

void RunBlockAllocatorBenchmarks(const BenchmarkSettings &p_Settings);

KIT_NAMESPACE_END