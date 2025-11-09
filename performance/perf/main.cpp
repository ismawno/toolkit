#include "perf/memory.hpp"
#include "perf/multiprocessing.hpp"
#include "perf/container.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/utils/logging.hpp"

using namespace TKit::Alias;

int main(int argc, char **argv)
{
    const TKit::Perf::Settings settings = TKit::Perf::CreateSettings(argc, argv);
    TKit::Perf::LogSettings(settings);

    TKit::Clock clock;
    TKit::Info("[TOOLKIT][PERF] Running thread pool sum...");
    RecordThreadPoolSum(settings.ThreadPoolSum);

    TKit::Info("[TOOLKIT][PERF] Running parallel sum...");
    RecordParallelSum(settings.ThreadPoolSum);

    TKit::Info("[TOOLKIT][PERF] Running malloc/free...");
    RecordMallocFree(settings.Allocation);

    TKit::Info("[TOOLKIT][PERF] Running block allocator...");
    RecordBlockAllocator(settings.Allocation);

    TKit::Info("[TOOLKIT][PERF] Running stack allocator...");
    RecordStackAllocator(settings.Allocation);

    TKit::Info("[TOOLKIT][PERF] Running arena allocator...");
    RecordArenaAllocator(settings.Allocation);

    TKit::Info("[TOOLKIT][PERF] Running vector...");
    RecordVector(settings.Container);

    TKit::Info("[TOOLKIT][PERF] Running dynamic array...");
    RecordDynamicArray(settings.Container);

    TKit::Info("[TOOLKIT][PERF] Running static array...");
    RecordStaticArray(settings.Container);

    TKit::Info("[TOOLKIT][PERF] Running deque...");
    RecordDeque(settings.Container);

    TKit::Info("[TOOLKIT][PERF] Running dynamic deque...");
    RecordDynamicDeque(settings.Container);

    TKit::Info("[TOOLKIT][PERF] Running static deque...");
    RecordStaticDeque(settings.Container);

    TKit::Info("[TOOLKIT][PERF] Done! ({:.1f} seconds) Results have been written to 'performance/results'",
               clock.GetElapsed().AsSeconds());
}
