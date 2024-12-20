#pragma once

#include "perf/settings.hpp"

namespace TKit
{
// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFreeST(const AllocationSettings &p_Settings) noexcept;
void RecordMallocFreeMT(const AllocationSettings &p_Settings, usize p_MaxThreads) noexcept;

void RecordBlockAllocatorConcurrentST(const AllocationSettings &p_Settings) noexcept;
void RecordBlockAllocatorSerialST(const AllocationSettings &p_Settings) noexcept;
void RecordBlockAllocatorMT(const AllocationSettings &p_Settings, usize p_MaxThreads) noexcept;

void RecordStackAllocator(const AllocationSettings &p_Settings) noexcept;
} // namespace TKit