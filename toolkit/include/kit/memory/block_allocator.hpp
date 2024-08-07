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

// This is a block allocator whose roll is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of chunks per block (each chunk represents an allocated object)
template <typename T> class KIT_API BlockAllocator final
{
    KIT_NON_COPYABLE(BlockAllocator);

  public:
    explicit BlockAllocator(const usz p_ChunksPerBlock) KIT_NOEXCEPT : m_BlockSize(alignedSize() * p_ChunksPerBlock)
    {
    }

    BlockAllocator(BlockAllocator &&p_Other) KIT_NOEXCEPT : m_Allocations(p_Other.m_Allocations),
                                                            m_BlockSize(p_Other.m_BlockSize),
                                                            m_FreeList(p_Other.m_FreeList),
                                                            m_Blocks(std::move(p_Other.m_Blocks))
    {
        p_Other.m_Allocations = 0;
        p_Other.m_FreeList = nullptr;
        p_Other.m_Blocks.clear();
    }

    ~BlockAllocator() KIT_NOEXCEPT
    {
        for (std::byte *block : m_Blocks)
            DeallocateAligned(block);
    }

    BlockAllocator &operator=(BlockAllocator &&p_Other) KIT_NOEXCEPT
    {
        if (this != &p_Other)
        {
            m_Allocations = p_Other.m_Allocations;
            m_BlockSize = p_Other.m_BlockSize;
            m_FreeList = p_Other.m_FreeList;
            m_Blocks = std::move(p_Other.m_Blocks);

            p_Other.m_Allocations = 0;
            p_Other.m_FreeList = nullptr;
            p_Other.m_Blocks.clear();
        }
        return *this;
    }

    T *Allocate() KIT_NOEXCEPT
    {
        KIT_BLOCK_ALLOCATOR_UNIQUE_LOCK(m_Mutex);
        ++m_Allocations;
        if (m_FreeList)
            return fromNextFreeChunk();
        return fromFirstChunkOfNewBlock();
    }

    void Deallocate(T *p_Ptr) KIT_NOEXCEPT
    {
        KIT_ASSERT(!Empty(), "The current allocator has no active allocations yet");
        KIT_ASSERT(Owns(p_Ptr), "Trying to deallocate a pointer that was not allocated by this allocator");
        KIT_BLOCK_ALLOCATOR_UNIQUE_LOCK(m_Mutex);

        --m_Allocations;
        Chunk *chunk = reinterpret_cast<Chunk *>(p_Ptr);
        chunk->Next = m_FreeList;
        m_FreeList = chunk;
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

    bool Owns(const T *p_Ptr) const KIT_NOEXCEPT
    {
        KIT_BLOCK_ALLOCATOR_SHARED_LOCK(m_Mutex);
        const std::byte *ptr = reinterpret_cast<const std::byte *>(p_Ptr);
        for (const std::byte *block : m_Blocks)
            if (ptr >= block && ptr < block + m_BlockSize)
                return true;
        return false;
    }

    usz BlockSize() const KIT_NOEXCEPT
    {
        return m_BlockSize;
    }
    usz BlockCount() const KIT_NOEXCEPT
    {
        return m_Blocks.size();
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
        return m_Allocations == 0;
    }
    u32 Allocations() const KIT_NOEXCEPT
    {
        return m_Allocations;
    }

  private:
    // Important: A chunk must be at least sizeof(void *), so allocations have a minimum size of sizeof(void *)
    struct Chunk
    {
        Chunk *Next = nullptr;
    };

    T *fromFirstChunkOfNewBlock() KIT_NOEXCEPT
    {
        const usz chunkSize = alignedSize();
        const usz align = alignment();

        std::byte *data = reinterpret_cast<std::byte *>(AllocateAligned(m_BlockSize, align));
        m_FreeList = reinterpret_cast<Chunk *>(data + chunkSize);

        const usz chunksPerBlock = m_BlockSize / chunkSize;
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

    // Could be an atomic, but all operations I perform with it are locked, so I guess it's fine
    u32 m_Allocations = 0;
    usz m_BlockSize;
    Chunk *m_FreeList = nullptr;
    DynamicArray<std::byte *> m_Blocks;
#ifdef KIT_BLOCK_ALLOCATOR_THREAD_SAFE
    mutable std::shared_mutex m_Mutex;
#endif
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