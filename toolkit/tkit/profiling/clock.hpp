#pragma once

#ifndef TKIT_ENABLE_PROFILING
#    error                                                                                                             \
        "[TOOLKIT][PROFILING] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_PROFILING"
#endif

#include "tkit/profiling/timespan.hpp"
#include "tkit/utils/logging.hpp"

#define TKIT_BEGIN_CLOCK(level)                                                                                        \
    const TKit::Clock __tkit_##level##_clock                                                                           \
    {                                                                                                                  \
    }

#define TKIT_END_CLOCK(level, unit, ...)                                                                               \
    const auto elapsed = __tkit_##level##_clock.GetElapsed().As##unit();                                               \
    if (TKit::Detail::t_LogMask & TKIT_##level##_LOGS_BIT)                                                             \
    TKit::Detail::Log(TKit::Format(__VA_ARGS__, elapsed), #level, TKIT_LOG_COLOR_##level)

#ifdef TKIT_ENABLE_DEBUG_LOGS
#    define TKIT_BEGIN_DEBUG_CLOCK() TKIT_BEGIN_CLOCK(DEBUG)
#    define TKIT_END_DEBUG_CLOCK(unit, ...) TKIT_END_CLOCK(DEBUG, unit, __VA_ARGS__)
#else
#    define TKIT_BEGIN_DEBUG_CLOCK()
#    define TKIT_END_DEBUG_CLOCK(unit, ...)
#endif

#ifdef TKIT_ENABLE_INFO_LOGS
#    define TKIT_BEGIN_INFO_CLOCK() TKIT_BEGIN_CLOCK(INFO)
#    define TKIT_END_INFO_CLOCK(unit, ...) TKIT_END_CLOCK(INFO, unit, __VA_ARGS__)
#else
#    define TKIT_BEGIN_INFO_CLOCK()
#    define TKIT_END_INFO_CLOCK(unit, ...)
#endif

#ifdef TKIT_ENABLE_WARNING_LOGS
#    define TKIT_BEGIN_WARNING_CLOCK() TKIT_BEGIN_CLOCK(WARNING)
#    define TKIT_END_WARNING_CLOCK(unit, ...) TKIT_END_CLOCK(WARNING, unit, __VA_ARGS__)
#else
#    define TKIT_BEGIN_WARNING_CLOCK()
#    define TKIT_END_WARNING_CLOCK(unit, ...)
#endif

#ifdef TKIT_ENABLE_ERROR_LOGS
#    define TKIT_BEGIN_ERROR_CLOCK() TKIT_BEGIN_CLOCK(ERROR)
#    define TKIT_END_ERROR_CLOCK(unit, ...) TKIT_END_CLOCK(ERROR, unit, __VA_ARGS__)
#else
#    define TKIT_BEGIN_ERROR_CLOCK()
#    define TKIT_END_ERROR_CLOCK(unit, ...)
#endif

namespace TKit
{
class Clock
{
  public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Clock() : m_Start(GetCurrentTimePoint())
    {
    }

    u64 GetStartTime() const
    {
        return timePointToU64(m_Start);
    }

    TimePoint GetStartTimePoint() const
    {
        return m_Start;
    }

    Timespan GetElapsed() const
    {
        return Timespan(GetCurrentTimePoint() - m_Start);
    }
    Timespan Restart();

    static u64 GetCurrentTime()
    {
        return timePointToU64(GetCurrentTimePoint());
    }

    static TimePoint GetCurrentTimePoint()
    {
        return std::chrono::high_resolution_clock::now();
    }

  private:
    TimePoint m_Start;
    static u64 timePointToU64(const TimePoint timePoint);
};
} // namespace TKit
