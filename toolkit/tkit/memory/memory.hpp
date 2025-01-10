#pragma once

#include "tkit/core/api.hpp"
#include "tkit/core/alias.hpp"
#include <functional>

namespace TKit
{
// Could abstract this in some way (by defining those as function pointers) to allow for custom allocators
// For now, I'll leave it as it is

/**
 * @brief Allocate a chunk of memory of a given size.
 *
 * Uses default `::operator new()`. It is here as a placeholder for future custom global allocators.
 *
 * @param p_Size The size of the memory to allocate.
 * @return A pointer to the allocated memory.
 */
TKIT_API void *Allocate(size_t p_Size) noexcept;

/**
 * @brief Deallocate a chunk of memory
 *
 * Uses default `::operator delete()`. It is here as a placeholder for future custom global allocators.
 *
 * @param p_Ptr A pointer to the memory to deallocate.
 */
TKIT_API void Deallocate(void *p_Ptr) noexcept;

/**
 * @brief Allocate a chunk of memory of a given size with a given alignment.
 *
 * Uses default `::operator new()`. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param p_Size The size of the memory to allocate.
 * @param p_Alignment The alignment of the memory to allocate.
 * @return A pointer to the allocated memory.
 */
TKIT_API void *AllocateAligned(size_t p_Size, std::align_val_t p_Alignment) noexcept;

/**
 * @brief Deallocate a chunk of memory with a given alignment.
 *
 * Uses default `::operator delete()`. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param p_Ptr A pointer to the memory to deallocate.
 * @param p_Alignment The alignment of the memory to deallocate.
 */
TKIT_API void DeallocateAligned(void *p_Ptr, std::align_val_t p_Alignment) noexcept;

/**
 * @brief Allocate a chunk of memory of a given size with a given alignment.
 *
 * Uses the default platform-specific aligned allocation. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param p_Size The size of the memory to allocate.
 * @param p_Alignment The alignment of the memory to allocate.
 * @return A pointer to the allocated memory.
 */
TKIT_API void *AllocateAlignedPlatformSpecific(size_t p_Size, size_t p_Alignment) noexcept;

/**
 * @brief Deallocate a chunk of memory. Uses default platform-specific aligned deallocation.
 *
 * Uses the default platform-specific aligned allocation. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param p_Ptr A pointer to the memory to deallocate.
 */
TKIT_API void DeallocateAlignedPlatformSpecific(void *p_Ptr) noexcept;

template <typename T> class DefaultAllocator
{
  public:
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using size_type = usize;
    using difference_type = idiff;

    template <typename U> struct rebind
    {
        using other = DefaultAllocator<U>;
    };

    DefaultAllocator() noexcept = default;

    template <typename U> DefaultAllocator(const DefaultAllocator<U> &) noexcept
    {
    }

    pointer allocate(size_type p_N)
    {
        return static_cast<pointer>(Allocate(p_N * sizeof(T)));
    }

    void deallocate(pointer p_Ptr, size_type)
    {
        Deallocate(p_Ptr);
    }

    bool operator==(const DefaultAllocator &) const noexcept
    {
        return true;
    }
    bool operator!=(const DefaultAllocator &) const noexcept
    {
        return false;
    }
};

} // namespace TKit

#ifndef TKIT_DISABLE_MEMORY_OVERRIDES

TKIT_API void *operator new(size_t p_Size);
TKIT_API void *operator new[](size_t p_Size);
TKIT_API void *operator new(size_t p_Size, std::align_val_t p_Alignment);
TKIT_API void *operator new[](size_t p_Size, std::align_val_t p_Alignment);
TKIT_API void *operator new(size_t p_Size, const std::nothrow_t &) noexcept;
TKIT_API void *operator new[](size_t p_Size, const std::nothrow_t &) noexcept;

TKIT_API void operator delete(void *p_Ptr) noexcept;
TKIT_API void operator delete[](void *p_Ptr) noexcept;
TKIT_API void operator delete(void *p_Ptr, std::align_val_t p_Alignment) noexcept;
TKIT_API void operator delete[](void *p_Ptr, std::align_val_t p_Alignment) noexcept;
TKIT_API void operator delete(void *p_Ptr, const std::nothrow_t &) noexcept;
TKIT_API void operator delete[](void *p_Ptr, const std::nothrow_t &) noexcept;

TKIT_API void operator delete(void *p_Ptr, size_t p_Size) noexcept;
TKIT_API void operator delete[](void *p_Ptr, size_t p_Size) noexcept;
TKIT_API void operator delete(void *p_Ptr, size_t p_Size, std::align_val_t p_Alignment) noexcept;
TKIT_API void operator delete[](void *p_Ptr, size_t p_Size, std::align_val_t p_Alignment) noexcept;
TKIT_API void operator delete(void *p_Ptr, size_t p_Size, const std::nothrow_t &) noexcept;
TKIT_API void operator delete[](void *p_Ptr, size_t p_Size, const std::nothrow_t &) noexcept;

#endif
