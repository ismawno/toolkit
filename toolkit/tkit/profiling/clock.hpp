#pragma once

#ifndef TKIT_ENABLE_PROFILING
#    error                                                                                                             \
        "[TOOLKIT][PROFILING] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_PROFILING"
#endif

#include "tkit/profiling/timespan.hpp"

namespace TKit
{
class Clock
{
  public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Clock() : m_Start(GetCurrentTimePoint())
    {
    }

    u64 GetStartTime() const
    {
        return timePointToU64(m_Start);
    }

    TimePoint GetStartTimePoint() const
    {
        return m_Start;
    }

    Timespan GetElapsed() const
    {
        return Timespan(GetCurrentTimePoint() - m_Start);
    }
    Timespan Restart();

    static u64 GetCurrentTime()
    {
        return timePointToU64(GetCurrentTimePoint());
    }

    static TimePoint GetCurrentTimePoint()
    {
        return std::chrono::high_resolution_clock::now();
    }

  private:
    TimePoint m_Start;
    static u64 timePointToU64(const TimePoint timePoint);
};
} // namespace TKit
