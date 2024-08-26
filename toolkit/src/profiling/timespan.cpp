#include "core/pch.hpp"
#include "kit/profiling/timespan.hpp"

namespace KIT
{
Timespan::Timespan(Nanoseconds p_Elapsed) KIT_NOEXCEPT : m_Elapsed(p_Elapsed)
{
}

void Timespan::Sleep(Timespan p_Duration) KIT_NOEXCEPT
{
    std::this_thread::sleep_for(p_Duration.m_Elapsed);
}

Timespan &Timespan::operator+=(const Timespan &other)
{
    m_Elapsed += other.m_Elapsed;
    return *this;
}

Timespan &Timespan::operator-=(const Timespan &other)
{
    m_Elapsed -= other.m_Elapsed;
    return *this;
}
} // namespace KIT
