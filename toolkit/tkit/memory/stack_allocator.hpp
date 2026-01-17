#pragma once

#ifndef TKIT_ENABLE_STACK_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][STACK-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_STACK_ALLOCATOR"
#endif

#include "tkit/utils/debug.hpp"
#include "tkit/utils/non_copyable.hpp"
#include "tkit/memory/memory.hpp"

namespace TKit
{
/**
 * @brief A simple stack allocator that allocates memory in a stack-like fashion.
 *
 * It is useful for temporary allocations and allows many types of elements to coexist in a single contiguous chunk of
 * memory.
 *
 * This allocator can both allocate and initialize objects in place in that same memory. Use the
 * `Allocate()`/`Deallocate()` for the former and `Create()`/`Destroy()` for the latter. Never mix them, as it
 * will lead to undefined behavior. For every `Allocate()` there must be a `Deallocate()` and for every `Create()`
 * there must be a `Destroy()`.
 *
 * @note Thread safety considerations: This allocator requires precise ordering of allocations and deallocations.
 * A multithreaded environment has the exact opposite property, so this allocator is not thread safe.
 *
 */
class StackAllocator
{
    TKIT_NON_COPYABLE(StackAllocator)
  public:
    explicit StackAllocator(usize p_Capacity, usize p_Alignment = alignof(std::max_align_t));

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    StackAllocator(void *p_Buffer, usize p_Capacity, usize p_Alignment = alignof(std::max_align_t));
    ~StackAllocator();

    StackAllocator(StackAllocator &&p_Other);
    StackAllocator &operator=(StackAllocator &&p_Other);

    /**
     * @brief Allocate a new block of memory into the stack allocator.
     *
     * @param p_Size The size of the block to allocate.
     * @param p_Alignment The alignment of the block.
     * @return A pointer to the allocated block.
     */
    void *Allocate(usize p_Size);
    /**
     * @brief Allocate a new block of memory into the stack allocator and casts the result to `T`.
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
     * @brief Deallocate a block of memory from the stack allocator.
     *
     * @note The pointer parameter is not strictly necessary and is kept there for
     * consistency and for debugging purposes when asserts are enabled. If disabled, this method will ignore
     * the parameter and deallocate the last block of memory.
     *
     * @param p_Ptr The pointer to the block to deallocate.
     */
    void Deallocate(const void *p_Ptr, usize p_Size);

    template <typename T>
        requires(!std::same_as<T, void>)
    void Deallocate(const T *p_Ptr, const usize p_Count = 1)
    {
        Deallocate(static_cast<const void *>(p_Ptr), p_Count * sizeof(T));
    }

    /**
     * @brief Allocate a new block of memory in the stack allocator and create a new object of type `T` out of it.
     *
     * @tparam T The type of the block.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T *Create(Args &&...p_Args)
    {
        T *ptr = Allocate<T>();
        if (!ptr)
            return nullptr;
        return Memory::Construct(ptr, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Deallocate a block of memory from the stack allocator and destroy the object of type `T` created from it.
     *
     * @tparam T The type of the block.
     * @param p_Ptr The pointer to the block to deallocate.
     */
    template <typename T>
        requires(!std::same_as<T, void>)
    constexpr void Destroy(const T *p_Ptr)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        Deallocate(p_Ptr);
    }

    /**
     * @brief Allocate a new block of memory in the stack allocator and create an array of objects of type `T` out of
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
    constexpr T *NCreate(const usize p_Count, Args &&...p_Args)
    {
        T *ptr = Allocate<T>(p_Count);
        if (!ptr)
            return nullptr;
        Memory::ConstructRange(ptr, ptr + p_Count, std::forward<Args>(p_Args)...);
        return ptr;
    }

    /**
     * @brief Deallocate a block of memory from the stack allocator and destroy the object of type `T` created from it.
     *
     * @tparam T The type of the block.
     * @param p_Ptr The pointer to the block to deallocate.
     * @param p_Count The number of elements of type `T` to deallocate.
     */
    template <typename T> constexpr void NDestroy(const T *p_Ptr, const usize p_Count)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            TKIT_ASSERT(p_Ptr, "[TOOLKIT][STACK-ALLOC] Cannot deallocate a null pointer");
            TKIT_ASSERT(m_Top != 0, "[TOOLKIT][STACK-ALLOC] Unable to deallocate because the stack allocator is empty");
            TKIT_ASSERT(m_Buffer + m_Top - Memory::NextAlignedSize(sizeof(T) * p_Count, m_Alignment) ==
                            reinterpret_cast<const std::byte *>(p_Ptr),
                        "[TOOLKIT][STACK-ALLOC] Elements must be deallocated in the reverse order they were allocated");

            for (usize i = 0; i < p_Count; ++i)
                p_Ptr[i].~T();
        }
        Deallocate(p_Ptr);
    }

    /**
     * @brief Check if a pointer belongs to the stack allocator.
     *
     * @param p_Ptr The pointer to check.
     * @return Whether the pointer belongs to the stack allocator.
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
