#include "perf/memory.hpp"
#include "perf/multiprocessing.hpp"
#include "tkit/memory/block_allocator.hpp"
#include "tkit/profiling/clock.hpp"

using namespace TKit;

int main()
{
    const Settings settings = ReadOrWriteSettingsFile();

    Clock clock;
    TKIT_LOG_INFO("Running thread pool sum...");
    RecordThreadPoolSum<std::mutex>(settings.ThreadPoolSum, settings.MaxThreads);

    TKIT_LOG_INFO("Running parallel sum...");
    RecordParallelSum(settings.ThreadPoolSum, settings.MaxThreads);

    TKIT_LOG_INFO("Running malloc/free ST...");
    RecordMallocFreeST(settings.Allocation);

    TKIT_LOG_INFO("Running block allocator ST...");
    RecordBlockAllocatorConcurrentST(settings.Allocation);
    RecordBlockAllocatorSerialST(settings.Allocation);

    TKIT_LOG_INFO("Running malloc/free MT...");
    RecordMallocFreeMT(settings.Allocation, settings.MaxThreads);

    TKIT_LOG_INFO("Running block allocator MT...");
    RecordBlockAllocatorMT(settings.Allocation, settings.MaxThreads);

    TKIT_LOG_INFO("Running stack allocator...");
    RecordStackAllocator(settings.Allocation);

    TKIT_LOG_INFO("Done! ({:.1f} seconds) Results have been written to 'performance/results'",
                  clock.GetElapsed().AsSeconds());
}