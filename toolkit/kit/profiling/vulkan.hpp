#pragma once

#ifdef TKIT_ENABLE_VULKAN_PROFILING
#    include "tracy/TracyVulkan.hpp"

namespace TKit
{
using VkProfilingContext = TracyVkCtx;
} // namespace TKit

// Symbol table is missing for now
#    define TKIT_PROFILE_CREATE_VULKAN_CONTEXT(p_Physdev, p_Device, p_Queue, p_Cmdbuf)                                 \
        TracyVkContext(p_Physdev, p_Device, p_Queue, p_Cmdbuf)
#    define TKIT_PROFILE_CREATE_VULKAN_CALIBRATED_CONTEXT(p_Physdev, p_Device, p_Queue, p_Cmdbuf, p_Gpdctd, p_Gct)     \
        TracyVkContextCalibrated(p_Physdev, p_Device, p_Queue, p_Cmdbuf, p_Gpdctd, p_Gct)

#    define TKIT_PROFILE_DESTROY_VULKAN_CONTEXT(p_Context) TracyVkDestroy(p_Context)

#    define TKIT_PROFILE_VULKAN_SCOPE(p_Name, p_Context, p_Cmdbuf) TracyVkZone(p_Context, p_Cmdbuf, p_Name)
#    define TKIT_PROFILE_VULKAN_CSCOPE(p_Name, p_Color, p_Context, p_Cmdbuf)                                           \
        TracyVkZoneC(p_Context, p_Cmdbuf, p_Name, p_Color)

#    define TKIT_PROFILE_VULKAN_NAMED_SCOPE(p_ScopeName, p_Name, p_Context, p_Cmdbuf, p_Active)                        \
        TracyVkNamedZone(p_Context, p_ScopeName, p_Cmdbuf, p_Name, p_Active)
#    define TKIT_PROFILE_VULKAN_NAMED_CSCOPE(p_ScopeName, p_Name, p_Color, p_Context, p_Cmdbuf, p_Active)              \
        TracyVkNamedZoneC(p_Context, p_ScopeName, p_Cmdbuf, p_Name, p_Color, p_Active)

#    define TKIT_PROFILE_VULKAN_COLLECT(p_Context, p_Cmdbuf) TracyVkCollect(p_Context, p_Cmdbuf)

#else

#    define TKIT_PROFILE_CREATE_VULKAN_CONTEXT(p_Instance, p_Physdev, p_Device, p_Queue, p_Cmdbuf, p_InstanceProcAddr, \
                                               p_DeviceProcAddr)                                                       \
        nullptr
#    define TKIT_PROFILE_DESTROY_VULKAN_CONTEXT(p_Context)

#    define TKIT_PROFILE_VULKAN_SCOPE(p_Name, p_Context, p_Cmdbuf)
#    define TKIT_PROFILE_VULKAN_CSCOPE(p_Name, p_Color, p_Context, p_Cmdbuf)

#    define TKIT_PROFILE_VULKAN_NAMED_SCOPE(p_ScopeName, p_Name, p_Context, p_Cmdbuf, p_Active)
#    define TKIT_PROFILE_VULKAN_NAMED_CSCOPE(p_ScopeName, p_Name, p_Color, p_Context, p_Cmdbuf, p_Active)

#    define TKIT_PROFILE_VULKAN_COLLECT(p_Context, p_Cmdbuf)

#endif