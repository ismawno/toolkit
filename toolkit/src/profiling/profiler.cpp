#include "core/pch.hpp"
#include "kit/profiling/profiler.hpp"
#include <mutex>

KIT_NAMESPACE_BEGIN

static std::mutex s_Mutex;

Profiler::Timer::Timer(const char *name) KIT_NOEXCEPT
{
    Profiler::BeginMeasurement(name);
}
Profiler::Timer::~Timer() KIT_NOEXCEPT
{
    Profiler::EndMeasurement();
}

void Profiler::BeginMeasurement(const char *name) KIT_NOEXCEPT
{
    std::scoped_lock lock(s_Mutex);
    s_OngoingMeasurements.emplace_back(name, Clock{});
}

void Profiler::EndMeasurement() KIT_NOEXCEPT
{
    Measurement measurement;
    std::scoped_lock lock(s_Mutex);

    const auto [name, clock] = s_OngoingMeasurements.back();
    measurement.Name = name;
    measurement.Elapsed = clock.Elapsed();
    s_OngoingMeasurements.pop_back();

    s_Measurements.push_back(measurement);
}

const Measurement &Profiler::GetLast() KIT_NOEXCEPT
{
    return s_Measurements.back();
}

void Profiler::Clear() KIT_NOEXCEPT
{
    std::scoped_lock lock(s_Mutex);
    s_Measurements.clear();
}

bool Profiler::Empty() KIT_NOEXCEPT
{
    return s_Measurements.empty();
}

const DynamicArray<Measurement> &Profiler::Measurements() KIT_NOEXCEPT
{
    return s_Measurements;
}

HashMap<const char *, AggregatedMeasurement> Profiler::AggregateMeasurements() KIT_NOEXCEPT
{
    HashMap<const char *, AggregatedMeasurement> aggregated;

    // Should actually be a shared lock, but this method is not supposed to be called often
    std::scoped_lock lock(s_Mutex);

    for (const Measurement &measurement : s_Measurements)
    {
        AggregatedMeasurement &aggMeasurement = aggregated[measurement.Name];
        aggMeasurement.Name = measurement.Name;
        aggMeasurement.Min = std::min(aggMeasurement.Min, measurement.Elapsed);
        aggMeasurement.Max = std::max(aggMeasurement.Max, measurement.Elapsed);
        aggMeasurement.Total += measurement.Elapsed;
        ++aggMeasurement.Calls;
    }

    for (auto &[name, aggMeasurement] : aggregated)
        aggMeasurement.Average = aggMeasurement.Total / aggMeasurement.Calls;

    return aggregated;
}

KIT_NAMESPACE_END