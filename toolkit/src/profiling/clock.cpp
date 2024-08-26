#include "core/pch.hpp"
#include "kit/profiling/clock.hpp"

namespace KIT
{
static u64 timePointToU64(const Clock::TimePoint p_TimePoint) noexcept
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(p_TimePoint.time_since_epoch()).count();
}

Clock::Clock() noexcept : m_Start(CurrentTimePoint())
{
}

u64 Clock::StartTime() const noexcept
{
    return timePointToU64(m_Start);
}

Clock::TimePoint Clock::StartTimePoint() const noexcept
{
    return m_Start;
}

Timespan Clock::Elapsed() const noexcept
{
    return Timespan(CurrentTimePoint() - m_Start);
}

Timespan Clock::Restart() noexcept
{
    const auto now = CurrentTimePoint();
    const auto elapsed = now - m_Start;
    m_Start = now;
    return Timespan(elapsed);
}

u64 Clock::CurrentTime() noexcept
{
    return timePointToU64(CurrentTimePoint());
}

Clock::TimePoint Clock::CurrentTimePoint() noexcept
{
    return std::chrono::high_resolution_clock::now();
}
} // namespace KIT