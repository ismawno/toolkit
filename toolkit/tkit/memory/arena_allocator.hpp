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
    explicit ArenaAllocator(usize p_Capacity, usize p_Alignment = alignof(std::max_align_t));

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    ArenaAllocator(void *p_Buffer, usize p_Capacity, usize p_Alignment = alignof(std::max_align_t));
    ~ArenaAllocator();

    ArenaAllocator(ArenaAllocator &&p_Other);
    ArenaAllocator &operator=(ArenaAllocator &&p_Other);

    /**
     * @brief Allocate a new block of memory into the arena allocator.
     *
     * @param p_Size The size of the block to allocate.
     * @return A pointer to the allocated block.
     */
    void *Allocate(usize p_Size);

    /**
     * @brief Allocate a new block of memory into the arena allocator and casts the result to `T`.
     *
     * @param p_Count The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T> T *Allocate(const usize p_Count = 1)
    {
        T *ptr = static_cast<T *>(Allocate(p_Count * sizeof(T)));
        TKIT_ASSERT(Memory::IsAligned(ptr, alignof(T)),
                    "[TOOLKIT][STACK-ALLOC] Requested type T to be allocated has stricter alignment requirements than "
                    "the ones provided by this allocator. Considering bumping the alignment parameter");
        return ptr;
    }

    /**
     * @brief Reset the arena allocator to its initial state, deallocating all memory.
     *
     * It may be used again after calling this method.
     *
     */
    void Reset()
    {
        m_Top = 0;
    }

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
        return ptr >= m_Buffer && ptr < m_Buffer + m_Top;
    }

    bool IsEmpty() const
    {
        return m_Top == 0;
    }

    bool IsFull() const
    {
        return m_Top == m_Capacity;
    }

    usize GetCapacity() const
    {
        return m_Capacity;
    }
    usize GetAllocatedBytes() const
    {
        return m_Top;
    }
    usize GetRemainingBytes() const
    {
        return m_Capacity - m_Top;
    }

  private:
    void deallocateBuffer();

    std::byte *m_Buffer = nullptr;
    usize m_Top = 0;
    usize m_Capacity = 0;
    usize m_Alignment = 0;
    bool m_Provided;
};
} // namespace TKit
