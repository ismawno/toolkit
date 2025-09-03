#include "perf/container.hpp"
#include "tkit/profiling/clock.hpp"
#include "tkit/container/static_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/dynamic_deque.hpp"
#include "tkit/container/static_deque.hpp"
#include <deque>
#include <fstream>

namespace TKit::Perf
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

    Example(const Example &p_Other)
    {
        Element = new u32(*p_Other.Element);
    }
    Example(Example &&p_Other)
    {
        Element = p_Other.Element;
        p_Other.Element = nullptr;
    }

    Example &operator=(const Example &p_Other)
    {
        if (this == &p_Other)
            return *this;
        *Element = *p_Other.Element;
        return *this;
    }
    Example &operator=(Example &&p_Other)
    {
        if (this == &p_Other)
            return *this;
        Element = p_Other.Element;
        p_Other.Element = nullptr;
        return *this;
    }

    u32 *Element;
};

// using Example = u32; // to test trivial types

void RecordVector(const ContainerSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/vector.csv");
    file << "passes,pushback,pushfront,popback,popfront,copy,move\n";

    std::vector<Example> array;
    array.reserve(p_Settings.MaxPasses + 1);
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

        std::vector<Example> copyArray = array;
        const Timespan copyTime = clock.Restart();

        std::vector<Example> moveArray = std::move(copyArray);
        const Timespan moveTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << ',' << copyTime.AsNanoseconds()
             << ',' << moveTime.AsNanoseconds() << '\n';
    }
}

void RecordStaticArray(const ContainerSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/static_array.csv");
    file << "passes,append,insert,pop,remove\n";

    StaticArray<Example, 1000000> array;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
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

void RecordDynamicArray(const ContainerSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/dynamic_array.csv");
    file << "passes,append,insert,pop,remove,copy,move\n";

    DynamicArray<Example> array;
    array.Reserve(p_Settings.MaxPasses + 1);
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
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

void RecordDeque(const ContainerSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/deque.csv");
    file << "passes,pushback,pushfront,popback,popfront,copy,move\n";

    std::deque<Example> deque;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
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
void RecordStaticDeque(const ContainerSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/static_deque.csv");
    file << "passes,pushback,pushfront,popback,popfront\n";

    StaticDeque<Example, 1000000> deque;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        deque.PushBack(Example{});
        const Timespan pushBackTime = clock.Restart();

        deque.PushFront(Example{});
        const Timespan pushFrontTime = clock.Restart();

        deque.PopBack();
        const Timespan popBackTime = clock.Restart();

        deque.PopFront();
        const Timespan popFrontTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << '\n';
    }
}

void RecordDynamicDeque(const ContainerSettings &p_Settings)
{
    std::ofstream file(g_Root + "/performance/results/dynamic_deque.csv");
    file << "passes,pushback,pushfront,popback,popfront,copy,move\n";

    DynamicDeque<Example> deque;
    for (usize passes = p_Settings.MinPasses; passes <= p_Settings.MaxPasses; passes += p_Settings.PassIncrement)
    {
        Clock clock;
        deque.PushBack(Example{});
        const Timespan pushBackTime = clock.Restart();

        deque.PushFront(Example{});
        const Timespan pushFrontTime = clock.Restart();

        deque.PopBack();
        const Timespan popBackTime = clock.Restart();

        deque.PopFront();
        const Timespan popFrontTime = clock.Restart();

        DynamicDeque<Example> copyDeque = deque;
        const Timespan copyTime = clock.Restart();

        DynamicDeque<Example> moveDeque = std::move(copyDeque);
        const Timespan moveTime = clock.Restart();

        file << passes << ',' << pushBackTime.AsNanoseconds() << ',' << pushFrontTime.AsNanoseconds() << ','
             << popBackTime.AsNanoseconds() << ',' << popFrontTime.AsNanoseconds() << ',' << copyTime.AsNanoseconds()
             << ',' << moveTime.AsNanoseconds() << '\n';
    }
}

} // namespace TKit::Perf
