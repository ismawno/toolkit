#pragma once

#ifdef KIT_ENABLE_PROFILING

// The main reason of this wrap around the tracy library is, in case I switch to another profiling library, I can keep
// the same interface and just change the implementation of the macros.

#    include "tracy/Tracy.hpp"

#    define KIT_PROFILE_MARK_FRAME FrameMark
#    define KIT_PROFILE_MARK_FRAME_START(p_Name) FrameMarkStart(p_Name)
#    define KIT_PROFILE_MARK_FRAME_END(p_Name) FrameMarkEnd(p_Name)
#    define KIT_PROFILE_NMARK_FRAME(p_Name) FrameMarkNamed(p_Name)

#    define KIT_PROFILE_SCOPE ZoneScoped
#    define KIT_PROFILE_NSCOPE(p_Name) ZoneScopedN(p_Name)
#    define KIT_PROFILE_CSCOPE(p_Color) ZoneScopedC(p_Color)
#    define KIT_PROFILE_NCSCOPE(p_Name, p_Color) ZoneScopedNC(p_Name, p_Color)
#    define KIT_PROFILE_SCOPE_TEXT(p_Text, p_Size) ZoneText(p_Text, p_Size)
#    define KIT_PROFILE_SCOPE_NAME(p_Name, p_Size) ZoneName(p_Name, p_Size)
#    define KIT_PROFILE_SCOPE_COLOR(p_Color) ZoneColor(p_Color)

#    define KIT_PROFILE_NAMED_SCOPE(p_ScopeName, p_Enabled) ZoneNamed(p_ScopeName)
#    define KIT_PROFILE_NAMED_NSCOPE(p_ScopeName, p_Name, p_Enabled) ZoneNamedN(p_ScopeName, p_Name, p_Enabled)
#    define KIT_PROFILE_NAMED_CSCOPE(p_ScopeName, p_Color, p_Enabled) ZoneNamedC(p_ScopeName, p_Color, p_Enabled)
#    define KIT_PROFILE_NAMED_NCSCOPE(p_ScopeName, p_Name, p_Color, p_Enabled)                                         \
        ZoneNamedNC(p_ScopeName, p_Name, p_Color, p_Enabled)
#    define KIT_PROFILE_NAMED_SCOPE_TEXT(p_ScopeName, p_Text, p_Size) ZoneTextV(p_ScopeName, p_Text, p_Size)
#    define KIT_PROFILE_NAMED_SCOPE_NAME(p_ScopeName, p_Name, p_Size) ZoneNameV(p_ScopeName, p_Name, p_Size)
#    define KIT_PROFILE_NAMED_SCOPE_COLOR(p_ScopeName, p_Color) ZoneColorV(p_ScopeName, p_Color)

#    define KIT_PROFILE_MESSAGE(p_Message) TracyMessageL(p_Message)
#    define KIT_PROFILE_NMESSAGE(p_Message, p_Size) TracyMessage(p_Message, p_Size)

#    define KIT_PROFILE_DECLARE_MUTEX(p_Type, p_MutexName) TracyLockable(p_Type, p_MutexName)
#    define KIT_PROFILE_DECLARE_SHARED_MUTEX(p_Type, p_MutexName) TracySharedLockable(p_Type, p_MutexName)
#    define KIT_PROFILE_MARK_LOCK(p_Lock) LockMark(p_Lock)

#    define KIT_PROFILE_MARK_ALLOCATION(p_Ptr, p_Size) TracyAlloc(p_Ptr, p_Size)
#    define KIT_PROFILE_MARK_DEALLOCATION(p_Ptr) TracyFree(p_Ptr)

#    define KIT_PROFILE_MARK_POOLED_ALLOCATION(p_Name, p_Ptr, p_Size) TracyAllocN(p_Ptr, p_Size, p_Name)
#    define KIT_PROFILE_MARK_POOLED_DEALLOCATION(p_Name, p_Ptr) TracyFreeN(p_Ptr, p_Name)

#else

#    define KIT_PROFILE_MARK_FRAME
#    define KIT_PROFILE_MARK_FRAME_START(p_Name)
#    define KIT_PROFILE_MARK_FRAME_END(p_Name)
#    define KIT_PROFILE_NMARK_FRAME(p_Name)

#    define KIT_PROFILE_SCOPE
#    define KIT_PROFILE_NSCOPE(p_Name)
#    define KIT_PROFILE_CSCOPE(p_Color)
#    define KIT_PROFILE_NCSCOPE(p_Name)
#    define KIT_PROFILE_SCOPE_TEXT(p_Text, p_Size)
#    define KIT_PROFILE_SCOPE_NAME(p_Name, p_Size)
#    define KIT_PROFILE_SCOPE_COLOR(p_Color)

#    define KIT_PROFILE_NAMED_SCOPE(p_ScopeName, p_Enabled)
#    define KIT_PROFILE_NAMED_NSCOPE(p_ScopeName, p_Name, p_Enabled)
#    define KIT_PROFILE_NAMED_CSCOPE(p_ScopeName, p_Color, p_Enabled)
#    define KIT_PROFILE_NAMED_NCSCOPE(p_ScopeName, p_Name, p_Color, p_Enabled)
#    define KIT_PROFILE_NAMED_SCOPE_TEXT(p_ScopeName, p_Text, p_Size)
#    define KIT_PROFILE_NAMED_SCOPE_NAME(p_ScopeName, p_Name, p_Size)
#    define KIT_PROFILE_NAMED_SCOPE_COLOR(p_ScopeName, p_Color)

#    define KIT_PROFILE_MESSAGE(p_Message)
#    define KIT_PROFILE_NMESSAGE(p_Message, p_Size)

#    define KIT_PROFILE_DECLARE_MUTEX(p_Type, p_MutexName) p_Type p_MutexName
#    define KIT_PROFILE_DECLARE_SHARED_MUTEX(p_Type, p_MutexName) p_Type p_MutexName

#    define KIT_PROFILE_MARK_LOCK(p_Lock)

#    define KIT_PROFILE_MARK_ALLOCATION(p_Ptr, p_Size)
#    define KIT_PROFILE_MARK_DEALLOCATION(p_Ptr)

#    define KIT_PROFILE_MARK_POOLED_ALLOCATION(p_Name, p_Ptr, p_Size)
#    define KIT_PROFILE_MARK_POOLED_DEALLOCATION(p_Name, p_Ptr)

#endif