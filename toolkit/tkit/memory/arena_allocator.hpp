#pragma once

#ifndef TKIT_ENABLE_ARENA_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][ARENA-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_ARENA_ALLOCATOR"
#endif

#include "tkit/utils/non_copyable.hpp"
#include "tkit/preprocessor/system.hpp"
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
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4324)
class alignas(TKIT_CACHE_LINE_SIZE) ArenaAllocator
{
    TKIT_NON_COPYABLE(ArenaAllocator)
  public:
    explicit ArenaAllocator(usize capacity, usize alignment = alignof(std::max_align_t));

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    ArenaAllocator(void *buffer, usize capacity, usize alignment = alignof(std::max_align_t));
    ~ArenaAllocator();

    ArenaAllocator(ArenaAllocator &&other);
    ArenaAllocator &operator=(ArenaAllocator &&other);

    /**
     * @brief Allocate a new block of memory into the arena allocator.
     *
     * @param size The size of the block to allocate.
     * @return A pointer to the allocated block.
     */
    void *Allocate(usize size);

    /**
     * @brief Allocate a new block of memory into the arena allocator and casts the result to `T`.
     *
     * @param count The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T> T *Allocate(const usize count = 1)
    {
        T *ptr = static_cast<T *>(Allocate(count * sizeof(T)));
        TKIT_ASSERT(IsAligned(ptr, alignof(T)),
                    "[TOOLKIT][ARENA-ALLOC] Requested type T to be allocated has stricter alignment requirements than "
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
    T *Create(Args &&...args)
    {
        T *ptr = Allocate<T>();
        if (!ptr)
            return nullptr;
        return Construct(ptr, std::forward<Args>(args)...);
    }

    /**
     * @brief Allocate a new block of memory in the arena allocator and create an array of objects of type `T` out of
     * it.
     *
     * The block is created with the size of `sizeof(T) * count`.
     *
     * @tparam T The type of the block.
     * @param count The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    T *NCreate(const usize count, Args &&...args)
    {
        T *ptr = Allocate<T>(count);
        if (!ptr)
            return nullptr;
        ConstructRange(ptr, ptr + count, std::forward<Args>(args)...);
        return ptr;
    }

    /**
     * @brief Check if a pointer belongs to the arena allocator.
     *
     * @param ptr The pointer to check.
     * @return Whether the pointer belongs to the arena allocator.
     */
    bool Belongs(const void *ptr) const
    {
        const std::byte *bptr = reinterpret_cast<const std::byte *>(ptr);
        return bptr >= m_Buffer && bptr < m_Buffer + m_Top;
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
TKIT_COMPILER_WARNING_IGNORE_POP()
} // namespace TKit
