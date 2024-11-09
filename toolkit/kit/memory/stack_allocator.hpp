#pragma once

#include "kit/core/logging.hpp"
#include "kit/core/non_copyable.hpp"

namespace KIT
{
/**
 * @brief A simple stack allocator that allocates memory in a stack-like fashion. It is useful for temporary allocations
 * and allows many types of elements to coexist in a single contiguous chunk of memory.
 *
 * This allocator can both allocate and initialize objects in place in that same memory. Use the
 * Allocate/Deallocate/Push/Pop for the former and Create/Destroy for the latter. Never mix them, as it will lead to
 * undefined behavior. (Allocate/Deallocate are equivalent to Push/Pop, so you can use them interchangeably, but for
 * every Create, there must be a Destroy)
 *
 * @note Thread safety considerations: This allocator requires precise ordering of allocations and deallocations.
 * A multithreaded environment has the exact opposite property, so this allocator is not thread safe.
 *
 */
class KIT_API StackAllocator
{
    KIT_NON_COPYABLE(StackAllocator)
  public:
    struct Entry
    {
        // If asserts are enabled, I store the alignment offset so that I can assert the order of
        // allocations/deallocations is respected

        Entry(std::byte *p_Ptr, const usize p_Size, const usize p_AlignmentOffset)
            : Ptr(p_Ptr), Size(p_Size), AlignmentOffset(p_AlignmentOffset)
        {
        }
        std::byte *Ptr;
        usize Size;
        usize AlignmentOffset;
    };

    // The alignment parameter specifies the starting alignment of the whole block so that your first allocation will
    // not be padded in case you need specific alignment requirements for it, but it does not restrict the alignment of
    // the individual allocations at all. You can still specify alignments of 64 if you want when allocating
    explicit StackAllocator(usize p_Size, usize p_Alignment = 8) noexcept;
    ~StackAllocator() noexcept;

    StackAllocator(StackAllocator &&p_Other) noexcept;
    StackAllocator &operator=(StackAllocator &&p_Other) noexcept;

    // Push and Allocate do exactly the same, I just wanted to keep both APIs
    // Pop and Deallocate are *almost* the same. Deallocate requires the actual pointer to be passed. It is used mainly
    // for debugging purposes when asserts are enabled, to ensure proper deallocation order. Pop just mindlessly pops,
    // no asserts, no nothing

    // Alignment is set to 1, because in place allocations *should* allow any kind of alignment, including ones under 8
    // (not like malloc or posix_memalign). All of this in 64 bit systems

    /**
     * @brief Allocate a new block of memory into the stack allocator (Same as Allocate()).
     *
     * @param p_Size The size of the block to allocate.
     * @param p_Alignment The alignment of the block.
     * @return void* A pointer to the allocated block.
     */
    void *Push(usize p_Size, usize p_Alignment = 1) noexcept;

    /**
     * @brief Pop the last block of memory from the stack allocator.
     *
     */
    void Pop() noexcept;

    /**
     * @brief Allocate a new block of memory in the stack allocator (Same as Push()).
     *
     * @param p_Size The size of the block to allocate.
     * @param p_Alignment The alignment of the block.
     * @return void* A pointer to the allocated block.
     */
    void *Allocate(usize p_Size, usize p_Alignment = 1) noexcept;

    /**
     * @brief Deallocate a block of memory from the stack allocator. This method, if used correctly, should behave
     * exactly like Pop(). The pointer is kept there for consistency and for debugging purposes when asserts are
     * enabled. If disabled, this method is just a wrapper around Pop().
     *
     * @param p_Ptr The pointer to the block to deallocate.
     */
    void Deallocate([[maybe_unused]] const void *p_Ptr) noexcept;

    /**
     * @brief Allocate a new block of memory in the stack allocator and create a new object of type T out of it.
     *
     * @tparam T The type of the block.
     * @param p_N The number of elements of type T to allocate.
     * @return T* A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *Create(Args &&...p_Args) noexcept
    {
        T *ptr = static_cast<T *>(Allocate(sizeof(T), alignof(T)));
        if (!ptr)
            return nullptr;
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    /**
     * @brief Allocate a new block of memory in the stack allocator and create an array of objects of type T out of it.
     * The block is created with the size of T * p_N.
     *
     * @tparam T The type of the block.
     * @param p_N The number of elements of type T to allocate.
     * @return T* A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *NCreate(const usize p_N, Args &&...p_Args) noexcept
    {
        T *ptr = static_cast<T *>(Allocate(p_N * sizeof(T), alignof(T)));
        if (!ptr)
            return nullptr;
        for (usize i = 0; i < p_N; ++i)
            ::new (&ptr[i]) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    /**
     * @brief Deallocate a block of memory from the stack allocator and destroy the object of type T created from it.
     *
     * @tparam T The type of the block.
     * @param p_Ptr The pointer to the block to deallocate.
     */
    template <typename T> void Destroy(T *p_Ptr) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            KIT_ASSERT(!m_Entries.empty(), "Unable to deallocate because the stack allocator is empty");
            KIT_ASSERT(m_Entries.back().Ptr == reinterpret_cast<std::byte *>(p_Ptr),
                       "Elements must be deallocated in the reverse order they were allocated");

            const usize n = m_Entries.back().Size / sizeof(T);
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
     * @brief Get the size of the stack allocator.
     *
     */
    usize GetSize() const noexcept;

    /**
     * @brief Get the total amount of memory allocated in the stack allocator.
     *
     */
    usize GetAllocated() const noexcept;

    /**
     * @brief Get the total amount of memory remaining in the stack allocator.
     *
     */
    usize GetRemaining() const noexcept;

    /**
     * @brief Check if a pointer belongs to the stack allocator.
     *
     * @param p_Ptr The pointer to check.
     * @return Whether the pointer belongs to the stack allocator.
     */
    bool Belongs(const void *p_Ptr) const noexcept;

    /**
     * @brief Check if the stack allocator is empty.
     *
     * @return Whether the stack allocator is empty.
     */
    bool IsEmpty() const noexcept;

    /**
     * @brief Check if the stack allocator is full.
     *
     * @return Whether the stack allocator is full.
     */
    bool IsFull() const noexcept;

  private:
    void deallocateBuffer() noexcept;

    std::byte *m_Buffer = nullptr;
    usize m_Size = 0;
    usize m_Remaining = 0;
    DynamicArray<Entry> m_Entries;
};
} // namespace KIT