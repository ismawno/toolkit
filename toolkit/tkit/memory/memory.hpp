#pragma once

#include "tkit/core/api.hpp"
#include "tkit/core/alias.hpp"
#include <functional>

namespace TKit::Memory
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

/**
 * @brief A custom allocator that uses a custom size_type (usually u32) for indexing.
 *
 * This allocator is intended for environments or applications where the maximum container size never exceeds 2^32,
 * making 32-bit indices (u32) sufficient. By using a smaller index type, it can offer performance benefits in tight
 * loops and reduce cache pressure, particularly when managing a large number of small containers or indices.
 *
 * @tparam T The type of object to allocate.
 *
 * @note
 * - This allocator is stateless and always compares equal.
 *
 * - It does not provide `construct()` or `destroy()` methods explicitly
 *   because, in C++17 and later, `std::allocator_traits` can handle
 *   construction and destruction via placement-new and direct destructor calls.
 */
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

} // namespace TKit::Memory

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
