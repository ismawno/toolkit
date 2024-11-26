#include "tkit/core/pch.hpp"
#include "tkit/profiling/timespan.hpp"

namespace TKit
{
Timespan::Timespan(Nanoseconds p_Elapsed) noexcept : m_Elapsed(p_Elapsed)
{
}

void Timespan::Sleep(Timespan p_Duration) noexcept
{
    std::this_thread::sleep_for(p_Duration.m_Elapsed);
}

Timespan &Timespan::operator+=(const Timespan &p_Other)
{
    m_Elapsed += p_Other.m_Elapsed;
    return *this;
}

Timespan &Timespan::operator-=(const Timespan &p_Other)
{
    m_Elapsed -= p_Other.m_Elapsed;
    return *this;
}
} // namespace TKit
