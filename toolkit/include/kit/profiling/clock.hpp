#pragma once

#include "kit/profiling/timespan.hpp"

namespace KIT
{
class KIT_API Clock
{
  public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Clock() noexcept;

    u64 StartTime() const noexcept;
    TimePoint StartTimePoint() const noexcept;

    Timespan Elapsed() const noexcept;
    Timespan Restart() noexcept;

    static u64 CurrentTime() noexcept;
    static TimePoint CurrentTimePoint() noexcept;

  private:
    TimePoint m_Start;
};
} // namespace KIT