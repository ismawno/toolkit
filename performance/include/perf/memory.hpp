#pragma once

#include "perf/settings.hpp"

KIT_NAMESPACE_BEGIN

// This function assumes the default new/delete uses the malloc/free functions.
void RecordMallocFreeSingleThreaded(const AllocationSettings &p_Settings);
template <template <typename> typename Allocator>
void RecordBlockAllocatorSingleThreaded(const AllocationSettings &p_Settings);

KIT_NAMESPACE_END