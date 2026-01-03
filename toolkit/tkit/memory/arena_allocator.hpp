#pragma once

#ifndef TKIT_ENABLE_ARENA_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][ARENA-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_ARENA_ALLOCATOR"
#endif

#include "tkit/utils/non_copyable.hpp"
#include "tkit/memory/memory.hpp"

namespace TKit
{
/**
 * @brief A simple arena allocator that allocates memory in a stack-like fashion, but does not feature deallocation of
 * individual blocks.
 *
 * It is useful for temporary allocations and allows many types of elements to coexist in a single contiguous chunk of
 * memory.
 *
 * Please, take into account that, if allocating non-trivially destructible objects, you will have to manually call the
 * destructor for each object before releasing the memory. This allocator only handles memory deallocation of the whole
 * block. It will not call any destructor.
 *
 * @note Thread safety considerations: This allocator is not thread safe.
 *
 */
class ArenaAllocator
{
    TKIT_NON_COPYABLE(ArenaAllocator)
  public:
    // The alignment parameter specifies the starting alignment of the whole memory buffer so that your first allocation
    // will not be padded in case you need specific alignment requirements for it, but it does not restrict the
    // alignment of the individual allocations at all. You can still specify alignments of, say, 64 if you want when
    // allocating

    explicit ArenaAllocator(usize p_Size, usize p_Alignment = alignof(std::max_align_t));

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    ArenaAllocator(void *p_Buffer, usize p_Size);
    ~ArenaAllocator();

    ArenaAllocator(ArenaAllocator &&p_Other);
    ArenaAllocator &operator=(ArenaAllocator &&p_Other);

    /**
     * @brief Allocate a new block of memory into the arena allocator.
     *
     * @param p_Size The size of the block to allocate.
     * @param p_Alignment The alignment of the block.
     * @return A pointer to the allocated block.
     */
    void *Allocate(usize p_Size, usize p_Alignment = alignof(std::max_align_t));

    /**
     * @brief Allocate a new block of memory into the arena allocator and casts the result to `T`.
     *
     * @param p_Count The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T> T *Allocate(const usize p_Count = 1)
    {
        return static_cast<T *>(Allocate(p_Count * sizeof(T), alignof(T)));
    }

    /**
     * @brief Reset the arena allocator to its initial state, deallocating all memory.
     *
     * It may be used again after calling this method.
     *
     */
    void Reset();

    /**
     * @brief Allocate a new block of memory in the arena allocator and create a new object of type `T` out of it.
     *
     * @tparam T The type of the block.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *Create(Args &&...p_Args)
    {
        T *ptr = Allocate<T>();
        if (!ptr)
            return nullptr;
        return Memory::Construct(ptr, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Allocate a new block of memory in the arena allocator and create an array of objects of type `T` out of
     * it.
     *
     * The block is created with the size of `sizeof(T) * p_Count`.
     *
     * @tparam T The type of the block.
     * @param p_Count The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *NCreate(const usize p_Count, Args &&...p_Args)
    {
        T *ptr = Allocate<T>(p_Count);
        if (!ptr)
            return nullptr;
        Memory::ConstructRange(ptr, ptr + p_Count, std::forward<Args>(p_Args)...);
        return ptr;
    }

    /**
     * @brief Check if a pointer belongs to the arena allocator.
     *
     * @param p_Ptr The pointer to check.
     * @return Whether the pointer belongs to the arena allocator.
     */
    bool Belongs(const void *p_Ptr) const
    {
        const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
        return ptr >= m_Buffer && ptr < m_Buffer + (m_Size - m_Remaining);
    }

    bool IsEmpty() const
    {
        return m_Remaining == m_Size;
    }

    bool IsFull() const
    {
        return m_Remaining == 0;
    }

    usize GetSize() const
    {
        return m_Size;
    }
    usize GetAllocated() const
    {
        return m_Size - m_Remaining;
    }
    usize GetRemaining() const
    {
        return m_Remaining;
    }

  private:
    void deallocateBuffer();

    std::byte *m_Buffer;
    usize m_Size = 0;
    usize m_Remaining = 0;
    bool m_Provided;
};
} // namespace TKit
