#pragma once

#include "kit/profiling/clock.hpp"
#include "kit/core/non_copyable.hpp"

namespace KIT
{
struct KIT_API Measurement
{
    const char *Name = nullptr;
    Timespan Elapsed{};
};

struct KIT_API AggregatedMeasurement
{
    const char *Name = nullptr;
    Timespan Average{};
    Timespan Min{};
    Timespan Max{};
    Timespan Total{};
    u32 Calls = 0;
};

// A simple and straightforward profiler. I have implemented several in the past (last one besides this one is in
// cpp-kit), some of them with a lot of fancy features such as measurement hierarchies, percentages etcetera. Truth is,
// none of those features is worth the effort, and they introduce such overhead that they end up mangling the
// measurements you try to take, specially in functions that take a very short time. A truly example of over engineering

// This one is not like that at least. You Begin and End measurements with the methods below, or use a Timer to automate
// the process with its constructor/destructor. You then can retrieve the measurements, and aggregate them in case you
// took several with the same name
class KIT_API Profiler
{
  public:
    struct Timer
    {
        KIT_NON_COPYABLE(Timer)
        explicit Timer(const char *name) KIT_NOEXCEPT;
        ~Timer() KIT_NOEXCEPT;
    };

    // I had originally thought of returning the measurement when calling EndMeasurement, but 1: It will be discarded
    // often (specially when using Timer), 2: returning a reference to the allocated DynamicArray slot is dangerous
    // because of resizing and 3: returning a copy when it is not guaranteed to be used seems wasteful. Im sure compiler
    // may optimize it away with copy ellision or some other crap, but still
    static void BeginMeasurement(const char *name) KIT_NOEXCEPT;
    static void EndMeasurement() KIT_NOEXCEPT;
    static const Measurement &GetLast() KIT_NOEXCEPT;

    static void Clear() KIT_NOEXCEPT;
    static bool Empty() KIT_NOEXCEPT;

    static const DynamicArray<Measurement> &Measurements() KIT_NOEXCEPT;
    static HashMap<const char *, AggregatedMeasurement> AggregateMeasurements() KIT_NOEXCEPT;

  private:
    using OngoingMeasurement = std::pair<const char *, Clock>;

    // I have considered using a stack allocator to store measurement, but its a bit pointless. These wont be resized
    // often, and already serve somewhat as a stack. One of the advantages of a stack allocator is that you can have
    // different data types in the same stack, close together in memory. But in this case, all memory is the same and
    // will be close enough anyway
    static inline DynamicArray<Measurement> s_Measurements{};
    static inline DynamicArray<OngoingMeasurement> s_OngoingMeasurements{};
};
} // namespace KIT