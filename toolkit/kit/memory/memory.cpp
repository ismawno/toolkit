#include "kit/core/pch.hpp"
#include "kit/memory/memory.hpp"
#include "kit/core/logging.hpp"
#include "kit/profiling/macros.hpp"

namespace KIT
{
void *Allocate(const usize p_Size) noexcept
{
    void *ptr = std::malloc(p_Size);
    KIT_ASSERT(ptr, "Failed to allocate memory with size {}", p_Size);
    KIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void Deallocate(void *p_Ptr) noexcept
{
    KIT_ASSERT(p_Ptr, "Trying to deallocate a nullptr");
    std::free(p_Ptr);
    KIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
}

void *AllocateAligned(const usize p_Size, const usize p_Alignment) noexcept
{
    KIT_ASSERT(p_Alignment >= sizeof(void *), "Alignment must be at least the size of a pointer");
    void *ptr = nullptr;
#ifdef KIT_OS_WINDOWS
    ptr = _aligned_malloc(p_Size, p_Alignment);
    KIT_ASSERT(ptr, "Failed to allocate memory with size {} and alignment {}", p_Size, p_Alignment);
#else
    // Add here warning suppression if needed
    KIT_WARNING_IGNORE_PUSH
    KIT_GCC_WARNING_IGNORE("-Wunused-variable")
    KIT_CLANG_WARNING_IGNORE("-Wunused-variable")
    int result = posix_memalign(&ptr, p_Alignment, p_Size);
    KIT_WARNING_IGNORE_POP
    KIT_ASSERT(result == 0, "Failed to allocate aligned memory with size {} and alignment {}. Result: {}", p_Size,
               p_Alignment, result);
#endif
    KIT_PROFILE_MARK_ALLOCATION(ptr, p_Size);
    return ptr;
}

void DeallocateAligned(void *p_Ptr) noexcept
{
    KIT_ASSERT(p_Ptr, "Trying to deallocate a nullptr");
#ifdef KIT_OS_WINDOWS
    _aligned_free(p_Ptr);
#else
    std::free(p_Ptr);
#endif
    KIT_PROFILE_MARK_DEALLOCATION(p_Ptr);
}
} // namespace KIT

#ifdef KIT_ENABLE_PROFILING

void *operator new(const KIT::usize p_Size)
{
    return KIT::Allocate(p_Size);
}
void *operator new[](const KIT::usize p_Size)
{
    return KIT::Allocate(p_Size);
}

void operator delete(void *p_Ptr) noexcept
{
    KIT::Deallocate(p_Ptr);
}
void operator delete[](void *p_Ptr) noexcept
{
    KIT::Deallocate(p_Ptr);
}

void *operator new(const KIT::usize p_Size, const std::nothrow_t &) noexcept
{
    return KIT::Allocate(p_Size);
}
void *operator new[](const KIT::usize p_Size, const std::nothrow_t &) noexcept
{
    return KIT::Allocate(p_Size);
}

void operator delete(void *p_Ptr, const std::nothrow_t &) noexcept
{
    KIT::Deallocate(p_Ptr);
}
void operator delete[](void *p_Ptr, const std::nothrow_t &) noexcept
{
    KIT::Deallocate(p_Ptr);
}

#endif