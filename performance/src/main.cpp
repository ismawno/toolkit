#include "perf/memory.hpp"
#include "perf/multiprocessing.hpp"
#include "kit/memory/block_allocator.hpp"
#include "kit/profiling/clock.hpp"

using namespace KIT_NAMESPACE_NAME;

int main()
{
    const Settings settings = ReadOrWriteSettingsFile();

    Clock clock;
    KIT_LOG_INFO("Running thread pool sum...");
    RecordThreadPoolSum<std::mutex>(settings.ThreadPoolSum, settings.MaxThreads);

    KIT_LOG_INFO("Running parallel sum...");
    RecordParallelSum(settings.ThreadPoolSum, settings.MaxThreads);

    KIT_LOG_INFO("Running malloc/free ST...");
    RecordMallocFreeST(settings.Allocation);

    KIT_LOG_INFO("Running block allocator ST...");
    RecordBlockAllocatorST(settings.Allocation);

    KIT_LOG_INFO("Running malloc/free MT...");
    RecordMallocFreeMT(settings.Allocation, settings.MaxThreads);

    KIT_LOG_INFO("Running block allocator MT...");
    RecordBlockAllocatorMT(settings.Allocation, settings.MaxThreads);

    KIT_LOG_INFO("Running stack allocator...");
    RecordStackAllocator(settings.Allocation);

    KIT_LOG_INFO("Done! ({.1f} seconds) Results have been written to 'performance/results'",
                 clock.Elapsed().AsSeconds());
}