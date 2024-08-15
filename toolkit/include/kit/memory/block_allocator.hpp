#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include "kit/core/non_copyable.hpp"
#include "kit/memory/memory.hpp"
#include "kit/core/logging.hpp"
#include "kit/core/concepts.hpp"
#include <shared_mutex>
#include <mutex>
#include <atomic>

KIT_NAMESPACE_BEGIN

// This is a block allocator whose role is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of chunks per block (each chunk represents an allocated object)
template <typename T> class KIT_API BlockAllocator final
{
    KIT_NON_COPYABLE(BlockAllocator)
  public:
    explicit BlockAllocator(const usz p_ChunksPerBlock) KIT_NOEXCEPT : m_BlockSize(ChunkSize() * p_ChunksPerBlock)
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

    ~BlockAllocator() KIT_NOEXCEPT
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
        if constexpr (sizeof(T) < sizeof(Chunk))
            return AlignedSize<Chunk>();
        else
            return AlignedSize<T>();
    }
    static constexpr usz ChunkAlignment() KIT_NOEXCEPT
    {
        if constexpr (alignof(T) < alignof(Chunk))
            return alignof(Chunk);
        else
            return alignof(T);
    }

    template <typename... Args> T *Construct(Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = Allocate();
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }
    void Destroy(T *p_Ptr) KIT_NOEXCEPT
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        Deallocate(p_Ptr);
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
    // Important: An allocated piece (chunk) must be at least sizeof(void *), so an object of size chunk can fit
    struct Chunk
    {
        Chunk *Next = nullptr;
    };

    T *fromFirstChunkOfNewBlock() KIT_NOEXCEPT
    {
        constexpr usz chunkSize = ChunkSize();
        constexpr usz alignment = ChunkAlignment();

        std::byte *data = static_cast<std::byte *>(AllocateAligned(this->BlockSize(), alignment));
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
    usz m_BlockSize;
};

KIT_NAMESPACE_END

#define KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)                                                             \
    static inline BlockAllocator<p_ClassName> s_Allocator{p_ChunksPerBlock};                                           \
    static void *operator new(usz p_Size)                                                                              \
    {                                                                                                                  \
        KIT_ASSERT(p_Size == sizeof(p_ClassName),                                                                      \
                   "Trying to block allocate a derived class from a base class overloaded new/delete");                \
        return s_Allocator.Allocate();                                                                                 \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr)                                                                           \
    {                                                                                                                  \
        s_Allocator.Deallocate(reinterpret_cast<p_ClassName *>(p_Ptr));                                                \
    }

#ifndef KIT_OVERRIDE_NEW_DELETE
#    ifdef KIT_ENABLE_BLOCK_ALLOCATOR
#        define KIT_OVERRIDE_NEW_DELETE(...) KIT_BLOCK_ALLOCATED(__VA_ARGS__)
#    else
#        define KIT_OVERRIDE_NEW_DELETE(...)
#    endif
#endif