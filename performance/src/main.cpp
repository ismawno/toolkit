#include "perf/memory.hpp"
#include "perf/multiprocessing.hpp"
#include "kit/memory/block_allocator.hpp"

using namespace KIT_NAMESPACE_NAME;

int main()
{
    const Settings settings = ReadOrWriteSettingsFile();

    RecordThreadPoolSum<SpinMutex>(settings.ThreadPoolSum, settings.MaxThreads);

    // RecordMallocFreeST(settings.Allocation);
    // RecordBlockAllocatorST(settings.Allocation);

    // RecordMallocFreeMT(settings.Allocation, settings.MaxThreads);
    // RecordBlockAllocatorMT(settings.Allocation, settings.MaxThreads);

    // RecordStackAllocator(settings.Allocation);
}