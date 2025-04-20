#pragma once

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/alias.hpp"
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
 * Uses the default platform-specific aligned allocation. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param p_Size The size of the memory to allocate.
 * @param p_Alignment The alignment of the memory to allocate.
 * @return A pointer to the allocated memory.
 */
TKIT_API void *AllocateAligned(size_t p_Size, size_t p_Alignment) noexcept;

/**
 * @brief Deallocate a chunk of memory with a given alignment.
 *
 * Uses the default platform-specific aligned deallocation. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param p_Ptr A pointer to the memory to deallocate.
 * @param p_Alignment The alignment of the memory to deallocate.
 */
TKIT_API void DeallocateAligned(void *p_Ptr) noexcept;

/**
 * @brief Copy a chunk of memory from one location to another.
 *
 * Uses the default `::memcpy()`. It is here as a placeholder for future custom global memory management.
 *
 * @param p_Dst A pointer to the destination memory.
 * @param p_Src A pointer to the source memory.
 * @param p_Size The size of the memory to copy, in bytes.
 */
TKIT_API void *ForwardCopy(void *p_Dst, const void *p_Src, size_t p_Size) noexcept;

/**
 * @brief Copy a chunk of memory from one location to another in reverse order.
 *
 * Uses the default `::memmove()`. It is here as a placeholder for future custom global memory management.
 *
 * @param p_Dst A pointer to the destination memory.
 * @param p_Src A pointer to the source memory.
 * @param p_Size The size of the memory to copy, in bytes.
 */
TKIT_API void *BackwardCopy(void *p_Dst, const void *p_Src, size_t p_Size) noexcept;

/**
 * @brief Copy a range of elements from one iterator to another.
 *
 * Uses the default `std::copy()`. It is here as a placeholder for future custom global memory management.
 *
 * @param p_Dst An iterator pointing to the destination memory.
 * @param p_Begin An iterator pointing to the beginning of the source memory.
 * @param p_End An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto ForwardCopy(It1 p_Dst, It2 p_Begin, It2 p_End) noexcept
{
    return std::copy(p_Begin, p_End, p_Dst);
}

/**
 * @brief Copy a range of elements from one iterator to another in reverse order.
 *
 * Uses the default `std::copy_backward()`. It is here as a placeholder for future custom global memory management.
 *
 * @param p_Dst An iterator pointing to the destination memory.
 * @param p_Begin An iterator pointing to the beginning of the source memory.
 * @param p_End An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto BackwardCopy(It1 p_Dst, It2 p_Begin, It2 p_End) noexcept
{
    return std::copy_backward(p_Begin, p_End, p_Dst);
}

/**
 * @brief Move a chunk of memory from one location to another.
 *
 * Uses the default `std::move()`. It is here as a placeholder for future custom global memory management.
 *
 * @param p_Dst An iterator pointing to the destination memory.
 * @param p_Begin An iterator pointing to the beginning of the source memory.
 * @param p_End An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto ForwardMove(It1 p_Dst, It2 p_Begin, It2 p_End) noexcept
{
    return std::move(p_Begin, p_End, p_Dst);
}

/**
 * @brief Move a chunk of memory from one location to another in reverse order.
 *
 * Uses the default `std::move_backward()`. It is here as a placeholder for future custom global memory management.
 *
 * @param p_Dst An iterator pointing to the destination memory.
 * @param p_Begin An iterator pointing to the beginning of the source memory.
 * @param p_End An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto BackwardMove(It1 p_Dst, It2 p_Begin, It2 p_End) noexcept
{
    return std::move_backward(p_Begin, p_End, p_Dst);
}

/**
 * @brief A custom allocator that uses a custom size_type (usually `u32`) for indexing, compatible with STL.
 *
 * This allocator is intended for environments or applications where the maximum container size never exceeds 2^32,
 * making 32-bit indices (`u32`) sufficient. By using a smaller index type, it can offer performance benefits in tight
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
template <typename T> class STLAllocator
{
  public:
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using size_type = usize;
    using difference_type = idiff;

    template <typename U> struct rebind
    {
        using other = STLAllocator<U>;
    };

    STLAllocator() noexcept = default;

    template <typename U> STLAllocator(const STLAllocator<U> &) noexcept
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

    bool operator==(const STLAllocator &) const noexcept
    {
        return true;
    }
    bool operator!=(const STLAllocator &) const noexcept
    {
        return false;
    }
};

/**
 * @brief Construct an object of type `T` in the given memory location.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param p_Ptr A pointer to the memory location where the object should be constructed.
 * @param p_Args The arguments to pass to the constructor of `T`.
 * @return A pointer to the constructed object.
 */
template <typename T, typename... Args> T *Construct(T *p_Ptr, Args &&...p_Args) noexcept
{
    return std::launder(::new (p_Ptr) T(std::forward<Args>(p_Args)...));
}

/**
 * @brief Destroy an object of type `T` in the given memory location.
 *
 * @note This function does not deallocate the memory. It only calls the destructor of the object.
 *
 * @param p_Ptr A pointer to the memory location where the object should be destroyed.
 */
template <typename T> void Destruct(T *p_Ptr) noexcept
{
    p_Ptr->~T();
}

/**
 * @brief Construct an object of type `T` in the memory location an iterator points to.
 *
 * Useful when dealing with iterators that could be pointers themselves.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param p_It An iterator pointing to the memory location where the object should be constructed.
 * @param p_Args The arguments to pass to the constructor of `T`.
 * @return A pointer to the constructed object.
 */
template <typename It, typename... Args> auto ConstructFromIterator(const It p_It, Args &&...p_Args) noexcept
{
    if constexpr (std::is_pointer_v<It>)
        return Construct(p_It, std::forward<Args>(p_Args)...);
    else
        return Construct(&*p_It, std::forward<Args>(p_Args)...);
}

/**
 * @brief Destroy an object of type `T` in the memory location an iterator points to.
 *
 * Useful when dealing with iterators that could be pointers themselves.
 *
 * @note This function does not deallocate the memory. It only calls the destructor of the object.
 *
 * @param p_It An iterator pointing to the memory location where the object should be destroyed.
 */
template <typename It> void DestructFromIterator(const It p_It) noexcept
{
    if constexpr (std::is_pointer_v<It>)
        Destruct(p_It);
    else
        Destruct(&*p_It);
}

/**
 * @brief Construct a range of objects of type `T` given some constructor arguments.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param p_Begin An iterator pointing to the beginning of the range where the objects should be constructed.
 * @param p_End An iterator pointing to the end of the range where the objects should be constructed.
 * @param p_Args The arguments to pass to the constructor of `T`.
 */
template <typename It, typename... Args>
void ConstructRange(const It p_Begin, const It p_End, const Args &...p_Args) noexcept
{
    for (auto it = p_Begin; it != p_End; ++it)
        ConstructFromIterator(it, p_Args...);
}

/**
 * @brief Construct a range of objects of type `T` by copying from another range.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param p_Dst An iterator pointing to the beginning of the destination range where the objects should be constructed.
 * @param p_Begin An iterator pointing to the beginning of the source range where the objects should be copied from.
 * @param p_End An iterator pointing to the end of the source range where the objects should be copied from.
 */
template <typename It1, typename It2> void ConstructRangeCopy(It1 p_Dst, const It2 p_Begin, const It2 p_End) noexcept
{
    for (auto it = p_Begin; it != p_End; ++it, ++p_Dst)
        ConstructFromIterator(p_Dst, *it);
}

/**
 * @brief Construct a range of objects of type `T` by moving from another range.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param p_Dst An iterator pointing to the beginning of the destination range where the objects should be constructed.
 * @param p_Begin An iterator pointing to the beginning of the source range where the objects should be moved from.
 * @param p_End An iterator pointing to the end of the source range where the objects should be moved from.
 */
template <typename It1, typename It2> void ConstructRangeMove(It1 p_Dst, const It2 p_Begin, const It2 p_End) noexcept
{
    for (auto it = p_Begin; it != p_End; ++it, ++p_Dst)
        ConstructFromIterator(p_Dst, std::move(*it));
}

/**
 * @brief Destroy a range of objects of type `T`.
 *
 * @note This function does not deallocate the memory. It only calls the destructor of the object.
 *
 * @param p_Begin An iterator pointing to the beginning of the range where the objects should be destroyed.
 * @param p_End An iterator pointing to the end of the range where the objects should be destroyed.
 */
template <typename It> void DestructRange(const It p_Begin, const It p_End) noexcept
{
    for (auto it = p_Begin; it != p_End; ++it)
        DestructFromIterator(it);
}

} // namespace TKit::Memory

#ifndef TKIT_DISABLE_MEMORY_OVERRIDES

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(28251)

TKIT_API void *operator new(size_t p_Size);
TKIT_API void *operator new[](size_t p_Size);
TKIT_API void *operator new(size_t p_Size, std::align_val_t p_Alignment);
TKIT_API void *operator new[](size_t p_Size, std::align_val_t p_Alignment);
TKIT_API void *operator new(size_t p_Size, const std::nothrow_t &) noexcept;
TKIT_API void *operator new[](size_t p_Size, const std::nothrow_t &) noexcept;

TKIT_COMPILER_WARNING_IGNORE_POP()

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
