#pragma once

#include "tkit/core/alias.hpp"
#include "tkit/core/non_copyable.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/core/logging.hpp"
#include "tkit/core/concepts.hpp"
#include "tkit/multiprocessing/spin_lock.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/container/alias.hpp"
#include <mutex>

namespace TKit
{
// This is a block allocator whose role is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of chunks per block (each chunk represents an allocated object)

// On my macOS m1 this allocator is able to allocate 10000 elements of 128 bytes in 0.035 ms and deallocate them in
// 0.012 (3.5ns per allocation and 1.2ns per deallocation). This is roughly a 10x improvement over the default
// new/delete, using the Serial variants. When using the safe variants, latency is doubled (aprox)

/**
 * @brief A block allocator that allocates memory in blocks of a fixed size.
 *
 * Each block consists of a fixed number of chunks, each chunk being the size of the type `T`. The allocator allocates
 * new blocks when the current block is full, and manages a free list of chunks that have been deallocated. The
 * allocator has a thread-safe and a serial variant, the latter being faster.
 *
 * The block allocator deallocates all memory when it is destroyed. It is up to the user to ensure that all memory is
 * freed at that point, especially when dealing with non-trivial destructors.
 *
 * Some performance numbers (measured on my macOS M1):
 * - Allocating 10000 elements of 128 bytes in 0.035 ms (3.5 ns per allocation)
 * - Deallocating 10000 elements of 128 bytes in 0.012 ms (1.2 ns per deallocation)
 *
 * This is roughly a 10x improvement over the default new/delete, using the serial variants. When using the concurrent
 * variants, latency is doubled (aprox)
 *
 * @tparam T
 */
template <typename T> class TKIT_API BlockAllocator
{
    TKIT_NON_COPYABLE(BlockAllocator)
  public:
    explicit BlockAllocator(const usize p_ChunksPerBlock) noexcept : m_BlockSize(GetChunkSize() * p_ChunksPerBlock)
    {
    }

    BlockAllocator(BlockAllocator &&p_Other) noexcept
        : m_Allocations(p_Other.m_Allocations), m_FreeList(p_Other.m_FreeList), m_Blocks(std::move(p_Other.m_Blocks)),
          m_BlockSize(p_Other.m_BlockSize)
    {
        p_Other.m_Allocations = 0;
        p_Other.m_FreeList = nullptr;
        p_Other.m_Blocks.clear();
    }

    ~BlockAllocator() noexcept
    {
        Reset();
    }

    BlockAllocator &operator=(BlockAllocator &&p_Other) noexcept
    {
        if (this != &p_Other)
        {
            m_Allocations = p_Other.m_Allocations;
            m_FreeList = p_Other.m_FreeList;
            m_Blocks = std::move(p_Other.m_Blocks);
            m_BlockSize = p_Other.m_BlockSize;

            p_Other.m_Allocations = 0;
            p_Other.m_FreeList = nullptr;
            p_Other.m_Blocks.clear();
        }
        return *this;
    }

    /**
     * @brief Get the size of the allocated blocks in bytes.
     *
     */
    usize GetBlockSize() const noexcept
    {
        return m_BlockSize;
    }

    /**
     * @brief Get the number of chunks per block (the number of objects of type `T` that fit each block).
     *
     */
    usize GetChunksPerBlock() const noexcept
    {
        return m_BlockSize / GetChunkSize();
    }

    /**
     * @brief Get the size of a chunk in bytes.
     *
     */
    static TKIT_CONSTEVAL usize GetChunkSize() noexcept
    {
        if constexpr (sizeof(T) < sizeof(Chunk))
            return TKIT_SIZE_OF(Chunk);
        else
            return TKIT_SIZE_OF(T);
    }

    /**
     * @brief Get the alignment of a chunk in bytes.
     *
     */
    static TKIT_CONSTEVAL usize GetChunkAlignment() noexcept
    {
        if constexpr (alignof(T) < alignof(Chunk))
            return TKIT_ALIGN_OF(Chunk);
        else
            return TKIT_ALIGN_OF(T);
    }

    /**
     * @brief Create an object of type `T` in the allocator.
     *
     * The object is constructed with the provided arguments. The memory's object is allocated in a thread-safe manner.
     * The user must ensure that the object's constructor is thread-safe.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A pointer to the newly created object.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]] T *CreateConcurrent(Args &&...p_Args) noexcept
    {
        T *ptr = AllocateConcurrent();
        return Memory::Construct(ptr, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Destroy an object of type `T` in the allocator.
     *
     * The object is destroyed and the memory deallocated in a thread-safe manner. The user must ensure that the
     * object's destructor is thread-safe.
     *
     * @param p_Ptr A pointer to the object to destroy.
     */
    void DestroyConcurrent(T *p_Ptr) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        DeallocateConcurrent(p_Ptr);
    }

    /**
     * @brief Create an object of type `T` in the allocator.
     *
     * The object is constructed with the provided arguments. The memory's object is allocated in a serial manner with
     * no thread-safety guarantees.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A pointer to the newly created object.
     */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]] T *CreateSerial(Args &&...p_Args) noexcept
    {
        T *ptr = AllocateSerial();
        return Memory::Construct(ptr, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Destroy an object of type `T` in the allocator.
     *
     * The object is destroyed and the memory deallocated with no thread-safety guarantees.
     *
     * @param p_Ptr A pointer to the object to destroy.
     */
    void DestroySerial(T *p_Ptr) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        DeallocateSerial(p_Ptr);
    }

    /**
     * @brief Allocate memory for an object of type `T` in a thread-safe manner.
     *
     * @return A pointer to the allocated memory.
     */
    [[nodiscard]] T *AllocateConcurrent() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        return AllocateSerial();
    }

    /**
     * @brief Deallocate memory for an object of type `T` in a thread-safe manner.
     *
     * @param p_Ptr A pointer to the memory to deallocate.
     */
    void DeallocateConcurrent(T *p_Ptr) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        DeallocateSerial(p_Ptr);
    }

    /**
     * @brief Allocate memory for an object of type `T` with no thread-safety guarantees.
     *
     * @return A pointer to the allocated memory.
     */
    [[nodiscard]] T *AllocateSerial() noexcept
    {
        ++m_Allocations;
        if (m_FreeList)
            return fromNextFreeChunk();
        return fromFirstChunkOfNewBlock();
    }

    /**
     * @brief Deallocate memory for an object of type `T` with no thread-safety guarantees.
     *
     * @param p_Ptr A pointer to the memory to deallocate.
     */
    void DeallocateSerial(T *p_Ptr) noexcept
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT] The current allocator has no active allocations yet");
        TKIT_ASSERT(Owns(p_Ptr), "[TOOLKIT] Trying to deallocate a pointer that was not allocated by this allocator");

        --m_Allocations;
        Chunk *chunk = reinterpret_cast<Chunk *>(p_Ptr);
        chunk->Next = m_FreeList;
        m_FreeList = chunk;
    }

    /**
     * @brief Reserve memory for a new block of chunks in a thread-safe manner.
     *
     * If the allocator already has a free chunk available, this method does nothing.
     *
     */
    void ReserveConcurrent()
    {
        if (m_FreeList)
            return;
        std::scoped_lock lock(m_Mutex);
        TKIT_PROFILE_MARK_LOCK(m_Mutex);
        std::byte *data = allocateNewBlock(m_BlockSize);
        m_FreeList = reinterpret_cast<Chunk *>(data);
        m_Blocks.push_back(data);
    }

    /**
     * @brief Reserve memory for a new block of chunks with no thread-safety guarantees.
     *
     * If the allocator already has a free chunk available, this method does nothing.
     *
     */
    void ReserveSerial()
    {
        if (m_FreeList)
            return;
        std::byte *data = allocateNewBlock(m_BlockSize);
        m_FreeList = reinterpret_cast<Chunk *>(data);
        m_Blocks.push_back(data);
    }

    /**
     * @brief Deallocate all memory in the allocator.
     *
     */
    void Reset()
    {
        TKIT_LOG_WARNING_IF(!IsEmpty(),
                            "[TOOLKIT] The current allocator has active allocations. Resetting the allocator will "
                            "prematurely deallocate all memory, and no destructor will be called");
        for (std::byte *block : m_Blocks)
            Memory::DeallocateAlignedPlatformSpecific(block);
        m_Allocations = 0;
        m_FreeList = nullptr;
        m_Blocks.clear();
    }

    /**
     * @brief Check if a pointer belongs to this allocator.
     *
     * This method is not infallible, as deallocated pointers from this allocator will still return true, as they lay in
     * the memory block of the allocator.
     *
     * @param p_Ptr A pointer to check.
     * @return Whether the pointer belongs to this allocator.
     */
    bool Owns(const T *p_Ptr) const noexcept
    {
        const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
        for (const std::byte *block : m_Blocks)
            if (ptr >= block && ptr < block + this->GetBlockSize())
                return true;
        return false;
    }

    /**
     * @brief Get the number of blocks allocated by this allocator.
     *
     */
    usize GetBlockCount() const noexcept
    {
        return m_Blocks.size();
    }

    /**
     * @brief Check if the allocator is empty (no active allocations).
     *
     */
    bool IsEmpty() const noexcept
    {
        return m_Allocations == 0;
    }

    /**
     * @brief Get the number of active allocations in the allocator.
     *
     */
    u32 GetAllocations() const noexcept
    {
        return m_Allocations;
    }

  private:
    // Important: An allocated piece (chunk) must be at least sizeof(void *), so an object of size chunk can fit
    struct Chunk
    {
        Chunk *Next = nullptr;
    };

    static std::byte *allocateNewBlock(const usize p_BlockSize) noexcept
    {
        constexpr usize chunkSize = GetChunkSize();

        // With AllocateAligned, I dont need to worry about the alignment of the block itself, or subsequent individual
        // elements. They are of the same type, so adress + n * sizeof(T) will always be aligned if the start adress is
        // aligned (guaranteed by AllocateAligned)
        std::byte *data =
            static_cast<std::byte *>(Memory::AllocateAlignedPlatformSpecific(p_BlockSize, GetChunkAlignment()));

        const usize chunksPerBlock = p_BlockSize / chunkSize;
        for (usize i = 0; i < chunksPerBlock - 1; ++i)
        {
            Chunk *chunk = reinterpret_cast<Chunk *>(data + i * chunkSize);
            chunk->Next = reinterpret_cast<Chunk *>(data + (i + 1) * chunkSize);
        }
        Chunk *last = reinterpret_cast<Chunk *>(data + (chunksPerBlock - 1) * chunkSize);
        last->Next = nullptr;
        return data;
    }

    T *fromFirstChunkOfNewBlock() noexcept
    {
        std::byte *data = allocateNewBlock(m_BlockSize);
        m_FreeList = reinterpret_cast<Chunk *>(data + GetChunkSize());
        m_Blocks.push_back(data);
        return reinterpret_cast<T *>(data);
    }

    T *fromNextFreeChunk() noexcept
    {
        Chunk *chunk = m_FreeList;
        m_FreeList = chunk->Next;
        return reinterpret_cast<T *>(chunk);
    }

    DynamicArray<std::byte *> m_Blocks;
    Chunk *m_FreeList = nullptr;
    usize m_BlockSize;
    u32 m_Allocations = 0;
    TKIT_PROFILE_DECLARE_MUTEX(SpinLock, m_Mutex);
};

/**
 * @brief Get the global instance of a block allocator for type `T` with a fixed number of chunks per block.
 *
 * The instance is created the first time this function is called.
 *
 * @tparam T The type of the objects to allocate.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @return A reference to the global instance of the block allocator.
 */
template <typename T, usize ChunksPerBlock> BlockAllocator<T> &GetGlobalBlockAllocatorInstance() noexcept
{
    static BlockAllocator<T> allocator{ChunksPerBlock};
    return allocator;
}

/**
 * @brief Allocate memory for an object of type `T` in a thread-safe manner using the global block allocator for type
 * `T` with a fixed number of chunks per block.
 *
 * @tparam T The type of the object to allocate.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @return A pointer to the allocated memory.
 */
template <typename T, usize ChunksPerBlock> T *BAllocateConcurrent() noexcept
{
    return GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().AllocateConcurrent();
}

/**
 * @brief Deallocate memory for an object of type `T` in a thread-safe manner using the global block allocator for type
 * `T` with a fixed number of chunks per block.
 *
 * @tparam T The type of the object to deallocate.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @param p_Ptr A pointer to the memory to deallocate.
 */
template <typename T, usize ChunksPerBlock> void BDeallocateConcurrent(T *p_Ptr) noexcept
{
    GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().DeallocateConcurrent(p_Ptr);
}

/**
 * @brief Allocate memory for an object of type `T` with no thread-safety guarantees using the global block allocator
 * for type `T` with a fixed number of chunks per block.
 *
 * @tparam T The type of the object to allocate.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @return A pointer to the allocated memory.
 */
template <typename T, usize ChunksPerBlock> T *BAllocateSerial() noexcept
{
    return GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().AllocateSerial();
}

/**
 * @brief Deallocate memory for an object of type `T` with no thread-safety guarantees using the global block allocator
 * for type `T` with a fixed number of chunks per block.
 *
 * @tparam T The type of the object to deallocate.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @param p_Ptr A pointer to the memory to deallocate.
 */
template <typename T, usize ChunksPerBlock> void BDeallocateSerial(T *p_Ptr) noexcept
{
    GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().DeallocateSerial(p_Ptr);
}

/**
 * @brief Create an object of type `T` in the global block allocator for type `T` with a fixed number of chunks per
 * block.
 *
 * The object is constructed with the provided arguments. The memory's object is allocated in a thread-safe manner. The
 * user must ensure that the object's constructor is thread-safe.
 *
 * @tparam T The type of the object to create.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @param p_Args The arguments to pass to the constructor of `T`.
 * @return A pointer to the newly created object.
 */
template <typename T, usize ChunksPerBlock, typename... Args> T *BCreate(Args &&...p_Args) noexcept
{
    return GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().CreateConcurrent(std::forward<Args>(p_Args)...);
}

/**
 * @brief Destroy an object of type `T` in the global block allocator for type `T` with a fixed number of chunks per
 * block.
 *
 *
 * The object is destroyed and the memory deallocated in a thread-safe manner. The user must ensure that the object's
 * destructor is thread-safe.
 *
 * @tparam T The type of the object to destroy.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @param p_Ptr A pointer to the object to destroy.
 */
template <typename T, usize ChunksPerBlock> void BDestroy(T *p_Ptr) noexcept
{
    GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().DestroyConcurrent(p_Ptr);
}

/**
 * @brief Create an object of type `T` in the global block allocator for type `T` with a fixed number of chunks per
 * block.
 *
 *
 * The object is constructed with the provided arguments. The memory's object is allocated in a serial manner with no
 * thread-safety guarantees.
 *
 * @tparam T The type of the object to create.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @param p_Args The arguments to pass to the constructor of `T`.
 * @return A pointer to the newly created object.
 */
template <typename T, usize ChunksPerBlock, typename... Args> T *BCreateSerial(Args &&...p_Args) noexcept
{
    return GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().CreateSerial(std::forward<Args>(p_Args)...);
}

/**
 * @brief Destroy an object of type `T` in the global block allocator for type `T` with a fixed number of chunks per
 * block.
 *
 *
 * The object is destroyed and the memory deallocated with no thread-safety guarantees.
 *
 * @tparam T The type of the object to destroy.
 * @tparam ChunksPerBlock The number of chunks per block.
 * @param p_Ptr A pointer to the object to destroy.
 */
template <typename T, usize ChunksPerBlock> void BDestroySerial(T *p_Ptr) noexcept
{
    GetGlobalBlockAllocatorInstance<T, ChunksPerBlock>().DestroySerial(p_Ptr);
}
} // namespace TKit

#define TKIT_BLOCK_ALLOCATED_CONCURRENT(p_ClassName, p_ChunksPerBlock)                                                 \
    static void *operator new([[maybe_unused]] size_t p_Size)                                                          \
    {                                                                                                                  \
        TKIT_ASSERT(p_Size == TKIT_SIZE_OF(p_ClassName),                                                               \
                    "[TOOLKIT] Trying to block allocate a derived class from a base class overloaded new/delete");     \
        return TKit::BAllocateConcurrent<p_ClassName, p_ChunksPerBlock>();                                             \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr)                                                                           \
    {                                                                                                                  \
        TKit::BDeallocateConcurrent<p_ClassName, p_ChunksPerBlock>(static_cast<p_ClassName *>(p_Ptr));                 \
    }                                                                                                                  \
    static void *operator new([[maybe_unused]] size_t p_Size, const std::nothrow_t &)                                  \
    {                                                                                                                  \
        TKIT_ASSERT(p_Size == TKIT_SIZE_OF(p_ClassName),                                                               \
                    "[TOOLKIT] Trying to block allocate a derived class from a base class overloaded new/delete");     \
        return TKit::BAllocateConcurrent<p_ClassName, p_ChunksPerBlock>();                                             \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr, const std::nothrow_t &)                                                   \
    {                                                                                                                  \
        TKit::BDeallocateConcurrent<p_ClassName, p_ChunksPerBlock>(static_cast<p_ClassName *>(p_Ptr));                 \
    }

#define TKIT_BLOCK_ALLOCATED_SERIAL(p_ClassName, p_ChunksPerBlock)                                                     \
    static void *operator new([[maybe_unused]] size_t p_Size)                                                          \
    {                                                                                                                  \
        TKIT_ASSERT(p_Size == TKIT_SIZE_OF(p_ClassName),                                                               \
                    "[TOOLKIT] Trying to block allocate a derived class from a base class overloaded new/delete");     \
        return TKit::BAllocateSerial<p_ClassName, p_ChunksPerBlock>();                                                 \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr)                                                                           \
    {                                                                                                                  \
        TKit::BDeallocateSerial<p_ClassName, p_ChunksPerBlock>(static_cast<p_ClassName *>(p_Ptr));                     \
    }                                                                                                                  \
    static void *operator new([[maybe_unused]] size_t p_Size, const std::nothrow_t &)                                  \
    {                                                                                                                  \
        TKIT_ASSERT(p_Size == TKIT_SIZE_OF(p_ClassName),                                                               \
                    "[TOOLKIT] Trying to block allocate a derived class from a base class overloaded new/delete");     \
        return TKit::BAllocateSerial<p_ClassName, p_ChunksPerBlock>();                                                 \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr, const std::nothrow_t &)                                                   \
    {                                                                                                                  \
        TKit::BDeallocateSerial<p_ClassName, p_ChunksPerBlock>(static_cast<p_ClassName *>(p_Ptr));                     \
    }
