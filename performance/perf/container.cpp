#include "perf/container.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/container/static_array.hpp"
#include <fstream>

namespace TKit
{
struct Example
{
    Example() noexcept
    {
        Element = new u32;
    }
    ~Example() noexcept
    {
        delete Element;
    }

    Example(const Example &p_Other) noexcept
    {
        Element = new u32(*p_Other.Element);
    }
    Example(Example &&p_Other) noexcept
    {
        Element = p_Other.Element;
        p_Other.Element = nullptr;
    }

    Example &operator=(const Example &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;
        *Element = *p_Other.Element;
        return *this;
    }
    Example &operator=(Example &&p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;
        Element = p_Other.Element;
        p_Other.Element = nullptr;
        return *this;
    }

    u32 *Element;
};

void RecordDynamicArray(const ContainerSettings &p_Settings) noexcept
{
    std::ofstream file(g_Root + "/performance/results/dynamic_array.csv");
    file << "passes,pushback,pushfront,popback,popfront\n";

    DynamicArray<Example> array;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        array.resize(passes);
        Clock clock;
        array.push_back(Example{});
        const Timespan pushBackTime = clock.Restart();

        array.pop_back();
        const Timespan popBackTime = clock.Restart();

        array.insert(array.begin(), Example{});
        const Timespan pushFrontTime = clock.Restart();

        array.erase(array.begin());
        const Timespan popFrontTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << '\n';
    }
}

void RecordStaticArray(const ContainerSettings &p_Settings) noexcept
{
    std::ofstream file(g_Root + "/performance/results/static_array.csv");
    file << "passes,pushback,pushfront,popback,popfront\n";

    StaticArray<Example, 1000000> array;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        array.resize(passes);
        Clock clock;
        array.push_back(Example{});
        const Timespan pushBackTime = clock.Restart();

        array.pop_back();
        const Timespan popBackTime = clock.Restart();

        array.insert(array.begin(), Example{});
        const Timespan pushFrontTime = clock.Restart();

        array.erase(array.begin());
        const Timespan popFrontTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << '\n';
    }
}
} // namespace TKit