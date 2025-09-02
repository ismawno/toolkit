#pragma once

#ifndef TKIT_ENABLE_BLOCK_ALLOCATOR
#    error                                                                                                             \
        "[TOOLKIT][BLOCK-ALLOC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_BLOCK_ALLOCATOR"
#endif

#include "tkit/utils/non_copyable.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/utils/logging.hpp"

namespace TKit
{
// This is a block allocator whose role is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of allocations per block

// On my macOS m1 this allocator is able to allocate 10000 elements of 128 bytes in 0.035 ms and deallocate them in
// 0.012 (3.5ns per allocation and 1.2ns per deallocation). This is roughly a 10x improvement over the default
// new/delete, using the Serial variants. When using the safe variants, latency is doubled (aprox)

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
class BlockAllocator
{
    TKIT_NON_COPYABLE(BlockAllocator)
  public:
    // The alignment parameter specifies the alignment of all consecuent allocations. Thus, the buffer size must be a
    // multiple of the alignment

    BlockAllocator(usize p_BufferSize, usize p_AllocationSize, usize p_Alignment = alignof(std::max_align_t)) noexcept;

    // This constructor is NOT owning the buffer, so it will not deallocate it. Up to the user to manage the memory
    BlockAllocator(void *p_Buffer, usize p_BufferSize, usize p_AllocationSize) noexcept;
    ~BlockAllocator() noexcept;

    BlockAllocator(BlockAllocator &&p_Other) noexcept;
    BlockAllocator &operator=(BlockAllocator &&p_Other) noexcept;

    /**
     * @brief Create a block allocator suited to allocate elements from type `T`.
     *
     * @tparam T The type the allocator will be suited for.
     * @param p_N The capacity of the allocator, measured in how many objects of type `T` will be able to allocate.
     * @return A `BlockAllocator` instance.
     */
    template <typename T> static BlockAllocator CreateFromType(const usize p_N) noexcept
    {
        const usize size = sizeof(T) > sizeof(Allocation) ? sizeof(T) : sizeof(Allocation);
        return BlockAllocator{p_N * size, size, alignof(T)};
    }

    /**
     * @brief Allocate a new block of memory into the block allocator.
     *
     * This allocation has a fixed size. Attempting to use more memory than the block size will result in udefined
     * behaviour.
     *
     * @return A pointer to the allocated block.
     */
    void *Allocate() noexcept;

    /**
     * @brief Allocate a new block of memory into the block allocator.
     *
     * This allocation has a fixed size. Attempting to use more memory than the block size will result in udefined
     * behaviour.
     *
     * @tparam T The type of object to allocate.
     * @return A pointer to the allocated block.
     */
    template <typename T> T *Allocate() noexcept
    {
        TKIT_ASSERT(sizeof(T) <= m_AllocationSize, "[TOOLKIT][BLOCK-ALLOC] Block allocator cannot fit {} bytes!",
                    sizeof(T));
        return static_cast<T *>(Allocate());
    }

    /**
     * @brief Deallocate a block of memory from the block allocator.
     *
     * @param p_Ptr A pointer to the block to deallocate.
     */
    void Deallocate(void *p_Ptr) noexcept;

    /**
     * @brief Allocate a new block of memory and create a new object of type `T` out of it.
     *
     * @tparam T The type of the block.
     * @return A pointer to the allocated block.
     */
    template <typename T, typename... Args> T *Create(Args &&...p_Args) noexcept
    {
        T *ptr = Allocate<T>();
        return Memory::Construct(ptr, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Deallocate a block of memory and destroy the object of type `T` created from it.
     *
     * @tparam T The type of the block.
     * @param p_Ptr The pointer to the block to deallocate.
     */
    template <typename T> void Destroy(T *p_Ptr) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        Deallocate(p_Ptr);
    }

    void Reset() noexcept;

    /**
     * @brief Check if a pointer belongs to the block allocator.
     *
     * @note This is a simple check to see if the provided pointer lies within the boundaries of the buffer. It will not
     * be able to determine if the pointer is currently allocated or free.
     *
     * @param p_Ptr The pointer to check.
     * @return Whether the pointer belongs to the block allocator.
     */
    bool Belongs(const void *p_Ptr) const noexcept;

    bool IsEmpty() const noexcept;
    bool IsFull() const noexcept;

    usize GetBufferSize() const noexcept;
    usize GetAllocationSize() const noexcept;

    usize GetAllocationCount() const noexcept;
    usize GetRemainingCount() const noexcept;
    usize GetAllocationCapacityCount() const noexcept;

  private:
    struct Allocation
    {
        Allocation *Next;
    };

    void setupMemoryLayout() noexcept;
    void deallocateBuffer() noexcept;

    std::byte *m_Buffer;
    Allocation *m_FreeList;
    usize m_BufferSize;
    usize m_AllocationSize;
    u32 m_Allocations = 0;
    bool m_Provided;
};

} // namespace TKit
