#pragma once

#include "perf/settings.hpp"

KIT_NAMESPACE_BEGIN

// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFreeST(const AllocationSettings &p_Settings);
void RecordMallocFreeMT(const AllocationSettings &p_Settings, usize p_MaxThreads);

void RecordBlockAllocatorSafeST(const AllocationSettings &p_Settings);
void RecordBlockAllocatorUnsafeST(const AllocationSettings &p_Settings);
void RecordBlockAllocatorMT(const AllocationSettings &p_Settings, usize p_MaxThreads);

void RecordStackAllocator(const AllocationSettings &p_Settings);

KIT_NAMESPACE_END