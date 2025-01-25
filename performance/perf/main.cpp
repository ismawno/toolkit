#include "perf/memory.hpp"
#include "perf/multiprocessing.hpp"
#include "perf/container.hpp"
#include "tkit/memory/block_allocator.hpp"
#include "tkit/profiling/clock.hpp"

using namespace TKit;

int main()
{
    const Settings settings = ReadOrWriteSettingsFile();

    Clock clock;
    TKIT_LOG_INFO("[TOOLKIT] Running thread pool sum...");
    RecordThreadPoolSum(settings.ThreadPoolSum, settings.MaxThreads);

    TKIT_LOG_INFO("[TOOLKIT] Running parallel sum...");
    RecordParallelSum(settings.ThreadPoolSum, settings.MaxThreads);

    TKIT_LOG_INFO("[TOOLKIT] Running malloc/free...");
    RecordMallocFree(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running block allocator...");
    RecordBlockAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running stack allocator...");
    RecordStackAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running arena allocator...");
    RecordArenaAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running dynamic array...");
    RecordDynamicArray(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT] Running static array...");
    RecordStaticArray(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT] Done! ({:.1f} seconds) Results have been written to 'performance/results'",
                  clock.GetElapsed().AsSeconds());
}