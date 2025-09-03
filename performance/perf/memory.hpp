#pragma once

#include "perf/settings.hpp"

namespace TKit::Perf
{
// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFree(const AllocationSettings &p_Settings);

void RecordBlockAllocator(const AllocationSettings &p_Settings);
void RecordStackAllocator(const AllocationSettings &p_Settings);
void RecordArenaAllocator(const AllocationSettings &p_Settings);
} // namespace TKit::Perf
