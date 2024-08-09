#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include "kit/interface/non_copyable.hpp"
#include "kit/memory/memory.hpp"
#include "kit/logging/logging.hpp"
#include "kit/core/concepts.hpp"
#include <shared_mutex>
#include <mutex>
#include <atomic>

KIT_NAMESPACE_BEGIN

#ifdef KIT_BLOCK_ALLOCATOR_THREAD_SAFE
#    define KIT_BLOCK_ALLOCATOR_UNIQUE_LOCK(mutex) std::scoped_lock lock(mutex)
#    define KIT_BLOCK_ALLOCATOR_SHARED_LOCK(mutex) std::shared_lock lock(mutex)
#else
#    define KIT_BLOCK_ALLOCATOR_UNIQUE_LOCK(mutex)
#    define KIT_BLOCK_ALLOCATOR_SHARED_LOCK(mutex)
#endif

// This is a block allocator whose role is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of chunks per block (each chunk represents an allocated object)
template <typename T> class KIT_API BlockAllocator final
{
    KIT_NON_COPYABLE(BlockAllocator);

  public:
    explicit BlockAllocator(const usz p_ChunksPerBlock) KIT_NOEXCEPT : m_BlockSize(alignedSize() * p_ChunksPerBlock)
    {
    }

    BlockAllocator(BlockAllocator &&p_Other) noexcept
        : m_Allocations(p_Other.m_Allocations), m_BlockSize(p_Other.m_BlockSize), m_FreeList(p_Other.m_FreeList),
          m_BlockTail(p_Other.m_BlockTail)
    {
        p_Other.m_Allocations = 0;
        p_Other.m_FreeList = nullptr;
        p_Other.m_BlockTail = nullptr;
    }

    ~BlockAllocator() KIT_NOEXCEPT
    {
        // The destruction of the block allocator should happen serially
        Block *blockTail = m_BlockTail.load(std::memory_order_acquire);
        while (blockTail)
        {
            Block *prev = blockTail->Prev;
            DeallocateAligned(blockTail->Data);
            delete blockTail;
            blockTail = prev;
        }
    }

    BlockAllocator &operator=(BlockAllocator &&p_Other) noexcept
    {
        if (this != &p_Other)
        {
            m_Allocations = p_Other.m_Allocations;
            m_BlockSize = p_Other.m_BlockSize;
            m_FreeList = p_Other.m_FreeList;
            m_BlockTail = p_Other.m_BlockTail;

            p_Other.m_Allocations = 0;
            p_Other.m_FreeList = nullptr;
            p_Other.m_BlockTail = nullptr;
        }
        return *this;
    }

    T *Allocate() KIT_NOEXCEPT
    {
        m_Allocations.fetch_add(1, std::memory_order_relaxed);
        return allocateCAS(m_FreeList.load(std::memory_order_acquire));
    }

    void Deallocate(T *p_Ptr) KIT_NOEXCEPT
    {
        KIT_ASSERT(!Empty(), "The current allocator has no active allocations yet");
        KIT_ASSERT(Owns(p_Ptr), "Trying to deallocate a pointer that was not allocated by this allocator");

        m_Allocations.fetch_sub(1, std::memory_order_relaxed);
        Chunk *chunk = reinterpret_cast<Chunk *>(p_Ptr);
        Chunk *freeList = m_FreeList.load(std::memory_order_acquire);
        while (true)
        {
            chunk->Next = freeList;
            if (m_FreeList.compare_exchange_weak(freeList, chunk, std::memory_order_release, std::memory_order_acquire))
                return;
        }
    }

    template <typename... Args> T *Construct(Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = Allocate();
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    void Destroy(T *p_Ptr) KIT_NOEXCEPT
    {
        // Be wary! destructor must be thread safe
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        Deallocate(p_Ptr);
    }

    // This method is not infallible. Deallocated pointers from this allocator will still return true, as they lay in
    // the memory block of the allocator
    bool Owns(const T *p_Ptr) const KIT_NOEXCEPT
    {
        const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
        Block *block = m_BlockTail.load(std::memory_order_acquire);
        while (block)
        {
            if (ptr >= block->Data && ptr < block->Data + m_BlockSize)
                return true;
            block = block->Prev;
        }
    }

    usz BlockSize() const KIT_NOEXCEPT
    {
        return m_BlockSize;
    }
    usz BlockCount() const KIT_NOEXCEPT
    {
        return m_BlockCount.load(std::memory_order_relaxed);
    }

    usz ChunksPerBlock() const KIT_NOEXCEPT
    {
        return m_BlockSize / alignedSize();
    }
    usz ChunkSize() const KIT_NOEXCEPT
    {
        return alignedSize();
    }

    bool Empty() const KIT_NOEXCEPT
    {
        return m_Allocations.load(std::memory_order_relaxed) == 0;
    }
    u32 Allocations() const KIT_NOEXCEPT
    {
        return m_Allocations.load(std::memory_order_relaxed);
    }

  private:
    // Important: A chunk must be at least sizeof(void *), so allocations have a minimum size of sizeof(void *)
    struct Chunk
    {
        Chunk *Next = nullptr;
    };
    struct Block
    {
        std::byte *Data = nullptr;
        Block *Prev = nullptr;
    };

    T *allocateCAS(Chunk *p_FreeList) KIT_NOEXCEPT
    {
        // This while loop attempts to allocate from an existing block
        while (p_FreeList)
            if (m_FreeList.compare_exchange_weak(p_FreeList, p_FreeList->Next, std::memory_order_release,
                                                 std::memory_order_acquire))
                return reinterpret_cast<T *>(p_FreeList);
        // If, at some point in time, the free list is empty, we allocate a new block

        const usz chunkSize = alignedSize();
        const usz align = alignment();

        std::byte *data = reinterpret_cast<std::byte *>(AllocateAligned(m_BlockSize, align));
        const usz chunksPerBlock = m_BlockSize / chunkSize;
        for (usz i = 0; i < chunksPerBlock - 1; ++i)
        {
            Chunk *chunk = reinterpret_cast<Chunk *>(data + i * chunkSize);
            chunk->Next = reinterpret_cast<Chunk *>(data + (i + 1) * chunkSize);
        }
        Chunk *last = reinterpret_cast<Chunk *>(data + (chunksPerBlock - 1) * chunkSize);
        last->Next = nullptr;

        // Once the block has been allocated, much stuff may have happened in between. We need to check if the free list
        // is still nullptr, and if it is, we set it to the new block. If it is not, we have to revert the allocation
        // and try again
        if (Chunk *possibleFreeList = reinterpret_cast<Chunk *>(data + chunkSize); !m_FreeList.compare_exchange_strong(
                p_FreeList, possibleFreeList, std::memory_order_release, std::memory_order_acquire))
        {
            DeallocateAligned(data);
            return allocateCAS(p_FreeList);
        }

        // Free list is updated, so that is sorted. Now we need to create and add the new block to the list of blocks
        // This use of new is not that great
        Block *newBlock = new Block;
        newBlock->Data = data;
        Block *blockTail = m_BlockTail.load(std::memory_order_acquire);
        while (true)
        {
            newBlock->Prev = blockTail;
            if (m_BlockTail.compare_exchange_weak(blockTail, newBlock, std::memory_order_release,
                                                  std::memory_order_acquire))
                reinterpret_cast<T *>(data);
        }

        // Unreachable lol
        return nullptr;
    }

    constexpr usz alignedSize() const KIT_NOEXCEPT
    {
        if constexpr (sizeof(T) < sizeof(Chunk))
            return AlignedSize<Chunk>();
        else
            return AlignedSize<T>();
    }

    constexpr usz alignment() const KIT_NOEXCEPT
    {
        if constexpr (alignof(T) < alignof(Chunk))
            return alignof(Chunk);
        else
            return alignof(T);
    }

    std::atomic<u32> m_Allocations = 0;
    std::atomic<u32> m_BlockCount = 0;
    std::atomic<Block *> m_BlockTail = nullptr;
    std::atomic<Chunk *> m_FreeList = nullptr;
    usz m_BlockSize;
};

KIT_NAMESPACE_END

#ifndef KIT_DISABLE_BLOCK_ALLOCATOR
#    define KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)                                                         \
        static inline KIT::BlockAllocator<p_ClassName> s_Allocator{p_ChunksPerBlock};                                  \
        void *operator new(usz p_Size)                                                                                 \
        {                                                                                                              \
            KIT_ASSERT(p_Size == sizeof(p_ClassName),                                                                  \
                       "Trying to block allocate a derived class from a base class overloaded new/delete");            \
            return s_Allocator.Allocate();                                                                             \
        }                                                                                                              \
        void operator delete(void *p_Ptr)                                                                              \
        {                                                                                                              \
            s_Allocator.Deallocate(reinterpret_cast<p_ClassName *>(p_Ptr));                                            \
        }
#else
#    define KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)
#endif