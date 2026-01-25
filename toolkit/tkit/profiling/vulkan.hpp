#pragma once

#ifdef TKIT_ENABLE_VULKAN_INSTRUMENTATION
#    include <tracy/TracyVulkan.hpp>

namespace TKit
{
using VkProfilingContext = TracyVkCtx;
} // namespace TKit

// Symbol table is missing for now
#    define TKIT_PROFILE_CREATE_VULKAN_CONTEXT(instance, physdev, device, queue, cmdbuf, getInstance,      \
                                               getDevice)                                                            \
        TracyVkContext(instance, physdev, device, queue, cmdbuf, getInstance, getDevice)
#    define TKIT_PROFILE_CREATE_VULKAN_CALIBRATED_CONTEXT(instance, physdev, device, queue, cmdbuf,          \
                                                          gpdctd, gct, getInstance, getDevice)                 \
        TracyVkContextCalibrated(instance, physdev, device, queue, cmdbuf, gpdctd, gct, getInstance,   \
                                 getDevice)

#    define TKIT_PROFILE_DESTROY_VULKAN_CONTEXT(context) TracyVkDestroy(context)

#    define TKIT_PROFILE_VULKAN_SCOPE_IF(active, name, context, cmdbuf)                                        \
        TracyVkNamedZone(context, __tkit_gpu_scope##__LINE__, cmdbuf, name, active)
#    define TKIT_PROFILE_VULKAN_CSCOPE_IF(active, name, color, context, cmdbuf)                              \
        TracyVkNamedZoneC(context, __tkit_gpu_scope##__LINE__, cmdbuf, name, color, active)

#    define TKIT_PROFILE_VULKAN_SCOPE(name, context, cmdbuf)                                                     \
        TracyVkNamedZone(context, __tkit_gpu_scope##__LINE__, cmdbuf, name, true)
#    define TKIT_PROFILE_VULKAN_CSCOPE(name, color, context, cmdbuf)                                           \
        TracyVkNamedZoneC(context, __tkit_gpu_scope##__LINE__, cmdbuf, name, color, true)

#    define TKIT_PROFILE_VULKAN_COLLECT(context, cmdbuf) TracyVkCollect(context, cmdbuf)

#else

#    define TKIT_PROFILE_CREATE_VULKAN_CONTEXT(instance, physdev, device, queue, cmdbuf, instanceProcAddr, \
                                               deviceProcAddr)                                                       \
        nullptr
#    define TKIT_PROFILE_CREATE_VULKAN_CALIBRATED_CONTEXT(physdev, device, queue, cmdbuf, gpdctd, gct)

#    define TKIT_PROFILE_DESTROY_VULKAN_CONTEXT(context)

#    define TKIT_PROFILE_VULKAN_SCOPE_IF(active, name, context, cmdbuf)
#    define TKIT_PROFILE_VULKAN_CSCOPE_IF(active, name, color, context, cmdbuf)

#    define TKIT_PROFILE_VULKAN_SCOPE(name, context, cmdbuf)
#    define TKIT_PROFILE_VULKAN_CSCOPE(name, color, context, cmdbuf)

#    define TKIT_PROFILE_VULKAN_COLLECT(context, cmdbuf)

#endif
