#include "tkit/core/pch.hpp"
#include "tkit/profiling/clock.hpp"

namespace TKit
{
static u64 timePointToU64(const Clock::TimePoint p_TimePoint)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(p_TimePoint.time_since_epoch()).count();
}

Clock::Clock() : m_Start(GetCurrentTimePoint())
{
}

u64 Clock::GetStartTime() const
{
    return timePointToU64(m_Start);
}

Clock::TimePoint Clock::GetStartTimePoint() const
{
    return m_Start;
}

Timespan Clock::GetElapsed() const
{
    return Timespan(GetCurrentTimePoint() - m_Start);
}

Timespan Clock::Restart()
{
    const auto now = GetCurrentTimePoint();
    const auto elapsed = now - m_Start;
    m_Start = now;
    return Timespan(elapsed);
}

u64 Clock::GetCurrentTime()
{
    return timePointToU64(GetCurrentTimePoint());
}

Clock::TimePoint Clock::GetCurrentTimePoint()
{
    return std::chrono::high_resolution_clock::now();
}
} // namespace TKit