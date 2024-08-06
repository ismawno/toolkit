#include "core/pch.hpp"
#include "kit/profiling/clock.hpp"

KIT_NAMESPACE_BEGIN

static u64 timePointToU64(const Clock::TimePoint p_TimePoint) KIT_NOEXCEPT
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(p_TimePoint.time_since_epoch()).count();
}

Clock::Clock() KIT_NOEXCEPT : m_Start(CurrentTimePoint())
{
}

u64 Clock::StartTime() const KIT_NOEXCEPT
{
    return timePointToU64(m_Start);
}

Clock::TimePoint Clock::StartTimePoint() const KIT_NOEXCEPT
{
    return m_Start;
}

Timespan Clock::Elapsed() const KIT_NOEXCEPT
{
    return Timespan(CurrentTimePoint() - m_Start);
}

Timespan Clock::Restart() KIT_NOEXCEPT
{
    const auto now = CurrentTimePoint();
    const auto elapsed = now - m_Start;
    m_Start = now;
    return Timespan(elapsed);
}

u64 Clock::CurrentTime() KIT_NOEXCEPT
{
    return timePointToU64(CurrentTimePoint());
}

Clock::TimePoint Clock::CurrentTimePoint() KIT_NOEXCEPT
{
    return std::chrono::high_resolution_clock::now();
}

KIT_NAMESPACE_END