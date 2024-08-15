#pragma once

#include "perf/settings.hpp"

KIT_NAMESPACE_BEGIN

// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFree(const AllocationSettings &p_Settings);
void RecordBlockAllocator(const AllocationSettings &p_Settings);

KIT_NAMESPACE_END