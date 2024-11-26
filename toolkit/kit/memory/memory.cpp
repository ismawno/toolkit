#include "kit/core/pch.hpp"
#include "kit/memory/memory.hpp"
#include "kit/core/logging.hpp"
#include "kit/profiling/macros.hpp"

namespace TKit
{
void *Allocate(const usize p_Size) noexcept
{
    void *ptr = std::malloc(p_Size);
    TKIT_ASSERT(ptr, "Failed to allocate memory with size {}", p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void Deallocate(void *p_Ptr) noexcept
{
    TKIT_ASSERT(p_Ptr, "Trying to deallocate a nullptr");
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
    std::free(p_Ptr);
}

void *AllocateAligned(const usize p_Size, const usize p_Alignment) noexcept
{
    TKIT_ASSERT(p_Alignment >= sizeof(void *), "Alignment must be at least the size of a pointer");
    void *ptr = nullptr;
#ifdef TKIT_OS_WINDOWS
    ptr = _aligned_malloc(p_Size, p_Alignment);
    TKIT_ASSERT(ptr, "Failed to allocate memory with size {} and alignment {}", p_Size, p_Alignment);
#else
    // Add here warning suppression if needed
    TKIT_WARNING_IGNORE_PUSH
    TKIT_GCC_WARNING_IGNORE("-Wunused-variable")
    TKIT_CLANG_WARNING_IGNORE("-Wunused-variable")
    int result = posix_memalign(&ptr, p_Alignment, p_Size);
    TKIT_WARNING_IGNORE_POP
    TKIT_ASSERT(result == 0, "Failed to allocate aligned memory with size {} and alignment {}. Result: {}", p_Size,
                p_Alignment, result);
#endif
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void DeallocateAligned(void *p_Ptr) noexcept
{
    TKIT_ASSERT(p_Ptr, "Trying to deallocate a nullptr");
    TKIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
#ifdef TKIT_OS_WINDOWS
    _aligned_free(p_Ptr);
#else
    std::free(p_Ptr);
#endif
}
} // namespace TKit

#ifdef TKIT_ENABLE_PROFILING

void *operator new(const TKit::usize p_Size)
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}
void *operator new[](const TKit::usize p_Size)
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

void *operator new(const TKit::usize p_Size, const std::nothrow_t &) noexcept
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}
void *operator new[](const TKit::usize p_Size, const std::nothrow_t &) noexcept
{
    void *ptr = std::malloc(p_Size);
    TKIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
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