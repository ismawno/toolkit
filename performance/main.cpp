#include "kit/memory/block_allocator.hpp"
#include "kit/profiling/profiler.hpp"
#include <fstream>
#include <filesystem>

KIT_NAMESPACE_BEGIN

static const std::string s_Root = KIT_ROOT_PATH;

struct ExampleData
{
    f32 Values1[4];

    void SetValues()
    {
        Values1[0] = 1.0f;
        Values1[1] = 2.0f;
        Values1[2] = 3.0f;
        Values1[3] = 4.0f;
    }
};

void RecordBlockAllocatorBenchmark()
{
    Profiler::Clear();
    constexpr usz maxElements = 100000;
    constexpr usz minElements = 100;
    constexpr usz increment = 100;

    BlockAllocator<ExampleData> allocator(64 * 64);
    std::vector<ExampleData *> blockAllocated{maxElements};
    std::vector<ExampleData *> mallocAllocated{maxElements};

    const std::filesystem::path directory = s_Root + "/performance/results";
    if (!std::filesystem::exists(directory))
        std::filesystem::create_directory(directory);

#ifdef KIT_BLOCK_ALLOCATOR_THREAD_SAFE
    std::ofstream file{directory.string() + "/block_allocator_thread_safe.csv"};
#else
    std::ofstream file{directory.string() + "/block_allocator_thread_unsafe.csv"};
#endif
    file << "elements,block_allocation,malloc_allocation,block_traversal,malloc_traversal,block_deallocation,malloc_"
            "deallocation\n";

    for (usz elements = minElements; elements <= maxElements; elements += increment)
    {
        file << elements << ',';
        {
            Profiler::Timer timer("Allocation (block)");
            for (usz i = 0; i < elements; ++i)
                blockAllocated[i] = allocator.Construct();
        }
        Measurement last = Profiler::GetLast();
        file << last.Elapsed.AsMilliseconds() << ',';

        {
            Profiler::Timer timer("Allocation (malloc)");
            for (usz i = 0; i < elements; ++i)
                mallocAllocated[i] = new ExampleData;
        }
        last = Profiler::GetLast();
        file << last.Elapsed.AsMilliseconds() << ',';

        {
            Profiler::Timer timer("Traversal (block)");
            for (usz i = 0; i < elements; ++i)
                blockAllocated[i]->SetValues();
        }
        last = Profiler::GetLast();
        file << last.Elapsed.AsMilliseconds() << ',';

        {
            Profiler::Timer timer("Traversal (malloc)");
            for (usz i = 0; i < elements; ++i)
                mallocAllocated[i]->SetValues();
        }
        last = Profiler::GetLast();
        file << last.Elapsed.AsMilliseconds() << ',';

        {
            Profiler::Timer timer("Deallocation (block)");
            for (usz i = 0; i < elements; ++i)
                allocator.Destroy(blockAllocated[i]);
        }
        last = Profiler::GetLast();
        file << last.Elapsed.AsMilliseconds() << ',';

        {
            Profiler::Timer timer("Deallocation (malloc)");
            for (usz i = 0; i < elements; ++i)
                delete mallocAllocated[i];
        }

        last = Profiler::GetLast();
        file << last.Elapsed.AsMilliseconds() << '\n';
        Profiler::Clear();
    }
}

KIT_NAMESPACE_END

int main()
{
    KIT::RecordBlockAllocatorBenchmark();
}