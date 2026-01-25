#pragma once

#ifndef TKIT_ENABLE_STACK_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][STACK-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_STACK_ALLOCATOR"
#endif

#include "tkit/utils/debug.hpp"
#include "tkit/preprocessor/system.hpp"
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
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4324)
class alignas(TKIT_CACHE_LINE_SIZE) StackAllocator
{
    TKIT_NON_COPYABLE(StackAllocator)
  public:
    explicit StackAllocator(usize capacity, usize alignment = alignof(std::max_align_t));

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    StackAllocator(void *buffer, usize capacity, usize alignment = alignof(std::max_align_t));
    ~StackAllocator();

    StackAllocator(StackAllocator &&other);
    StackAllocator &operator=(StackAllocator &&other);

    /**
     * @brief Allocate a new block of memory into the stack allocator.
     *
     * @param size The size of the block to allocate.
     * @return A pointer to the allocated block.
     */
    void *Allocate(usize size);

    /**
     * @brief Allocate a new block of memory into the stack allocator and casts the result to `T`.
     *
     * @param count The number of elements of type `T` to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T> T *Allocate(const usize count = 1)
    {
        T *ptr = static_cast<T *>(Allocate(count * sizeof(T)));
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
     * @param ptr The pointer to the block to deallocate.
     */
    void Deallocate(const void *ptr, usize size);

    template <typename T>
        requires(!std::same_as<T, void>)
    void Deallocate(const T *ptr, const usize count = 1)
    {
        Deallocate(static_cast<const void *>(ptr), count * sizeof(T));
    }

    /**
     * @brief Allocate a new block of memory in the stack allocator and create a new object of type `T` out of it.
     *
     * @tparam T The type of the block.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T *Create(Args &&...args)
    {
        T *ptr = Allocate<T>();
        if (!ptr)
            return nullptr;
        return Memory::Construct(ptr, std::forward<Args>(args)...);
    }

    /**
     * @brief Deallocate a block of memory from the stack allocator and destroy the object of type `T` created from it.
     *
     * @tparam T The type of the block.
     * @param ptr The pointer to the block to deallocate.
     */
    template <typename T>
        requires(!std::same_as<T, void>)
    constexpr void Destroy(const T *ptr)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            ptr->~T();
        Deallocate(ptr);
    }

    /**
     * @brief Allocate a new block of memory in the stack allocator and create an array of objects of type `T` out of
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
    constexpr T *NCreate(const usize count, Args &&...args)
    {
        T *ptr = Allocate<T>(count);
        if (!ptr)
            return nullptr;
        Memory::ConstructRange(ptr, ptr + count, std::forward<Args>(args)...);
        return ptr;
    }

    /**
     * @brief Deallocate a block of memory from the stack allocator and destroy the object of type `T` created from it.
     *
     * @tparam T The type of the block.
     * @param ptr The pointer to the block to deallocate.
     * @param count The number of elements of type `T` to deallocate.
     */
    template <typename T> constexpr void NDestroy(const T *ptr, const usize count)
    {
        TKIT_ASSERT(ptr, "[TOOLKIT][STACK-ALLOC] Cannot deallocate a null pointer");
        TKIT_ASSERT(m_Top != 0, "[TOOLKIT][STACK-ALLOC] Unable to deallocate because the stack allocator is empty");
        TKIT_ASSERT(m_Buffer + m_Top - Memory::NextAlignedSize(sizeof(T) * count, m_Alignment) ==
                        reinterpret_cast<const std::byte *>(ptr),
                    "[TOOLKIT][STACK-ALLOC] Elements must be deallocated in the reverse order they were allocated");
        if constexpr (!std::is_trivially_destructible_v<T>)
            for (usize i = 0; i < count; ++i)
                ptr[i].~T();
        Deallocate(ptr, count);
    }

    /**
     * @brief Check if a pointer belongs to the stack allocator.
     *
     * @param ptr The pointer to check.
     * @return Whether the pointer belongs to the stack allocator.
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
