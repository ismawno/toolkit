#pragma once

#include "tkit/core/logging.hpp"
#include "tkit/core/non_copyable.hpp"

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
class TKIT_API ArenaAllocator
{
    TKIT_NON_COPYABLE(ArenaAllocator)

  public:
    // The alignment parameter specifies the starting alignment of the whole block so that your first allocation will
    // not be padded in case you need specific alignment requirements for it, but it does not restrict the alignment of
    // the individual allocations at all. You can still specify alignments of 64 if you want when allocating

    explicit ArenaAllocator(usize p_Size, usize p_Alignment = 8) noexcept;
    ~ArenaAllocator() noexcept;

    ArenaAllocator(ArenaAllocator &&p_Other) noexcept;
    ArenaAllocator &operator=(ArenaAllocator &&p_Other) noexcept;

    /**
     * @brief Allocate a new block of memory into the arena allocator.
     *
     * @param p_Size The size of the block to allocate.
     * @param p_Alignment The alignment of the block.
     * @return A pointer to the allocated block.
     */
    void *Push(usize p_Size, usize p_Alignment = 1) noexcept;

    /**
     * @brief Allocate a new block of memory into the arena allocator (Same as `Push()`).
     *
     * @param p_N The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T> T *Push(const usize p_N) noexcept
    {
        return static_cast<T *>(Push(p_N * sizeof(T), alignof(T)));
    }

    /**
     * @brief Reset the arena allocator to its initial state, deallocating all memory.
     *
     * It may be used again after calling this method.
     *
     */
    void Reset() noexcept;

    /**
     * @brief Allocate a new block of memory in the arena allocator and create a new object of type `T` out of it.
     *
     * @tparam T The type of the block.
     * @param p_N The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *Create(Args &&...p_Args) noexcept
    {
        T *ptr = static_cast<T *>(Push(sizeof(T), alignof(T)));
        if (!ptr)
            return nullptr;
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    /**
     * @brief Allocate a new block of memory in the arena allocator and create an array of objects of type `T` out of
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
        T *ptr = static_cast<T *>(Push(p_N * sizeof(T), alignof(T)));
        if (!ptr)
            return nullptr;
        for (usize i = 0; i < p_N; ++i)
            ::new (&ptr[i]) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    /**
     * @brief Get the size of the arena allocator.
     *
     */
    usize GetSize() const noexcept;

    /**
     * @brief Get the total amount of memory allocated in the arena allocator.
     *
     */
    usize GetAllocated() const noexcept;

    /**
     * @brief Get the total amount of memory remaining in the arena allocator.
     *
     */
    usize GetRemaining() const noexcept;

    /**
     * @brief Check if a pointer belongs to the arena allocator.
     *
     * @param p_Ptr The pointer to check.
     * @return Whether the pointer belongs to the arena allocator.
     */
    bool Belongs(const void *p_Ptr) const noexcept;

    /**
     * @brief Check if the arena allocator is empty.
     *
     */
    bool IsEmpty() const noexcept;

    /**
     * @brief Check if the arena allocator is full.
     *
     */
    bool IsFull() const noexcept;

  private:
    void deallocateBuffer() noexcept;

    std::byte *m_Buffer = nullptr;
    usize m_Size = 0;
    usize m_Remaining = 0;
};
} // namespace TKit