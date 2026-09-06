#include "perf/memory.hpp"
#include "perf/multiprocessing.hpp"
#include "perf/container.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/utils/logging.hpp"
#include "tkit/utils/literals.hpp"
#include "tkit/memory/stack_allocator.hpp"

using namespace TKit::Alias;
using namespace TKit::Literals;

int main(int argc, char **argv)
{
    TKit::ArenaAllocator arena{8_kib};
    TKit::StackAllocator stack{1_kib};
    TKit::TierAllocator tier{{.Allocator = &arena, .MaxAllocation = 8_kib}};
    TKit::PushArena(&arena);
    TKit::PushStack(&stack);
    TKit::PushTier(&tier);

    const TKit::Settings settings = TKit::CreateSettings(argc, argv);
#ifdef TKIT_ENABLE_INFO_LOGS
    TKit::LogSettings(settings);
#endif

    TKit::Clock clock;
    TKIT_LOG_INFO("[TOOLKIT][PERF] Running thread pool sum...");
    RecordThreadPoolSum(settings.ThreadPoolSum);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running parallel sum...");
    RecordParallelSum(settings.ThreadPoolSum);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running malloc/free...");
    RecordMallocFree(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running block allocator...");
    RecordBlockAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running stack allocator...");
    RecordStackAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running arena allocator...");
    RecordArenaAllocator(settings.Allocation);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running vector...");
    RecordVector(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running dynamic array...");
    RecordDynamicArray(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Running static array...");
    RecordStaticArray(settings.Container);

    TKIT_LOG_INFO("[TOOLKIT][PERF] Done! ({:.1f} seconds) Results have been written to 'performance/results'",
                  clock.GetElapsed().AsSeconds());
    TKit::PopArena();
    TKit::PopStack();
    TKit::PopTier();
}
