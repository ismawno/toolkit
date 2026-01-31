#include "perf/container.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include <deque>
#include <fstream>

namespace TKit
{
struct Example
{
    Example()
    {
        Element = new u32;
    }
    ~Example()
    {
        delete Element;
    }

    Example(const Example &other)
    {
        Element = new u32(*other.Element);
    }
    Example(Example &&other)
    {
        Element = other.Element;
        other.Element = nullptr;
    }

    Example &operator=(const Example &other)
    {
        if (this == &other)
            return *this;

        delete Element;
        *Element = *other.Element;
        return *this;
    }
    Example &operator=(Example &&other)
    {
        if (this == &other)
            return *this;
        delete Element;
        Element = other.Element;
        other.Element = nullptr;
        return *this;
    }

    u32 *Element;
};

// using Example = u32; // to test trivial types

void RecordVector(const ContainerSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/vector.csv");
    file << "passes,pushback,pushfront,popback,popfront,copy,move\n";

    std::vector<Example> array;
    array.reserve(settings.MaxPasses + 1);
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
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

        std::vector<Example> copyArray = array;
        const Timespan copyTime = clock.Restart();

        std::vector<Example> moveArray = std::move(copyArray);
        const Timespan moveTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << ',' << copyTime.AsNanoseconds()
             << ',' << moveTime.AsNanoseconds() << '\n';
    }
}

void RecordStaticArray(const ContainerSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/static_array.csv");
    file << "passes,append,insert,pop,remove\n";

    StaticArray<Example, 1000000> array;
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
    {
        array.Resize(passes);
        Clock clock;
        array.Append(Example{});
        const Timespan appendTime = clock.Restart();

        array.Pop();
        const Timespan popTime = clock.Restart();

        array.Insert(array.begin(), Example{});
        const Timespan insertTime = clock.Restart();

        array.RemoveOrdered(array.begin());
        const Timespan removeTime = clock.Restart();

        file << passes << ',' << appendTime.AsNanoseconds() << ',' << insertTime.AsNanoseconds() << ','
             << popTime.AsNanoseconds() << ',' << removeTime.AsNanoseconds() << '\n';
    }
}

void RecordDynamicArray(const ContainerSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/dynamic_array.csv");
    file << "passes,append,insert,pop,remove,copy,move\n";

    DynamicArray<Example> array;
    array.Reserve(settings.MaxPasses + 1);
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
    {
        array.Resize(passes);
        Clock clock;
        array.Append(Example{});
        const Timespan appendTime = clock.Restart();

        array.Pop();
        const Timespan popTime = clock.Restart();

        array.Insert(array.begin(), Example{});
        const Timespan insertTime = clock.Restart();

        array.RemoveOrdered(array.begin());
        const Timespan removeTime = clock.Restart();

        DynamicArray<Example> copyArray = array;
        const Timespan copyTime = clock.Restart();

        DynamicArray<Example> moveArray = std::move(copyArray);
        const Timespan moveTime = clock.Restart();

        file << passes << ',' << appendTime.AsNanoseconds() << ',' << insertTime.AsNanoseconds() << ','
             << popTime.AsNanoseconds() << ',' << removeTime.AsNanoseconds() << ',' << copyTime.AsNanoseconds() << ','
             << moveTime.AsNanoseconds() << '\n';
    }
}

void RecordDeque(const ContainerSettings &settings)
{
    std::ofstream file(g_Root + "/performance/results/deque.csv");
    file << "passes,pushback,pushfront,popback,popfront,copy,move\n";

    std::deque<Example> deque;
    for (usize passes = settings.MinPasses; passes <= settings.MaxPasses; passes += settings.PassIncrement)
    {
        Clock clock;
        deque.push_back(Example{});
        const Timespan pushBackTime = clock.Restart();

        deque.push_front(Example{});
        const Timespan pushFrontTime = clock.Restart();

        deque.pop_back();
        const Timespan popBackTime = clock.Restart();

        deque.pop_front();
        const Timespan popFrontTime = clock.Restart();

        std::deque<Example> copyDeque = deque;
        const Timespan copyTime = clock.Restart();

        std::deque<Example> moveDeque = std::move(copyDeque);
        const Timespan moveTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << ',' << copyTime.AsNanoseconds()
             << ',' << moveTime.AsNanoseconds() << '\n';
    }
}

} // namespace TKit
