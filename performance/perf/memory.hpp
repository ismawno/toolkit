#pragma once

#include "perf/settings.hpp"

namespace TKit::Perf
{
// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFree(const AllocationSettings &settings);

void RecordBlockAllocator(const AllocationSettings &settings);
void RecordStackAllocator(const AllocationSettings &settings);
void RecordArenaAllocator(const AllocationSettings &settings);
} // namespace TKit::Perf
