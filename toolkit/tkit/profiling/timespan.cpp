#include "tkit/core/pch.hpp"
#include "tkit/profiling/timespan.hpp"
#include <thread>

namespace TKit
{
void Timespan::Sleep(const Timespan duration)
{
    std::this_thread::sleep_for(duration.m_Elapsed);
}
} // namespace TKit
