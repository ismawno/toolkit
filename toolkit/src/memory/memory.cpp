#include "core/pch.hpp"
#include "kit/memory/memory.hpp"
#include "kit/core/logging.hpp"

namespace KIT
{
void *Allocate(const usize p_Size) noexcept
{
    void *ptr = std::malloc(p_Size);
    KIT_ASSERT(ptr, "Failed to allocate memory with size {}", p_Size);
    return ptr;
}

void Deallocate(void *p_Ptr) noexcept
{
    KIT_ASSERT(p_Ptr, "Trying to deallocate a nullptr");
    std::free(p_Ptr);
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
}
} // namespace KIT