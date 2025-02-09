#pragma once

#ifdef TKIT_ENABLE_INSTRUMENTATION

// The main reason of this wrap around the tracy library is, in case I switch to another profiling library, I can keep
// the same interface and just change the implementation of the macros.

#    include "tracy/Tracy.hpp"

#    define TKIT_PROFILE_MARK_FRAME() FrameMark
#    define TKIT_PROFILE_MARK_FRAME_START(p_Name) FrameMarkStart(p_Name)
#    define TKIT_PROFILE_MARK_FRAME_END(p_Name) FrameMarkEnd(p_Name)
#    define TKIT_PROFILE_NMARK_FRAME(p_Name) FrameMarkNamed(p_Name)

#    define TKIT_PROFILE_SCOPE_IF(p_Enabled) ZoneNamed(__tkit_perf_scope##__LINE__, p_Enabled)
#    define TKIT_PROFILE_NSCOPE_IF(p_Enabled, p_Name) ZoneNamedN(__tkit_perf_scope##__LINE__, p_Name, p_Enabled)
#    define TKIT_PROFILE_CSCOPE_IF(p_Enabled, p_Color) ZoneNamedC(__tkit_perf_scope##__LINE__, p_Color, p_Enabled)
#    define TKIT_PROFILE_NCSCOPE_IF(p_Enabled, p_Name, p_Color)                                                        \
        ZoneNamedNC(__tkit_perf_scope##__LINE__, p_Name, p_Color, p_Enabled)

#    define TKIT_PROFILE_SCOPE() ZoneNamed(__tkit_perf_scope##__LINE__, true)
#    define TKIT_PROFILE_NSCOPE(p_Name) ZoneNamedN(__tkit_perf_scope##__LINE__, p_Name, true)
#    define TKIT_PROFILE_CSCOPE(p_Color) ZoneNamedC(__tkit_perf_scope##__LINE__, p_Color, true)
#    define TKIT_PROFILE_NCSCOPE(p_Name) ZoneNamedNC(__tkit_perf_scope##__LINE__, p_Name, true)

#    define TKIT_PROFILE_SCOPE_TEXT(p_Text, p_Size) ZoneTextV(__tkit_perf_scope##__LINE__, p_Text, p_Size)
#    define TKIT_PROFILE_SCOPE_NAME(p_Name, p_Size) ZoneNameV(__tkit_perf_scope##__LINE__, p_Name, p_Size)
#    define TKIT_PROFILE_SCOPE_COLOR(p_Color) ZoneColorV(__tkit_perf_scope##__LINE__, p_Color)

#    define TKIT_PROFILE_MESSAGE(p_Message) TracyMessageL(p_Message)
#    define TKIT_PROFILE_NMESSAGE(p_Message, p_Size) TracyMessage(p_Message, p_Size)

#    define TKIT_PROFILE_DECLARE_MUTEX(p_Type, p_MutexName) TracyLockable(p_Type, p_MutexName)
#    define TKIT_PROFILE_DECLARE_SHARED_MUTEX(p_Type, p_MutexName) TracySharedLockable(p_Type, p_MutexName)
#    define TKIT_PROFILE_MARK_LOCK(p_Lock) LockMark(p_Lock)

#    define TKIT_PROFILE_MARK_ALLOCATION(p_Ptr, p_Size) TracyAlloc(p_Ptr, p_Size)
#    define TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr) TracyFree(p_Ptr)

#    define TKIT_PROFILE_MARK_POOLED_ALLOCATION(p_Name, p_Ptr, p_Size) TracyAllocN(p_Ptr, p_Size, p_Name)
#    define TKIT_PROFILE_MARK_POOLED_DEALLOCATION(p_Name, p_Ptr) TracyFreeN(p_Ptr, p_Name)

#else

#    define TKIT_PROFILE_MARK_FRAME()
#    define TKIT_PROFILE_MARK_FRAME_START(p_Name)
#    define TKIT_PROFILE_MARK_FRAME_END(p_Name)
#    define TKIT_PROFILE_NMARK_FRAME(p_Name)

#    define TKIT_PROFILE_SCOPE_IF(p_Enabled)
#    define TKIT_PROFILE_NSCOPE_IF(p_Enabled, p_Name)
#    define TKIT_PROFILE_CSCOPE_IF(p_Enabled, p_Color)
#    define TKIT_PROFILE_NCSCOPE_IF(p_Enabled, p_Name, p_Color)

#    define TKIT_PROFILE_SCOPE()
#    define TKIT_PROFILE_NSCOPE(p_Name)
#    define TKIT_PROFILE_CSCOPE(p_Color)
#    define TKIT_PROFILE_NCSCOPE(p_Name)

#    define TKIT_PROFILE_SCOPE_TEXT(p_Text, p_Size)
#    define TKIT_PROFILE_SCOPE_NAME(p_Name, p_Size)
#    define TKIT_PROFILE_SCOPE_COLOR(p_Color)

#    define TKIT_PROFILE_MESSAGE(p_Message)
#    define TKIT_PROFILE_NMESSAGE(p_Message, p_Size)

#    define TKIT_PROFILE_DECLARE_MUTEX(p_Type, p_MutexName) p_Type p_MutexName
#    define TKIT_PROFILE_DECLARE_SHARED_MUTEX(p_Type, p_MutexName) p_Type p_MutexName

#    define TKIT_PROFILE_MARK_LOCK(p_Lock)

#    define TKIT_PROFILE_MARK_ALLOCATION(p_Ptr, p_Size)
#    define TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr)

#    define TKIT_PROFILE_MARK_POOLED_ALLOCATION(p_Name, p_Ptr, p_Size)
#    define TKIT_PROFILE_MARK_POOLED_DEALLOCATION(p_Name, p_Ptr)

#endif