#include "tkit/core/pch.hpp"
#include "tkit/profiling/timespan.hpp"
#include <thread>

namespace TKit
{
void Timespan::Sleep(const Timespan p_Duration)
{
    std::this_thread::sleep_for(p_Duration.m_Elapsed);
}
} // namespace TKit
