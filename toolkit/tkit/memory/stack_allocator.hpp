#pragma once

#ifndef TKIT_ENABLE_STACK_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_STACK_ALLOCATOR"
#endif

#include "tkit/utils/logging.hpp"
#include "tkit/utils/non_copyable.hpp"
#include "tkit/container/static_array.hpp"

#ifndef TKIT_STACK_ALLOCATOR_MAX_ENTRIES
#    define TKIT_STACK_ALLOCATOR_MAX_ENTRIES 128
#endif

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
class TKIT_API StackAllocator
{
    TKIT_NON_COPYABLE(StackAllocator)
  public:
    struct Entry
    {
        Entry(std::byte *p_Ptr, const usize p_Size, const usize p_AlignmentOffset)
            : Ptr(p_Ptr), Size(p_Size), AlignmentOffset(p_AlignmentOffset)
        {
        }
        std::byte *Ptr;
        usize Size;
        usize AlignmentOffset;
    };

    // The alignment parameter specifies the starting alignment of the whole memory buffer so that your first allocation
    // will not be padded in case you need specific alignment requirements for it, but it does not restrict the
    // alignment of the individual allocations at all. You can still specify alignments of, say, 64 if you want when
    // allocating

    explicit StackAllocator(usize p_Size, usize p_Alignment = alignof(std::max_align_t)) noexcept;

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    StackAllocator(void *p_Buffer, usize p_Size) noexcept;
    ~StackAllocator() noexcept;

    StackAllocator(StackAllocator &&p_Other) noexcept;
    StackAllocator &operator=(StackAllocator &&p_Other) noexcept;

    /**
     * @brief Allocate a new block of memory into the stack allocator.
     *
     * @param p_Size The size of the block to allocate.
     * @param p_Alignment The alignment of the block.
     * @return A pointer to the allocated block.
     */
    void *Allocate(usize p_Size, usize p_Alignment = alignof(std::max_align_t)) noexcept;
    /**
     * @brief Allocate a new block of memory into the stack allocator and casts the result to `T`.
     *
     * @param p_N The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T> T *Allocate(const usize p_N = 1) noexcept
    {
        return static_cast<T *>(Allocate(p_N * sizeof(T), alignof(T)));
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
    void Deallocate(const void *p_Ptr) noexcept;

    /**
     * @brief Allocate a new block of memory in the stack allocator and create a new object of type `T` out of it.
     *
     * @tparam T The type of the block.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *Create(Args &&...p_Args) noexcept
    {
        T *ptr = static_cast<T *>(Allocate(sizeof(T), alignof(T)));
        if (!ptr)
            return nullptr;
        return Memory::Construct(ptr, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Allocate a new block of memory in the stack allocator and create an array of objects of type `T` out of
     * it.
     *
     * The block is created with the size of `sizeof(T) * p_N`.
     *
     * @tparam T The type of the block.
     * @param p_N The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *NCreate(const usize p_N, Args &&...p_Args) noexcept
    {
        T *ptr = static_cast<T *>(Allocate(p_N * sizeof(T), alignof(T)));
        if (!ptr)
            return nullptr;
        Memory::ConstructRange(ptr, ptr + p_N, std::forward<Args>(p_Args)...);
        return ptr;
    }

    /**
     * @brief Deallocate a block of memory from the stack allocator and destroy the object of type `T` created from it.
     *
     * @tparam T The type of the block.
     * @param p_Ptr The pointer to the block to deallocate.
     */
    template <typename T> void Destroy(T *p_Ptr) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            TKIT_ASSERT(!m_Entries.IsEmpty(), "[TOOLKIT] Unable to deallocate because the stack allocator is empty");
            TKIT_ASSERT(m_Entries.GetBack().Ptr == reinterpret_cast<std::byte *>(p_Ptr),
                        "[TOOLKIT] Elements must be deallocated in the reverse order they were allocated");

            const usize n = m_Entries.GetBack().Size / sizeof(T);
            for (usize i = 0; i < n; ++i)
                p_Ptr[i].~T();
        }
        Deallocate(p_Ptr);
    }

    /**
     * @brief Get the top entry of the stack allocator.
     *
     */
    const Entry &Top() const noexcept;

    /**
     * @brief Get the top entry of the stack allocator.
     *
     */
    template <typename T> T *Top() noexcept
    {
        return reinterpret_cast<T *>(Top().Ptr);
    }

    /**
     * @brief Check if a pointer belongs to the stack allocator.
     *
     * @param p_Ptr The pointer to check.
     * @return Whether the pointer belongs to the stack allocator.
     */
    bool Belongs(const void *p_Ptr) const noexcept;

    bool IsEmpty() const noexcept;
    bool IsFull() const noexcept;

    usize GetSize() const noexcept;
    usize GetAllocated() const noexcept;
    usize GetRemaining() const noexcept;

  private:
    void deallocateBuffer() noexcept;

    StaticArray<Entry, TKIT_STACK_ALLOCATOR_MAX_ENTRIES> m_Entries{};
    std::byte *m_Buffer;
    usize m_Size = 0;
    usize m_Remaining = 0;
    bool m_Provided;
};
} // namespace TKit