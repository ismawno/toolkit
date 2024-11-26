#pragma once

#include "perf/settings.hpp"

namespace TKit
{
// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFreeST(const AllocationSettings &p_Settings);
void RecordMallocFreeMT(const AllocationSettings &p_Settings, usize p_MaxThreads);

void RecordBlockAllocatorConcurrentST(const AllocationSettings &p_Settings);
void RecordBlockAllocatorSerialST(const AllocationSettings &p_Settings);
void RecordBlockAllocatorMT(const AllocationSettings &p_Settings, usize p_MaxThreads);

void RecordStackAllocator(const AllocationSettings &p_Settings);
} // namespace TKit