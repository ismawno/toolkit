#pragma once

#include "perf/settings.hpp"

namespace TKit::Perf
{
// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFree(const AllocationSettings &p_Settings) noexcept;

void RecordBlockAllocator(const AllocationSettings &p_Settings) noexcept;
void RecordStackAllocator(const AllocationSettings &p_Settings) noexcept;
void RecordArenaAllocator(const AllocationSettings &p_Settings) noexcept;
} // namespace TKit::Perf
