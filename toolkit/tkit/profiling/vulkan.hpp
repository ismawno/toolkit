#pragma once

#ifdef TKIT_ENABLE_VULKAN_INSTRUMENTATION
#    include <tracy/TracyVulkan.hpp>

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

#    define TKIT_PROFILE_VULKAN_SCOPE_IF(p_Active, p_Name, p_Context, p_Cmdbuf)                                        \
        TracyVkNamedZone(p_Context, __tkit_gpu_scope##__LINE__, p_Cmdbuf, p_Name, p_Active)
#    define TKIT_PROFILE_VULKAN_CSCOPE_IF(p_Active, p_Name, p_Color, p_Context, p_Cmdbuf)                              \
        TracyVkNamedZoneC(p_Context, __tkit_gpu_scope##__LINE__, p_Cmdbuf, p_Name, p_Color, p_Active)

#    define TKIT_PROFILE_VULKAN_SCOPE(p_Name, p_Context, p_Cmdbuf)                                                     \
        TracyVkNamedZone(p_Context, __tkit_gpu_scope##__LINE__, p_Cmdbuf, p_Name, true)
#    define TKIT_PROFILE_VULKAN_CSCOPE(p_Name, p_Color, p_Context, p_Cmdbuf)                                           \
        TracyVkNamedZoneC(p_Context, __tkit_gpu_scope##__LINE__, p_Cmdbuf, p_Name, p_Color, true)

#    define TKIT_PROFILE_VULKAN_COLLECT(p_Context, p_Cmdbuf) TracyVkCollect(p_Context, p_Cmdbuf)

#else

#    define TKIT_PROFILE_CREATE_VULKAN_CONTEXT(p_Instance, p_Physdev, p_Device, p_Queue, p_Cmdbuf, p_InstanceProcAddr, \
                                               p_DeviceProcAddr)                                                       \
        nullptr
#    define TKIT_PROFILE_CREATE_VULKAN_CALIBRATED_CONTEXT(p_Physdev, p_Device, p_Queue, p_Cmdbuf, p_Gpdctd, p_Gct)

#    define TKIT_PROFILE_DESTROY_VULKAN_CONTEXT(p_Context)

#    define TKIT_PROFILE_VULKAN_SCOPE_IF(p_Active, p_Name, p_Context, p_Cmdbuf)
#    define TKIT_PROFILE_VULKAN_CSCOPE_IF(p_Active, p_Name, p_Color, p_Context, p_Cmdbuf)

#    define TKIT_PROFILE_VULKAN_SCOPE(p_Name, p_Context, p_Cmdbuf)
#    define TKIT_PROFILE_VULKAN_CSCOPE(p_Name, p_Color, p_Context, p_Cmdbuf)

#    define TKIT_PROFILE_VULKAN_COLLECT(p_Context, p_Cmdbuf)

#endif