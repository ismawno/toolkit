#include "perf/memory.hpp"
#include "perf/multiprocessing.hpp"
#include "perf/container.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/utils/logging.hpp"

using namespace TKit::Alias;

int main(int argc, char **argv)
{
    const TKit::Perf::Settings settings = TKit::Perf::CreateSettings(argc, argv);
#ifdef TKIT_ENABLE_INFO_LOGS
    TKit::Perf::LogSettings(settings);
#endif

    TKit::Clock clock;
    TKIT_LOG_INFO("[TOOLKIT] Running thread pool sum...");
    RecordThreadPoolSum(settings.ThreadPoolSum);

    TKIT_LOG_INFO("[TOOLKIT] Running parallel sum...");
    RecordParallelSum(settings.ThreadPoolSum);

    TKIT_LOG_INFO("[TOOLKIT] Running malloc/free...");
    RecordMallocFree(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running block allocator...");
    RecordBlockAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running stack allocator...");
    RecordStackAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running arena allocator...");
    RecordArenaAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT] Running vector...");
    RecordVector(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT] Running dynamic array...");
    RecordDynamicArray(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT] Running static array...");
    RecordStaticArray(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT] Done! ({:.1f} seconds) Results have been written to 'performance/results'",
                  clock.GetElapsed().AsSeconds());
}
