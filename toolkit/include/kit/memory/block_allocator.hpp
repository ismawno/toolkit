#pragma once

#include "kit/core/alias.hpp"
#include "kit/core/non_copyable.hpp"
#include "kit/memory/memory.hpp"
#include "kit/core/logging.hpp"
#include "kit/core/concepts.hpp"
#include "kit/multiprocessing/spin_mutex.hpp"
#include <mutex>

namespace KIT
{
// This is a block allocator whose role is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of chunks per block (each chunk represents an allocated object)

// On my macOS m1 this allocator is able to allocate 10000 elements of 128 bytes in 0.035 ms and deallocate them in
// 0.012 (3.5ns per allocation and 1.2ns per deallocation). This is roughly a 10x improvement over the default
// new/delete, using the unsafe variants. When usingthe safe variants, latency is doubled (aprox)

template <typename T> class KIT_API BlockAllocator final
{
    KIT_NON_COPYABLE(BlockAllocator)
  public:
    explicit BlockAllocator(const usize p_ChunksPerBlock) noexcept : m_BlockSize(ChunkSize() * p_ChunksPerBlock)
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

    usize BlockSize() const noexcept
    {
        return m_BlockSize;
    }

    usize ChunksPerBlock() const noexcept
    {
        return m_BlockSize / ChunkSize();
    }

    static KIT_CONSTEVAL usize ChunkSize() noexcept
    {
        if constexpr (sizeof(T) < sizeof(Chunk))
            return AlignedSize<Chunk>();
        else
            return AlignedSize<T>();
    }
    static KIT_CONSTEVAL usize ChunkAlignment() noexcept
    {
        if constexpr (alignof(T) < alignof(Chunk))
            return alignof(Chunk);
        else
            return alignof(T);
    }

    template <typename... Args> [[nodiscard]] T *Create(Args &&...p_Args) noexcept
    {
        T *ptr = Allocate();
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }
    void Destroy(T *p_Ptr) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        Deallocate(p_Ptr);
    }

    template <typename... Args> [[nodiscard]] T *CreateUnsafe(Args &&...p_Args) noexcept
    {
        T *ptr = AllocateUnsafe();
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }
    void DestroyUnsafe(T *p_Ptr) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        DeallocateUnsafe(p_Ptr);
    }

    [[nodiscard]] T *Allocate() noexcept
    {
        std::scoped_lock lock(m_Mutex);
        return AllocateUnsafe();
    }

    void Deallocate(T *p_Ptr) noexcept
    {
        std::scoped_lock lock(m_Mutex);
        DeallocateUnsafe(p_Ptr);
    }

    [[nodiscard]] T *AllocateUnsafe() noexcept
    {
        ++m_Allocations;
        if (m_FreeList)
            return fromNextFreeChunk();
        return fromFirstChunkOfNewBlock();
    }

    void DeallocateUnsafe(T *p_Ptr) noexcept
    {
        KIT_ASSERT(!Empty(), "The current allocator has no active allocations yet");
        KIT_ASSERT(Owns(p_Ptr), "Trying to deallocate a pointer that was not allocated by this allocator");

        --m_Allocations;
        Chunk *chunk = reinterpret_cast<Chunk *>(p_Ptr);
        chunk->Next = m_FreeList;
        m_FreeList = chunk;
    }

    void Reserve()
    {
        if (m_FreeList)
            return;
        std::scoped_lock lock(m_Mutex);
        std::byte *data = allocateNewBlock(m_BlockSize);
        m_FreeList = reinterpret_cast<Chunk *>(data);
        m_Blocks.push_back(data);
    }

    void ReserveUnsafe()
    {
        if (m_FreeList)
            return;
        std::byte *data = allocateNewBlock(m_BlockSize);
        m_FreeList = reinterpret_cast<Chunk *>(data);
        m_Blocks.push_back(data);
    }

    void Reset()
    {
        KIT_LOG_WARNING_IF(!Empty(), "The current allocator has active allocations. Resetting the allocator will "
                                     "prematurely deallocate all memory, and no destructor will be called");
        for (std::byte *block : m_Blocks)
            DeallocateAligned(block);
        m_Allocations = 0;
        m_FreeList = nullptr;
        m_Blocks.clear();
    }

    // This method is not infallible. Deallocated pointers from this allocator will still return true, as they lay in
    // the memory block of the allocator
    bool Owns(const T *p_Ptr) const noexcept
    {
        const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
        for (const std::byte *block : m_Blocks)
            if (ptr >= block && ptr < block + this->BlockSize())
                return true;
        return false;
    }

    usize BlockCount() const noexcept
    {
        return m_Blocks.size();
    }

    bool Empty() const noexcept
    {
        return m_Allocations == 0;
    }
    u32 Allocations() const noexcept
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
        constexpr usize chunkSize = ChunkSize();
        constexpr usize alignment = ChunkAlignment();

        // With AllocateAligned, I dont need to worry about the alignment of the block itself, or subsequent individual
        // elements. They are of the same type, so adress + n * sizeof(T) will always be aligned if the start adress is
        // aligned (guaranteed by AllocateAligned)
        std::byte *data = static_cast<std::byte *>(AllocateAligned(p_BlockSize, alignment));

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
        m_FreeList = reinterpret_cast<Chunk *>(data + ChunkSize());
        m_Blocks.push_back(data);
        return reinterpret_cast<T *>(data);
    }

    T *fromNextFreeChunk() noexcept
    {
        Chunk *chunk = m_FreeList;
        m_FreeList = chunk->Next;
        return reinterpret_cast<T *>(chunk);
    }

    u32 m_Allocations = 0;
    Chunk *m_FreeList = nullptr;
    DynamicArray<std::byte *> m_Blocks;
    usize m_BlockSize;
    SpinMutex m_Mutex;
};

template <typename T, usize ChunksPerBlock> BlockAllocator<T> &GlobalBlockAllocatorInstance() noexcept
{
    static BlockAllocator<T> allocator{ChunksPerBlock};
    return allocator;
}

template <typename T, usize ChunksPerBlock> T *BAllocate() noexcept
{
    return GlobalBlockAllocatorInstance<T, ChunksPerBlock>().Allocate();
}
template <typename T, usize ChunksPerBlock> void BDeallocate(T *p_Ptr) noexcept
{
    GlobalBlockAllocatorInstance<T, ChunksPerBlock>().Deallocate(p_Ptr);
}

template <typename T, usize ChunksPerBlock> T *BAllocateUnsafe() noexcept
{
    return GlobalBlockAllocatorInstance<T, ChunksPerBlock>().AllocateUnsafe();
}
template <typename T, usize ChunksPerBlock> void BDeallocateUnsafe(T *p_Ptr) noexcept
{
    GlobalBlockAllocatorInstance<T, ChunksPerBlock>().DeallocateUnsafe(p_Ptr);
}

template <typename T, usize ChunksPerBlock, typename... Args> T *BCreate(Args &&...p_Args) noexcept
{
    return GlobalBlockAllocatorInstance<T, ChunksPerBlock>().Create(std::forward<Args>(p_Args)...);
}
template <typename T, usize ChunksPerBlock> void BDestroy(T *p_Ptr) noexcept
{
    GlobalBlockAllocatorInstance<T, ChunksPerBlock>().Destroy(p_Ptr);
}

template <typename T, usize ChunksPerBlock, typename... Args> T *BCreateUnsafe(Args &&...p_Args) noexcept
{
    return GlobalBlockAllocatorInstance<T, ChunksPerBlock>().CreateUnsafe(std::forward<Args>(p_Args)...);
}
template <typename T, usize ChunksPerBlock> void BDestroyUnsafe(T *p_Ptr) noexcept
{
    GlobalBlockAllocatorInstance<T, ChunksPerBlock>().DestroyUnsafe(p_Ptr);
}
} // namespace KIT

#define KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)                                                             \
    static void *operator new([[maybe_unused]] usize p_Size)                                                           \
    {                                                                                                                  \
        KIT_ASSERT(p_Size == sizeof(p_ClassName),                                                                      \
                   "Trying to block allocate a derived class from a base class overloaded new/delete");                \
        return KIT::BAllocate<p_ClassName, p_ChunksPerBlock>();                                                        \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr)                                                                           \
    {                                                                                                                  \
        KIT::BDeallocate<p_ClassName, p_ChunksPerBlock>(static_cast<p_ClassName *>(p_Ptr));                            \
    }

#define KIT_BLOCK_ALLOCATED_UNSAFE(p_ClassName, p_ChunksPerBlock)                                                      \
    static void *operator new([[maybe_unused]] usize p_Size)                                                           \
    {                                                                                                                  \
        KIT_ASSERT(p_Size == sizeof(p_ClassName),                                                                      \
                   "Trying to block allocate a derived class from a base class overloaded new/delete");                \
        return KIT::BAllocateUnsafe<p_ClassName, p_ChunksPerBlock>();                                                  \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr)                                                                           \
    {                                                                                                                  \
        KIT::BDeallocateUnsafe<p_ClassName, p_ChunksPerBlock>(static_cast<p_ClassName *>(p_Ptr));                      \
    }
