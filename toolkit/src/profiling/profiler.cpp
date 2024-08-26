#include "core/pch.hpp"
#include "kit/profiling/profiler.hpp"

namespace KIT
{
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
    s_OngoingMeasurements.emplace_back(name, Clock{});
}

void Profiler::EndMeasurement() KIT_NOEXCEPT
{
    Measurement measurement;

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
} // namespace KIT