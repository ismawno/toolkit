#pragma once

#include "tkit/preprocessor/system.hpp"
#include "tkit/utils/alias.hpp"
#include "tkit/utils/debug.hpp"
#include <algorithm>

#ifdef TKIT_COMPILER_MSVC
#    define TKIT_MEMORY_STACK_ALLOCATE(size) _alloca(size)
#    define TKIT_MEMORY_STACK_DEALLOCATE(ptr)
#    define TKIT_MEMORY_STACK_CHECK(size)                                                                              \
        TKIT_ASSERT(size <= TKit::MaxStackAlloc,                                                                       \
                    "[TOOLKIT][MEMORY] Stack allocation size exceeded. Requested size was {}, but maximum is {}",      \
                    size, TKit::MaxStackAlloc)
#elif defined(TKIT_COMPILER_GCC) || defined(TKIT_COMPILER_CLANG)
#    define TKIT_MEMORY_STACK_ALLOCATE(size) alloca(size)
#    define TKIT_MEMORY_STACK_DEALLOCATE(ptr)
#    define TKIT_MEMORY_STACK_CHECK(size)                                                                              \
        TKIT_ASSERT(size <= TKit::MaxStackAlloc,                                                                       \
                    "[TOOLKIT][MEMORY] Stack allocation size exceeded. Requested size was {}, but maximum is {}",      \
                    size, TKit::MaxStackAlloc)
#else
#    define TKIT_MEMORY_STACK_ALLOCATE(size) TKit::Memory::Allocate(size)
#    define TKIT_MEMORY_STACK_DEALLOCATE(ptr) TKit::Memory::Deallocate(ptr)
#    define TKIT_MEMORY_STACK_CHECK(size)
#endif

namespace TKit
{
class ArenaAllocator;
class StackAllocator;
class TierAllocator;
}; // namespace TKit

namespace TKit::Memory
{
void PushArena(ArenaAllocator *alloc);
void PushStack(StackAllocator *alloc);
void PushTier(TierAllocator *alloc);

ArenaAllocator *GetArena();
StackAllocator *GetStack();
TierAllocator *GetTier();

void PopArena();
void PopStack();
void PopTier();

// Could abstract this in some way (by defining those as function pointers) to allow for custom allocators
// For now, I'll leave it as it is

/**
 * @brief Allocate a chunk of memory of a given size.
 *
 * Uses default `malloc`. It is here as a placeholder for future custom global allocators.
 *
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory.
 */
void *Allocate(size_t size);

/**
 * @brief Deallocate a chunk of memory
 *
 * Uses default `free`. It is here as a placeholder for future custom global allocators.
 *
 * @param ptr A pointer to the memory to deallocate.
 */
void Deallocate(void *ptr);

/**
 * @brief Allocate a chunk of memory of a given size with a given alignment.
 *
 * Uses the default platform-specific aligned allocation. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param size The size of the memory to allocate.
 * @param alignment The alignment of the memory to allocate.
 * @return A pointer to the allocated memory.
 */
void *AllocateAligned(size_t size, size_t alignment);

/**
 * @brief Deallocate a chunk of memory with a given alignment.
 *
 * Uses the default platform-specific aligned deallocation. It is here as a placeholder for future custom global
 * allocators.
 *
 * @param ptr A pointer to the memory to deallocate.
 * @param alignment The alignment of the memory to deallocate.
 */
void DeallocateAligned(void *ptr);

/**
 * @brief Copy a chunk of memory from one location to another.
 *
 * Uses the default `::memcpy()`. It is here as a placeholder for future custom global memory management.
 *
 * @param dst A pointer to the destination memory.
 * @param src A pointer to the source memory.
 * @param size The size of the memory to copy, in bytes.
 */
void *ForwardCopy(void *dst, const void *src, size_t size);

/**
 * @brief Copy a chunk of memory from one location to another in reverse order.
 *
 * Uses the default `::memmove()`. It is here as a placeholder for future custom global memory management.
 *
 * @param dst A pointer to the destination memory.
 * @param src A pointer to the source memory.
 * @param size The size of the memory to copy, in bytes.
 */
void *BackwardCopy(void *dst, const void *src, size_t size);

/**
 * @brief Copy a range of elements from one iterator to another.
 *
 * Uses the default `std::copy()`. It is here as a placeholder for future custom global memory management.
 *
 * @param dst An iterator pointing to the destination memory.
 * @param begin An iterator pointing to the beginning of the source memory.
 * @param end An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto ForwardCopy(It1 dst, It2 begin, It2 end)
{
    return std::copy(begin, end, dst);
}

/**
 * @brief Copy a range of elements from one iterator to another in reverse order.
 *
 * Uses the default `std::copy_backward()`. It is here as a placeholder for future custom global memory management.
 *
 * @param dst An iterator pointing to the destination memory.
 * @param begin An iterator pointing to the beginning of the source memory.
 * @param end An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto BackwardCopy(It1 dst, It2 begin, It2 end)
{
    return std::copy_backward(begin, end, dst);
}

/**
 * @brief Move a chunk of memory from one location to another.
 *
 * Uses the default `std::move()`. It is here as a placeholder for future custom global memory management.
 *
 * @param dst An iterator pointing to the destination memory.
 * @param begin An iterator pointing to the beginning of the source memory.
 * @param end An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto ForwardMove(It1 dst, It2 begin, It2 end)
{
    return std::move(begin, end, dst);
}

/**
 * @brief Move a chunk of memory from one location to another in reverse order.
 *
 * Uses the default `std::move_backward()`. It is here as a placeholder for future custom global memory management.
 *
 * @param dst An iterator pointing to the destination memory.
 * @param begin An iterator pointing to the beginning of the source memory.
 * @param end An iterator pointing to the end of the source memory.
 */
template <typename It1, typename It2> constexpr auto BackwardMove(It1 dst, It2 begin, It2 end)
{
    return std::move_backward(begin, end, dst);
}

inline bool IsAligned(const void *ptr, const size_t alignment)
{
    const uptr addr = reinterpret_cast<uptr>(ptr);
    return (addr & (alignment - 1)) == 0;
}
inline bool IsAligned(const size_t address, const size_t alignment)
{
    return (address & (alignment - 1)) == 0;
}
inline usize NextAlignedSize(const usize size, const usize alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
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

    STLAllocator() = default;

    template <typename U> STLAllocator(const STLAllocator<U> &)
    {
    }

    pointer allocate(size_type n)
    {
        return static_cast<pointer>(Allocate(n * sizeof(T)));
    }

    void deallocate(pointer ptr, size_type)
    {
        Deallocate(ptr);
    }

    bool operator==(const STLAllocator &) const
    {
        return true;
    }
    bool operator!=(const STLAllocator &) const
    {
        return false;
    }
};

/**
 * @brief Construct an object of type `T` in the given memory location.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param ptr A pointer to the memory location where the object should be constructed.
 * @param args The arguments to pass to the constructor of `T`.
 * @return A pointer to the constructed object.
 */
template <typename T, typename... Args> T *Construct(T *ptr, Args &&...args)
{
    TKIT_ASSERT(
        IsAligned(ptr, alignof(T)),
        "[TOOLKIT][MEMORY] The address used to construct an object is not correctly aligned to its alignment of {}",
        alignof(T));
    return std::launder(::new (ptr) T(std::forward<Args>(args)...));
}

/**
 * @brief Destroy an object of type `T` in the given memory location.
 *
 * @note This function does not deallocate the memory. It only calls the destructor of the object.
 *
 * @param ptr A pointer to the memory location where the object should be destroyed.
 */
template <typename T> void Destruct(T *ptr)
{
    ptr->~T();
}

/**
 * @brief Construct an object of type `T` in the memory location an iterator points to.
 *
 * Useful when dealing with iterators that could be pointers themselves.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param it An iterator pointing to the memory location where the object should be constructed.
 * @param args The arguments to pass to the constructor of `T`.
 * @return A pointer to the constructed object.
 */
template <typename It, typename... Args> auto ConstructFromIterator(const It it, Args &&...args)
{
    if constexpr (std::is_pointer_v<It>)
        return Construct(it, std::forward<Args>(args)...);
    else
        return Construct(&*it, std::forward<Args>(args)...);
}

/**
 * @brief Destroy an object of type `T` in the memory location an iterator points to.
 *
 * Useful when dealing with iterators that could be pointers themselves.
 *
 * @note This function does not deallocate the memory. It only calls the destructor of the object.
 *
 * @param it An iterator pointing to the memory location where the object should be destroyed.
 */
template <typename It> void DestructFromIterator(const It it)
{
    if constexpr (std::is_pointer_v<It>)
        Destruct(it);
    else
        Destruct(&*it);
}

/**
 * @brief Construct a range of objects of type `T` given some constructor arguments.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param begin An iterator pointing to the beginning of the range where the objects should be constructed.
 * @param end An iterator pointing to the end of the range where the objects should be constructed.
 * @param args The arguments to pass to the constructor of `T`.
 */
template <typename It, typename... Args> void ConstructRange(const It begin, const It end, const Args &...args)
{
    for (auto it = begin; it != end; ++it)
        ConstructFromIterator(it, args...);
}

/**
 * @brief Construct a range of objects of type `T` by copying from another range.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param dst An iterator pointing to the beginning of the destination range where the objects should be constructed.
 * @param begin An iterator pointing to the beginning of the source range where the objects should be copied from.
 * @param end An iterator pointing to the end of the source range where the objects should be copied from.
 */
template <typename It1, typename It2> void ConstructRangeCopy(It1 dst, const It2 begin, const It2 end)
{
    for (auto it = begin; it != end; ++it, ++dst)
        ConstructFromIterator(dst, *it);
}

/**
 * @brief Construct a range of objects of type `T` by moving from another range.
 *
 * @note This function does not allocate memory. It only calls the constructor of the object.
 *
 * @param dst An iterator pointing to the beginning of the destination range where the objects should be constructed.
 * @param begin An iterator pointing to the beginning of the source range where the objects should be moved from.
 * @param end An iterator pointing to the end of the source range where the objects should be moved from.
 */
template <typename It1, typename It2> void ConstructRangeMove(It1 dst, const It2 begin, const It2 end)
{
    for (auto it = begin; it != end; ++it, ++dst)
        ConstructFromIterator(dst, std::move(*it));
}

/**
 * @brief Destroy a range of objects of type `T`.
 *
 * @note This function does not deallocate the memory. It only calls the destructor of the object.
 *
 * @param begin An iterator pointing to the beginning of the range where the objects should be destroyed.
 * @param end An iterator pointing to the end of the range where the objects should be destroyed.
 */
template <typename It> void DestructRange(const It begin, const It end)
{
    for (auto it = begin; it != end; ++it)
        DestructFromIterator(it);
}

} // namespace TKit::Memory
