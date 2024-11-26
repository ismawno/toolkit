#pragma once

#include "tkit/profiling/timespan.hpp"

namespace TKit
{
class TKIT_API Clock
{
  public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Clock() noexcept;

    u64 GetStartTime() const noexcept;
    TimePoint GetStartTimePoint() const noexcept;

    Timespan GetElapsed() const noexcept;
    Timespan Restart() noexcept;

    static u64 GetCurrentTime() noexcept;
    static TimePoint GetCurrentTimePoint() noexcept;

  private:
    TimePoint m_Start;
};
} // namespace TKit