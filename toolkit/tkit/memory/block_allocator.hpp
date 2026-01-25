#pragma once

#ifndef TKIT_ENABLE_BLOCK_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][BLOCK-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_BLOCK_ALLOCATOR"
#endif

#include "tkit/utils/non_copyable.hpp"
#include "tkit/preprocessor/system.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
/**
 * @brief A block allocator that allocates memory in blocks of a fixed size.
 *
 * Every allocation (block) this allocator provides has always the same size, specified at construction. It maintains a
 * free list of allocations and ensures loose contiguity of the allocated memory.
 *
 * The block allocator deallocates all memory when it is destroyed if it has not been provided by the user. It is up to
 * the user to ensure that all memory is freed at that point, especially when dealing with non-trivial destructors.
 *
 * This allocator holds a single memory buffer whose size is provided at construction and cannot be
 * modified afterwards. Attempting to allocate more memory than the buffer size will result in udefined behaviour.
 *
 * Some performance numbers (measured on my macOS M1):
 * - Allocating 10000 elements of 128 bytes in 0.035 ms (3.5 ns per allocation)
 * - Deallocating 10000 elements of 128 bytes in 0.012 ms (1.2 ns per deallocation)
 *
 * This is roughly a 10x improvement over the default new/delete.
 */
TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4324)
class alignas(TKIT_CACHE_LINE_SIZE) BlockAllocator
{
    TKIT_NON_COPYABLE(BlockAllocator)
  public:
    // The alignment parameter specifies the alignment of all consecuent allocations. Thus, the buffer size must be a
    // multiple of the alignment

    BlockAllocator(usize bufferSize, usize allocationSize, usize alignment = alignof(std::max_align_t));

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    BlockAllocator(void *buffer, usize bufferSize, usize allocationSize);
    ~BlockAllocator();

    BlockAllocator(BlockAllocator &&other);
    BlockAllocator &operator=(BlockAllocator &&other);

    /**
     * @brief Create a block allocator suited to allocate elements from type `T`.
     *
     * @tparam T The type the allocator will be suited for.
     * @param count The capacity of the allocator, measured in how many objects of type `T` will be able to allocate.
     * @return A `BlockAllocator` instance.
     */
    template <typename T> static BlockAllocator CreateFromType(const usize count)
    {
        const usize size = sizeof(T) > sizeof(Allocation) ? sizeof(T) : sizeof(Allocation);
        return BlockAllocator{count * size, size, alignof(T)};
    }

    /**
     * @brief Allocate a new block of memory into the block allocator.
     *
     * This allocation has a fixed size. Attempting to use more memory than the block size will result in udefined
     * behaviour.
     *
     * @return A pointer to the allocated block. If the allocation fails, `nullptr` will be returned.
     */
    void *Allocate();

    /**
     * @brief Allocate a new block of memory into the block allocator.
     *
     * This allocation has a fixed size. Attempting to use more memory than the block size will result in udefined
     * behaviour.
     *
     * @tparam T The type of object to allocate.
     * @return A pointer to the allocated block. If the allocation fails, `nullptr` will be returned.
     */
    template <typename T> T *Allocate()
    {
        TKIT_ASSERT(sizeof(T) <= m_AllocationSize,
                    "[TOOLKIT][BLOCK-ALLOC] Block allocator allocation size is {}, but sizeof(T) is {} bytes, which "
                    "does not fit into an allocation",
                    m_AllocationSize, sizeof(T));
        T *ptr = static_cast<T *>(Allocate());
        TKIT_ASSERT(!ptr || Memory::IsAligned(ptr, alignof(T)),
                    "[TOOLKIT][BLOCK-ALLOC] Type T has stronger memory alignment requirements than specified. Bump the "
                    "alignment of the allocator or prevent using it to allocate objects of such type");
        return ptr;
    }

    /**
     * @brief Deallocate a block of memory from the block allocator.
     *
     * @param ptr A pointer to the block to deallocate.
     */
    void Deallocate(void *ptr);

    /**
     * @brief Allocate a new block of memory and create a new object of type `T` out of it.
     *
     * @tparam T The type of object to allocate.
     * @return A pointer to the allocated block. If the allocation fails, `nullptr` will be returned and the object will
     * not be constructed.
     */
    template <typename T, typename... Args> T *Create(Args &&...args)
    {
        T *ptr = Allocate<T>();
        return ptr ? Memory::Construct(ptr, std::forward<Args>(args)...) : nullptr;
    }

    /**
     * @brief Deallocate a block of memory and destroy the object of type `T` created from it.
     *
     * @tparam T The type of object to deallocate.
     * @param ptr The pointer to the block to deallocate.
     */
    template <typename T> void Destroy(T *ptr)
    {
        TKIT_ASSERT(ptr, "[TOOLKIT][BLOCK-ALLOC] Cannot deallocate a null pointer");
        // TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][BLOCK-ALLOC] Cannot deallocate from an empty allocator");
        TKIT_ASSERT(Belongs(ptr),
                    "[TOOLKIT][BLOCK-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");
        if constexpr (!std::is_trivially_destructible_v<T>)
            ptr->~T();
        Deallocate(ptr);
    }

    void Reset();

    /**
     * @brief Check if a pointer belongs to the block allocator.
     *
     * @note This is a simple check to see if the provided pointer lies within the boundaries of the buffer. It will not
     * be able to determine if the pointer is currently allocated or free.
     *
     * @param ptr The pointer to check.
     * @return Whether the pointer belongs to the block allocator.
     */
    bool Belongs(const void *ptr) const
    {
        const std::byte *bptr = static_cast<const std::byte *>(ptr);
        return bptr >= m_Buffer && bptr < m_Buffer + m_BufferSize;
    }

    bool IsFull() const
    {
        return !m_FreeList;
    }

    usize GetBufferSize() const
    {
        return m_BufferSize;
    }
    usize GetAllocationSize() const
    {
        return m_AllocationSize;
    }

    usize GetAllocationCapacityCount() const
    {
        return m_BufferSize / m_AllocationSize;
    }

  private:
    struct Allocation
    {
        Allocation *Next;
    };

    void setupMemoryLayout();
    void deallocateBuffer();

    std::byte *m_Buffer;
    Allocation *m_FreeList;
    usize m_BufferSize;
    usize m_AllocationSize;
    bool m_Provided;
};
TKIT_COMPILER_WARNING_IGNORE_POP()
} // namespace TKit
