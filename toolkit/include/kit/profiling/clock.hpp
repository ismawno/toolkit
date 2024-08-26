#pragma once

#include "kit/profiling/timespan.hpp"

namespace KIT
{
class KIT_API Clock
{
  public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Clock() KIT_NOEXCEPT;

    u64 StartTime() const KIT_NOEXCEPT;
    TimePoint StartTimePoint() const KIT_NOEXCEPT;

    Timespan Elapsed() const KIT_NOEXCEPT;
    Timespan Restart() KIT_NOEXCEPT;

    static u64 CurrentTime() KIT_NOEXCEPT;
    static TimePoint CurrentTimePoint() KIT_NOEXCEPT;

  private:
    TimePoint m_Start;
};
} // namespace KIT