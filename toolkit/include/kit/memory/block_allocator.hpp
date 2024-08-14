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

// This is a block allocator whose role is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of chunks per block (each chunk represents an allocated object)

// It comes in two flavors: TSafeBlockAllocator and TUnsafeBlockAllocator. The former is a lock-free thread safe
// allocator using CAS, optimized to be used in a multi-threaded environment, while the latter is not thread safe,
// optimized for single-threaded environments

// I have decided to use static polymorphism because both allocator's functionality is exactly the same and I dont see
// how runtime polymorphism in this case could be useful

// -Known data races-
// -r1- Happens when a thread allocates and deallocates an object almost immediately. If, in that case, two threads
// allocate simultaneously and one of them is quicker and deallocates while the second is still allocating (and looking
// for a valid freelist state), you can end up with a data race, where a thread looks at a stale, detached value of the
// freelist while the other one modifies the corresponding chunk to reattach it.

// Both of these cases are inside CAS loops, that will only exit if the freelist state is VALIDATED, so it is okay if in
// the loop, the read counterpart gets a corrupted data, because in that case, it will try again. Because of that, and
// because I have no idea how to solve this without using locks (I am NOT using locks), I will leave it as it is

// NOTE: Data race above can also trigger if the quick thread modifies the data it just allocated
template <typename T, template <typename> typename Derived> class KIT_API BlockAllocator
{
    KIT_NON_COPYABLE(BlockAllocator);

  public:
    explicit BlockAllocator(const usz p_ChunksPerBlock) KIT_NOEXCEPT : m_BlockSize(ChunkSize() * p_ChunksPerBlock)
    {
    }

    usz BlockSize() const KIT_NOEXCEPT
    {
        return m_BlockSize;
    }

    usz ChunksPerBlock() const KIT_NOEXCEPT
    {
        return m_BlockSize / ChunkSize();
    }

    static constexpr usz ChunkSize() KIT_NOEXCEPT
    {
        return Derived<T>::ChunkSize();
    }
    static constexpr usz ChunkAlignment() KIT_NOEXCEPT
    {
        if constexpr (alignof(T) < alignof(Chunk))
            return alignof(Chunk);
        else
            return alignof(T);
    }

    T *Allocate() KIT_NOEXCEPT
    {
        return static_cast<Derived<T> *>(this)->Allocate();
    }
    void Deallocate(T *p_Ptr) KIT_NOEXCEPT
    {
        static_cast<Derived<T> *>(this)->Deallocate(p_Ptr);
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

    void Reset() KIT_NOEXCEPT
    {
        static_cast<Derived<T> *>(this)->Reset();
    }

    bool Owns(const T *p_Ptr) const KIT_NOEXCEPT
    {
        return static_cast<const Derived<T> *>(this)->Owns(p_Ptr);
    }

    usz BlockCount() const KIT_NOEXCEPT
    {
        return static_cast<const Derived<T> *>(this)->BlockCount();
    }

    bool Empty() const KIT_NOEXCEPT
    {
        return static_cast<const Derived<T> *>(this)->Empty();
    }

    u32 Allocations() const KIT_NOEXCEPT
    {
        return static_cast<const Derived<T> *>(this)->Allocations();
    }

  protected:
    struct Chunk
    {
        Chunk *Next = nullptr;
    };

  private:
    usz m_BlockSize;
};

template <typename T> class KIT_API TSafeBlockAllocator final : public BlockAllocator<T, TSafeBlockAllocator>
{
  public:
    explicit TSafeBlockAllocator(const usz p_ChunksPerBlock) KIT_NOEXCEPT
        : BlockAllocator<T, TSafeBlockAllocator>(p_ChunksPerBlock)
    {
    }

    TSafeBlockAllocator(TSafeBlockAllocator &&p_Other) noexcept
        : BlockAllocator<T, TSafeBlockAllocator>(std::move(p_Other))
    {
        m_Allocations.store(p_Other.m_Allocations.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_BlockChunkData.store(p_Other.m_BlockChunkData.load(std::memory_order_relaxed), std::memory_order_relaxed);

        p_Other.m_Allocations.store(0, std::memory_order_relaxed);
        p_Other.m_BlockChunkData.store({}, std::memory_order_relaxed);
    }

    ~TSafeBlockAllocator() KIT_NOEXCEPT
    {
        // The destruction of the block allocator should happen serially
        Reset();
    }

    TSafeBlockAllocator &operator=(TSafeBlockAllocator &&p_Other) noexcept
    {
        if (this != &p_Other)
        {
            BlockAllocator<T, TSafeBlockAllocator>::operator=(std::move(p_Other));
            m_Allocations.store(p_Other.m_Allocations.load(std::memory_order_relaxed), std::memory_order_relaxed);
            m_BlockChunkData.store(p_Other.m_BlockChunkData.load(std::memory_order_relaxed), std::memory_order_relaxed);

            p_Other.m_Allocations.store(0, std::memory_order_relaxed);
            p_Other.m_BlockChunkData.store({}, std::memory_order_relaxed);
        }
        return *this;
    }

    // In this implementation, the chunk size is slightly bigger than the size of the object because chunk metadata must
    // be allocated in a different and exclusive memory zone. This is extremely important, as if the chunk metadata
    // shared the "space" with the object memory (similar to an enum), because of the "wild" nature of amultithreaded
    // environment, the user could indirectly invalidate the data the FreeList points to, causing a lot of issues, such
    // as segfaults and nasty data races
    static constexpr usz ChunkSize() KIT_NOEXCEPT
    {
        return AlignedSize<Chunk>() + AlignedSize<T>();
    }

    T *Allocate() KIT_NOEXCEPT
    {
        m_Allocations.fetch_add(1, std::memory_order_relaxed);
        return allocateCAS(m_BlockChunkData.load(std::memory_order_acquire));
    }

    void Deallocate(T *p_Ptr) KIT_NOEXCEPT
    {
        KIT_ASSERT(!Empty(), "The current allocator has no active allocations yet");
        KIT_ASSERT(Owns(p_Ptr), "Trying to deallocate a pointer that was not allocated by this allocator");

        m_Allocations.fetch_sub(1, std::memory_order_relaxed);

        std::byte *ptr = reinterpret_cast<std::byte *>(p_Ptr);
        Chunk *chunk = reinterpret_cast<Chunk *>(ptr + AlignedSize<T>());

        BlockChunkData oldData = m_BlockChunkData.load(std::memory_order_acquire);
        while (true)
        {
            i32 counter = m_AllocDeallocCounter.load(std::memory_order_acquire);
            if (counter <= 0 &&
                m_AllocDeallocCounter.compare_exchange_weak(counter, counter - 1, std::memory_order_acq_rel))
                break;
        }

        BlockChunkData newData;
        do
        {
            // -r1- This is a data race (write), with its counterpart located in allocateCAS (read). More on this above
            chunk->Next = oldData.FreeList;
            newData = {oldData.BlockTail, chunk};
        } while (!m_BlockChunkData.compare_exchange_weak(oldData, newData, std::memory_order_release,
                                                         std::memory_order_relaxed));
        m_AllocDeallocCounter.fetch_add(1, std::memory_order_release);
    }

    void Reset()
    {
        KIT_LOG_WARNING_IF(!Empty(), "The current allocator has active allocations. Resetting the allocator will "
                                     "prematurely deallocate all memory, and no destructor will be called");
        // The destruction/reset of the block allocator should happen serially
        Block *blockTail = m_BlockChunkData.load(std::memory_order_relaxed).BlockTail;
        while (blockTail)
        {
            Block *prev = blockTail->Prev;
            DeallocateAligned(blockTail->Data);
            delete blockTail;
            blockTail = prev;
        }
        m_BlockChunkData.store({}, std::memory_order_relaxed);
        m_Allocations.store(0, std::memory_order_relaxed);
        m_BlockCount.store(0, std::memory_order_relaxed);
    }

    // This method is not infallible. Deallocated pointers from this allocator will still return true, as they lay in
    // the memory block of the allocator
    bool Owns(const T *p_Ptr) const KIT_NOEXCEPT
    {
        const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
        Block *block = m_BlockChunkData.load(std::memory_order_acquire).BlockTail;
        while (block)
        {
            if (ptr >= block->Data && ptr < block->Data + this->BlockSize())
                return true;
            block = block->Prev;
        }
        return false;
    }

    usz BlockCount() const KIT_NOEXCEPT
    {
        return m_BlockCount.load(std::memory_order_relaxed);
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
    // Important: A chunk must be at least sizeof(void *), so Chunk objects fit in the memory block of the allocator
    using Chunk = typename BlockAllocator<T, TSafeBlockAllocator>::Chunk;
    struct Block
    {
        std::byte *Data = nullptr;
        Block *Prev = nullptr;
    };
    struct BlockChunkData
    {
        Block *BlockTail = nullptr;
        Chunk *FreeList = nullptr;
    };

    T *allocateCAS(BlockChunkData p_BlockChunkData) KIT_NOEXCEPT
    {
        while (true)
        {
            i32 counter = m_AllocDeallocCounter.load(std::memory_order_acquire);
            if (counter >= 0 &&
                m_AllocDeallocCounter.compare_exchange_weak(counter, counter + 1, std::memory_order_acq_rel))
                break;
        }

        // This while loop attempts to allocate from an existing block
        while (p_BlockChunkData.FreeList)
        {
            // -r1- This is a data race (read), with its counterpart located in Deallocate (write). More on this above
            const BlockChunkData chunkData = {p_BlockChunkData.BlockTail, p_BlockChunkData.FreeList->Next};
            if (m_BlockChunkData.compare_exchange_weak(p_BlockChunkData, chunkData, std::memory_order_acq_rel))
            {
                m_AllocDeallocCounter.fetch_sub(1, std::memory_order_release);
                std::byte *freeList = reinterpret_cast<std::byte *>(p_BlockChunkData.FreeList);
                return reinterpret_cast<T *>(freeList - AlignedSize<T>());
            }
        }
        m_AllocDeallocCounter.fetch_sub(1, std::memory_order_release);
        // If, at some point in time, the free list is empty, we allocate a new block

        constexpr usz chunkSize = ChunkSize();
        constexpr usz alignment = BlockAllocator<T, TSafeBlockAllocator>::ChunkAlignment();
        constexpr usz objectSize = AlignedSize<T>();

        std::byte *data = reinterpret_cast<std::byte *>(AllocateAligned(this->BlockSize(), alignment));
        std::byte *shiftedData = data + objectSize;

        const usz chunksPerBlock = this->ChunksPerBlock();
        for (usz i = 0; i < chunksPerBlock - 1; ++i)
        {
            Chunk *chunk = reinterpret_cast<Chunk *>(shiftedData + i * chunkSize);
            chunk->Next = reinterpret_cast<Chunk *>(shiftedData + (i + 1) * chunkSize);
        }
        Chunk *last = reinterpret_cast<Chunk *>(shiftedData + (chunksPerBlock - 1) * chunkSize);
        last->Next = nullptr;

        // Once the block has been allocated, a lot of stuff may have happened in between. We need to check if the free
        // list or the block list have changed. If they have, we need to revert what we have done and try again. If they
        // havent, we must update the block and free list simultaneously
        Block *newBlock = new Block;
        newBlock->Data = data;
        newBlock->Prev = p_BlockChunkData.BlockTail;

        if (const BlockChunkData possibleData = {newBlock, reinterpret_cast<Chunk *>(shiftedData + chunkSize)};
            !m_BlockChunkData.compare_exchange_weak(p_BlockChunkData, possibleData, std::memory_order_acq_rel))
        {
            // Another thread was quicker than us, we must deallocate the block and try again
            DeallocateAligned(data);
            delete newBlock;
            return allocateCAS(p_BlockChunkData);
        }

        // We were able to proceed! BlockTail and FreeList have been updated simultaneously
        m_BlockCount.fetch_add(1, std::memory_order_relaxed);
        return reinterpret_cast<T *>(data);
    }

    std::atomic<u32> m_Allocations = 0;
    std::atomic<u32> m_BlockCount = 0;
    std::atomic<i32> m_AllocDeallocCounter = 0;

    // When allocating a new block, we must update the block list and the free list simultaneously. If we dont, it is
    // impossible (i think) to ensure that the block list and the free list are always in a valid, simultaneous state
    std::atomic<BlockChunkData> m_BlockChunkData{};
};

template <typename T> class KIT_API TUnsafeBlockAllocator final : public BlockAllocator<T, TUnsafeBlockAllocator>
{
  public:
    explicit TUnsafeBlockAllocator(const usz p_ChunksPerBlock) KIT_NOEXCEPT
        : BlockAllocator<T, TUnsafeBlockAllocator>(p_ChunksPerBlock)
    {
    }

    TUnsafeBlockAllocator(TUnsafeBlockAllocator &&p_Other) noexcept
        : BlockAllocator<T, TUnsafeBlockAllocator>(std::move(p_Other)), m_Allocations(p_Other.m_Allocations),
          m_FreeList(p_Other.m_FreeList), m_Blocks(std::move(p_Other.m_Blocks))
    {
        p_Other.m_Allocations = 0;
        p_Other.m_FreeList = nullptr;
        p_Other.m_Blocks.clear();
    }

    ~TUnsafeBlockAllocator() KIT_NOEXCEPT
    {
        Reset();
    }

    TUnsafeBlockAllocator &operator=(TUnsafeBlockAllocator &&p_Other) noexcept
    {
        if (this != &p_Other)
        {
            BlockAllocator<T, TUnsafeBlockAllocator>::operator=(std::move(p_Other));
            m_Allocations = p_Other.m_Allocations;
            m_FreeList = p_Other.m_FreeList;
            m_Blocks = std::move(p_Other.m_Blocks);

            p_Other.m_Allocations = 0;
            p_Other.m_FreeList = nullptr;
            p_Other.m_Blocks.clear();
        }
        return *this;
    }

    static constexpr usz ChunkSize() KIT_NOEXCEPT
    {
        if constexpr (sizeof(T) < sizeof(Chunk))
            return AlignedSize<Chunk>();
        else
            return AlignedSize<T>();
    }

    T *Allocate() KIT_NOEXCEPT
    {
        ++m_Allocations;
        if (m_FreeList)
            return fromNextFreeChunk();
        return fromFirstChunkOfNewBlock();
    }

    void Deallocate(T *p_Ptr) KIT_NOEXCEPT
    {
        KIT_ASSERT(!Empty(), "The current allocator has no active allocations yet");
        KIT_ASSERT(Owns(p_Ptr), "Trying to deallocate a pointer that was not allocated by this allocator");

        --m_Allocations;
        Chunk *chunk = reinterpret_cast<Chunk *>(p_Ptr);
        chunk->Next = m_FreeList;
        m_FreeList = chunk;
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
    bool Owns(const T *p_Ptr) const KIT_NOEXCEPT
    {
        const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
        for (const std::byte *block : m_Blocks)
            if (ptr >= block && ptr < block + this->BlockSize())
                return true;
        return false;
    }

    usz BlockCount() const KIT_NOEXCEPT
    {
        return m_Blocks.size();
    }

    bool Empty() const KIT_NOEXCEPT
    {
        return m_Allocations == 0;
    }
    u32 Allocations() const KIT_NOEXCEPT
    {
        return m_Allocations;
    }

  private:
    // Important: A chunk must be at least sizeof(void *), so allocations have a minimum size of sizeof(void *)
    using Chunk = typename BlockAllocator<T, TUnsafeBlockAllocator>::Chunk;

    T *fromFirstChunkOfNewBlock() KIT_NOEXCEPT
    {
        constexpr usz chunkSize = ChunkSize();
        constexpr usz alignment = BlockAllocator<T, TUnsafeBlockAllocator>::ChunkAlignment();

        std::byte *data = reinterpret_cast<std::byte *>(AllocateAligned(this->BlockSize(), alignment));
        m_FreeList = reinterpret_cast<Chunk *>(data + chunkSize);

        const usz chunksPerBlock = this->ChunksPerBlock();
        for (usz i = 0; i < chunksPerBlock - 1; ++i)
        {
            Chunk *chunk = reinterpret_cast<Chunk *>(data + i * chunkSize);
            chunk->Next = reinterpret_cast<Chunk *>(data + (i + 1) * chunkSize);
        }
        Chunk *last = reinterpret_cast<Chunk *>(data + (chunksPerBlock - 1) * chunkSize);
        last->Next = nullptr;

        m_Blocks.push_back(data);
        return reinterpret_cast<T *>(data);
    }

    T *fromNextFreeChunk() KIT_NOEXCEPT
    {
        Chunk *chunk = m_FreeList;
        m_FreeList = chunk->Next;
        return reinterpret_cast<T *>(chunk);
    }

    u32 m_Allocations = 0;
    Chunk *m_FreeList = nullptr;
    DynamicArray<std::byte *> m_Blocks;
};

KIT_NAMESPACE_END

#ifdef KIT_ENABLE_BLOCK_ALLOCATOR
#    define KIT_BLOCK_ALLOCATED(p_Allocator, p_ClassName, p_ChunksPerBlock)                                            \
        static inline p_Allocator<p_ClassName> s_Allocator{p_ChunksPerBlock};                                          \
        static void *operator new(usz p_Size)                                                                          \
        {                                                                                                              \
            KIT_ASSERT(p_Size == sizeof(p_ClassName),                                                                  \
                       "Trying to block allocate a derived class from a base class overloaded new/delete");            \
            return s_Allocator.Allocate();                                                                             \
        }                                                                                                              \
        static void operator delete(void *p_Ptr)                                                                       \
        {                                                                                                              \
            s_Allocator.Deallocate(reinterpret_cast<p_ClassName *>(p_Ptr));                                            \
        }
#else
#    define KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)
#endif