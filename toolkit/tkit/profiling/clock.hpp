#pragma once

#ifndef TKIT_ENABLE_PROFILING
#    error                                                                                                             \
        "[TOOLKIT][PROFILING] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_PROFILING"
#endif

#include "tkit/profiling/timespan.hpp"

namespace TKit
{
class TKIT_API Clock
{
  public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Clock();

    u64 GetStartTime() const;
    TimePoint GetStartTimePoint() const;

    Timespan GetElapsed() const;
    Timespan Restart();

    static u64 GetCurrentTime();
    static TimePoint GetCurrentTimePoint();

  private:
    TimePoint m_Start;
};
} // namespace TKit
