#pragma once

#ifdef KIT_ENABLE_VULKAN_PROFILING
#    include "tracy/TracyVulkan.hpp"

namespace KIT
{
using VkProfilingContext = TracyVkCtx;
}

// Calibrated is missing for now
#    ifdef TRACY_VK_USE_SYMBOL_TABLE
#        define KIT_PROFILE_CREATE_VULKAN_CONTEXT(p_Instance, p_Physdev, p_Device, p_Queue, p_Cmdbuf,                  \
                                                  p_InstanceProcAddr, deviceProcAddr)                                  \
            TracyVkContext(p_Instance, p_Physdev, p_Device, p_Queue, p_Cmdbuf, p_InstanceProcAddr, p_DeviceProcAddr)
#    else
#        define KIT_PROFILE_CREATE_VULKAN_CONTEXT(p_Physdev, p_Device, p_Queue, p_Cmdbuf)                              \
            TracyVkContext(p_Physdev, p_Device, p_Queue, p_Cmdbuf)
#    endif

#    define KIT_PROFILE_DESTROY_VULKAN_CONTEXT(p_Context) TracyVkDestroy(p_Context)

#    define KIT_PROFILE_VULKAN_SCOPE(p_Name, p_Context, p_Cmdbuf) TracyVkZone(p_Context, p_Cmdbuf, p_Name)
#    define KIT_PROFILE_VULKAN_CSCOPE(p_Name, p_Color, p_Context, p_Cmdbuf)                                            \
        TracyVkZoneC(p_Context, p_Cmdbuf, p_Name, p_Color)

#    define KIT_PROFILE_VULKAN_NAMED_SCOPE(p_ScopeName, p_Name, p_Context, p_Cmdbuf, p_Active)                         \
        TracyVkNamedZone(p_Context, p_ScopeName, p_Cmdbuf, p_Name, p_Active)
#    define KIT_PROFILE_VULKAN_NAMED_CSCOPE(p_ScopeName, p_Name, p_Color, p_Context, p_Cmdbuf, p_Active)               \
        TracyVkNamedZoneC(p_Context, p_ScopeName, p_Cmdbuf, p_Name, p_Color, p_Active)

#    define KIT_PROFILE_VULKAN_COLLECT(p_Context, p_Cmdbuf) TracyVkCollect(p_Context, p_Cmdbuf)

#else

#    define KIT_PROFILE_CREATE_VULKAN_CONTEXT(p_Instance, p_Physdev, p_Device, p_Queue, p_Cmdbuf, p_InstanceProcAddr,  \
                                              p_DeviceProcAddr)                                                        \
        nullptr
#    define KIT_PROFILE_DESTROY_VULKAN_CONTEXT(p_Context)

#    define KIT_PROFILE_VULKAN_SCOPE(p_Name, p_Context, p_Cmdbuf)
#    define KIT_PROFILE_VULKAN_CSCOPE(p_Name, p_Color, p_Context, p_Cmdbuf)

#    define KIT_PROFILE_VULKAN_NAMED_SCOPE(p_ScopeName, p_Name, p_Context, p_Cmdbuf, p_Active)
#    define KIT_PROFILE_VULKAN_NAMED_CSCOPE(p_ScopeName, p_Name, p_Color, p_Context, p_Cmdbuf, p_Active)

#    define KIT_PROFILE_VULKAN_COLLECT(p_Context, p_Cmdbuf)

#endif