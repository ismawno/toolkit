#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include "kit/interface/non_copyable.hpp"
#include "kit/memory/memory.hpp"
#include "kit/logging/logging.hpp"

KIT_NAMESPACE_BEGIN

// This is THREAD UNSAFE. TODO: Make it thread safe
template <typename T> class KIT_API BlockAllocator final
{
    KIT_NON_COPYABLE(BlockAllocator);

  public:
    explicit BlockAllocator(const size_t p_ChunksPerBlock) KIT_NOEXCEPT : m_BlockSize(alignedSize() * p_ChunksPerBlock)
    {
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

    template <typename... Args> T *Construct(Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = Allocate();
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }

    void Destroy(T *p_Ptr) KIT_NOEXCEPT
    {
        p_Ptr->~T();
        Deallocate(p_Ptr);
    }

    bool Owns(const T *p_Ptr) const KIT_NOEXCEPT
    {
        const Byte *ptr = reinterpret_cast<const Byte *>(p_Ptr);
        for (const Byte *block : m_Blocks)
            if (ptr >= block && ptr < block + m_BlockSize)
                return true;
        return false;
    }

    size_t BlockSize() const KIT_NOEXCEPT
    {
        return m_BlockSize;
    }
    size_t BlockCount() const KIT_NOEXCEPT
    {
        return m_Blocks.size();
    }

    size_t ChunksPerBlock() const KIT_NOEXCEPT
    {
        return m_BlockSize / alignedSize();
    }
    size_t ChunkSize() const KIT_NOEXCEPT
    {
        return alignedSize();
    }

    bool Empty() const KIT_NOEXCEPT
    {
        return m_Allocations == 0;
    }
    uint32_t Allocations() const KIT_NOEXCEPT
    {
        return m_Allocations;
    }

  private:
    struct Chunk
    {
        Chunk *Next = nullptr;
    };

    T *fromFirstChunkOfNewBlock() KIT_NOEXCEPT
    {
        const size_t chunkSize = alignedSize();
        const size_t align = alignment();

        Byte *data = reinterpret_cast<Byte *>(AllocateAligned(m_BlockSize, align));
        m_FreeList = reinterpret_cast<Chunk *>(data + chunkSize);

        const size_t chunksPerBlock = m_BlockSize / chunkSize;
        for (size_t i = 0; i < chunksPerBlock - 1; ++i)
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

    constexpr size_t alignedSize() const KIT_NOEXCEPT
    {
        if constexpr (sizeof(T) < sizeof(Chunk))
            return AlignedSize<Chunk>();
        else
            return AlignedSize<T>();
    }

    constexpr size_t alignment() const KIT_NOEXCEPT
    {
        if constexpr (alignof(T) < alignof(Chunk))
            return alignof(Chunk);
        else
            return alignof(T);
    }

    uint32_t m_Allocations = 0;
    size_t m_BlockSize;
    Chunk *m_FreeList = nullptr;
    DynamicArray<Byte *> m_Blocks;
};

KIT_NAMESPACE_END

#ifndef KIT_DISABLE_BLOCK_ALLOCATOR
#    define KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)                                                         \
        static inline KIT::BlockAllocator<p_ClassName> s_Allocator{p_ChunksPerBlock};                                  \
        void *operator new(size_t p_Size)                                                                              \
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