#include "tkit/core/pch.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/core/logging.hpp"
#include "tkit/profiling/macros.hpp"

namespace TKit
{
void *Allocate(const size_t p_Size) noexcept
{
    void *ptr = ::operator new(p_Size);
    TKIT_ASSERT(ptr, "[TOOLKIT] Failed to allocate memory with size {}", p_Size);
    return ptr;
}

void Deallocate(void *p_Ptr) noexcept
{
    ::operator delete(p_Ptr);
}

void *AllocateAligned(const size_t p_Size, const std::align_val_t p_Alignment) noexcept
{
    void *ptr = ::operator new(p_Size, p_Alignment);
    TKIT_ASSERT(ptr, "[TOOLKIT] Failed to allocate memory with size {} and alignment {}", p_Size,
                static_cast<size_t>(p_Alignment));
    return ptr;
}

void DeallocateAligned(void *p_Ptr, const std::align_val_t p_Alignment) noexcept
{
    ::operator delete(p_Ptr, p_Alignment);
}

void *AllocateAlignedPlatformSpecific(const size_t p_Size, const size_t p_Alignment) noexcept
{
    void *ptr = nullptr;
#ifdef TKIT_OS_WINDOWS
    ptr = _aligned_malloc(p_Size, p_Alignment);
    TKIT_ASSERT(ptr, "[TOOLKIT] Failed to allocate memory with size {} and alignment {}", p_Size, p_Alignment);
#else
    TKIT_ASSERT_RETURNS(posix_memalign(&ptr, p_Alignment, p_Size), 0,
                        "[TOOLKIT] Failed to allocate memory with size {} and alignment {}", p_Size, p_Alignment);

#endif
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void DeallocateAlignedPlatformSpecific(void *p_Ptr) noexcept
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
#ifdef TKIT_OS_WINDOWS
    _aligned_free(p_Ptr);
#else
    std::free(p_Ptr);
#endif
}
} // namespace TKit

#ifndef TKIT_DISABLE_MEMORY_OVERRIDES
void *operator new(const size_t p_Size)
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}
void *operator new[](const size_t p_Size)
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}
void *operator new(const size_t p_Size, const std::align_val_t p_Alignment)
{
    return TKit::AllocateAlignedPlatformSpecific(p_Size, static_cast<size_t>(p_Alignment));
}
void *operator new[](const size_t p_Size, const std::align_val_t p_Alignment)
{
    return TKit::AllocateAlignedPlatformSpecific(p_Size, static_cast<size_t>(p_Alignment));
}
void *operator new(const size_t p_Size, const std::nothrow_t &) noexcept
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}
void *operator new[](const size_t p_Size, const std::nothrow_t &) noexcept
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void operator delete(void *p_Ptr) noexcept
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
    std::free(p_Ptr);
}
void operator delete[](void *p_Ptr) noexcept
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
    std::free(p_Ptr);
}
void operator delete(void *p_Ptr, const std::align_val_t) noexcept
{
    TKit::DeallocateAlignedPlatformSpecific(p_Ptr);
}
void operator delete[](void *p_Ptr, const std::align_val_t) noexcept
{
    TKit::DeallocateAlignedPlatformSpecific(p_Ptr);
}
void operator delete(void *p_Ptr, const std::nothrow_t &) noexcept
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
    std::free(p_Ptr);
}
void operator delete[](void *p_Ptr, const std::nothrow_t &) noexcept
{
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
    std::free(p_Ptr);
}

#endif
