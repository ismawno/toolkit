#include "core/pch.hpp"
#include "kit/profiling/clock.hpp"

namespace KIT
{
static u64 timePointToU64(const Clock::TimePoint p_TimePoint) noexcept
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(p_TimePoint.time_since_epoch()).count();
}

Clock::Clock() noexcept : m_Start(GetCurrentTimePoint())
{
}

u64 Clock::GetStartTime() const noexcept
{
    return timePointToU64(m_Start);
}

Clock::TimePoint Clock::GetStartTimePoint() const noexcept
{
    return m_Start;
}

Timespan Clock::GetElapsed() const noexcept
{
    return Timespan(GetCurrentTimePoint() - m_Start);
}

Timespan Clock::Restart() noexcept
{
    const auto now = GetCurrentTimePoint();
    const auto elapsed = now - m_Start;
    m_Start = now;
    return Timespan(elapsed);
}

u64 Clock::GetCurrentTime() noexcept
{
    return timePointToU64(GetCurrentTimePoint());
}

Clock::TimePoint Clock::GetCurrentTimePoint() noexcept
{
    return std::chrono::high_resolution_clock::now();
}
} // namespace KIT