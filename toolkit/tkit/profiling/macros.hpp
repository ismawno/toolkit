#pragma once

#if defined(TKIT_ENABLE_INSTRUMENTATION) || defined(TKIT_ENABLE_SAMPLING)
#    include "tracy/Tracy.hpp"
#    define TKIT_PROFILE_NOOP() TracyNoop
#else
#    define TKIT_PROFILE_NOOP()
#endif

#ifdef TKIT_ENABLE_INSTRUMENTATION

// The main reason of this wrap around the tracy library is, in case I switch to another profiling library, I can keep
// the same interface and just change the implementation of the macros.

namespace TKit
{
using ProfilingPlotFormat = tracy::PlotFormatType;
} // namespace TKit

#    define TKIT_PROFILE_MARK_FRAME() FrameMark
#    define TKIT_PROFILE_MARK_FRAME_START(name) FrameMarkStart(name)
#    define TKIT_PROFILE_MARK_FRAME_END(name) FrameMarkEnd(name)
#    define TKIT_PROFILE_NMARK_FRAME(name) FrameMarkNamed(name)

#    define TKIT_PROFILE_SCOPE_IF(enabled) ZoneNamed(__tkit_perf_scope##__LINE__, enabled)
#    define TKIT_PROFILE_NSCOPE_IF(enabled, name) ZoneNamedN(__tkit_perf_scope##__LINE__, name, enabled)
#    define TKIT_PROFILE_CSCOPE_IF(enabled, color) ZoneNamedC(__tkit_perf_scope##__LINE__, color, enabled)
#    define TKIT_PROFILE_NCSCOPE_IF(enabled, name, color)                                                        \
        ZoneNamedNC(__tkit_perf_scope##__LINE__, name, color, enabled)

#    define TKIT_PROFILE_SCOPE() ZoneNamed(__tkit_perf_scope##__LINE__, true)
#    define TKIT_PROFILE_NSCOPE(name) ZoneNamedN(__tkit_perf_scope##__LINE__, name, true)
#    define TKIT_PROFILE_CSCOPE(color) ZoneNamedC(__tkit_perf_scope##__LINE__, color, true)
#    define TKIT_PROFILE_NCSCOPE(name, color) ZoneNamedNC(__tkit_perf_scope##__LINE__, name, color, true)

#    define TKIT_PROFILE_SCOPE_TEXT(text, size) ZoneTextV(__tkit_perf_scope##__LINE__, text, size)
#    define TKIT_PROFILE_SCOPE_NAME(name, size) ZoneNameV(__tkit_perf_scope##__LINE__, name, size)
#    define TKIT_PROFILE_SCOPE_COLOR(color) ZoneColorV(__tkit_perf_scope##__LINE__, color)

#    define TKIT_PROFILE_MESSAGE(message) TracyMessageL(message)
#    define TKIT_PROFILE_NMESSAGE(message, size) TracyMessage(message, size)

#    define TKIT_PROFILE_DECLARE_MUTEX(type, mutexName) TracyLockable(type, mutexName)
#    define TKIT_PROFILE_DECLARE_SHARED_MUTEX(type, mutexName) TracySharedLockable(type, mutexName)
#    define TKIT_PROFILE_MARK_LOCK(lock) LockMark(lock)

#    define TKIT_PROFILE_MARK_ALLOCATION(ptr, size) TracyAlloc(ptr, size)
#    define TKIT_PROFILE_MARK_DEALLOCATION(ptr) TracyFree(ptr)

#    define TKIT_PROFILE_MARK_POOLED_ALLOCATION(name, ptr, size) TracyAllocN(ptr, size, name)
#    define TKIT_PROFILE_MARK_POOLED_DEALLOCATION(name, ptr) TracyFreeN(ptr, name)

#    define TKIT_PROFILE_PLOT(name, value) TracyPlot(name, value)
#    define TKIT_PROFILE_PLOT_CONFIG(name, format, step, fill, color)                                        \
        TracyPlotConfig(name, format, step, fill, color)

#else

#    define TKIT_PROFILE_MARK_FRAME()
#    define TKIT_PROFILE_MARK_FRAME_START(name)
#    define TKIT_PROFILE_MARK_FRAME_END(name)
#    define TKIT_PROFILE_NMARK_FRAME(name)

#    define TKIT_PROFILE_SCOPE_IF(enabled)
#    define TKIT_PROFILE_NSCOPE_IF(enabled, name)
#    define TKIT_PROFILE_CSCOPE_IF(enabled, color)
#    define TKIT_PROFILE_NCSCOPE_IF(enabled, name, color)

#    define TKIT_PROFILE_SCOPE()
#    define TKIT_PROFILE_NSCOPE(name)
#    define TKIT_PROFILE_CSCOPE(color)
#    define TKIT_PROFILE_NCSCOPE(name, color)

#    define TKIT_PROFILE_SCOPE_TEXT(text, size)
#    define TKIT_PROFILE_SCOPE_NAME(name, size)
#    define TKIT_PROFILE_SCOPE_COLOR(color)

#    define TKIT_PROFILE_MESSAGE(message)
#    define TKIT_PROFILE_NMESSAGE(message, size)

#    define TKIT_PROFILE_DECLARE_MUTEX(type, mutexName) type mutexName
#    define TKIT_PROFILE_DECLARE_SHARED_MUTEX(type, mutexName) type mutexName

#    define TKIT_PROFILE_MARK_LOCK(lock)

#    define TKIT_PROFILE_MARK_ALLOCATION(ptr, size)
#    define TKIT_PROFILE_MARK_DEALLOCATION(ptr)

#    define TKIT_PROFILE_MARK_POOLED_ALLOCATION(name, ptr, size)
#    define TKIT_PROFILE_MARK_POOLED_DEALLOCATION(name, ptr)

#    define TKIT_PROFILE_PLOT(name, value)
#    define TKIT_PROFILE_PLOT_CONFIG(name, format, step, fill, color)

#endif
