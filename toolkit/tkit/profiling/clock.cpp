#include "tkit/core/pch.hpp"
#include "tkit/profiling/clock.hpp"

namespace TKit
{
u64 Clock::timePointToU64(const TimePoint p_TimePoint)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(p_TimePoint.time_since_epoch()).count();
}

Timespan Clock::Restart()
{
    const auto now = GetCurrentTimePoint();
    const auto elapsed = now - m_Start;
    m_Start = now;
    return Timespan(elapsed);
}

} // namespace TKit
