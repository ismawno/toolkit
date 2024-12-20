#include "perf/container.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/container/static_array.hpp"
#include <fstream>

namespace TKit
{
void RecordDynamicArray(const ContainerSettings &p_Settings) noexcept
{
    std::ofstream file(g_Root + "/performance/results/dynamic_array.csv");
    file << "passes,pushback,pushfront,popback,popfront\n";

    DynamicArray<usize> array;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        array.resize(passes);
        Clock clock;
        array.push_back(passes);
        const Timespan pushBackTime = clock.Restart();

        array.pop_back();
        const Timespan popBackTime = clock.Restart();

        array.insert(array.begin(), passes);
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

    StaticArray<usize, 1000000> array;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        array.resize(passes);
        Clock clock;
        array.push_back(passes);
        const Timespan pushBackTime = clock.Restart();

        array.pop_back();
        const Timespan popBackTime = clock.Restart();

        array.insert(array.begin(), passes);
        const Timespan pushFrontTime = clock.Restart();

        array.erase(array.begin());
        const Timespan popFrontTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << '\n';
    }
}
} // namespace TKit