#pragma once

#include "kit/core/alias.hpp"
#include "kit/core/non_copyable.hpp"
#include "kit/memory/memory.hpp"
#include "kit/core/logging.hpp"
#include "kit/core/concepts.hpp"
#include "kit/multiprocessing/spin_mutex.hpp"

KIT_NAMESPACE_BEGIN

// This is a block allocator whose role is to 1: speed up single allocations and 2: improve contiguity, which is
// guaranteed up to the amount of chunks per block (each chunk represents an allocated object)

// On my macOS m1 this allocator is able to allocate 10000 elements of 128 bytes in 0.035 ms and deallocate them in
// 0.012 (3.5ns per allocation and 1.2ns per deallocation). This is roughly a 10x improvement over the default
// new/delete, using the unsafe variants. When usingthe safe variants, latency is doubled (aprox)

template <typename T> class KIT_API BlockAllocator final
{
    KIT_NON_COPYABLE(BlockAllocator)
  public:
    explicit BlockAllocator(const usize p_ChunksPerBlock) KIT_NOEXCEPT : m_BlockSize(ChunkSize() * p_ChunksPerBlock)
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

    usize BlockSize() const KIT_NOEXCEPT
    {
        return m_BlockSize;
    }

    usize ChunksPerBlock() const KIT_NOEXCEPT
    {
        return m_BlockSize / ChunkSize();
    }

    static KIT_CONSTEVAL usize ChunkSize() KIT_NOEXCEPT
    {
        if constexpr (sizeof(T) < sizeof(Chunk))
            return AlignedSize<Chunk>();
        else
            return AlignedSize<T>();
    }
    static KIT_CONSTEVAL usize ChunkAlignment() KIT_NOEXCEPT
    {
        if constexpr (alignof(T) < alignof(Chunk))
            return alignof(Chunk);
        else
            return alignof(T);
    }

    template <typename... Args> [[nodiscard]] T *Create(Args &&...p_Args) KIT_NOEXCEPT
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

    template <typename... Args> [[nodiscard]] T *CreateUnsafe(Args &&...p_Args) KIT_NOEXCEPT
    {
        T *ptr = AllocateUnsafe();
        ::new (ptr) T(std::forward<Args>(p_Args)...);
        return ptr;
    }
    void DestroyUnsafe(T *p_Ptr) KIT_NOEXCEPT
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            p_Ptr->~T();
        DeallocateUnsafe(p_Ptr);
    }

    [[nodiscard]] T *Allocate() KIT_NOEXCEPT
    {
        std::scoped_lock lock(m_Mutex);
        return AllocateUnsafe();
    }

    void Deallocate(T *p_Ptr) KIT_NOEXCEPT
    {
        std::scoped_lock lock(m_Mutex);
        DeallocateUnsafe(p_Ptr);
    }

    [[nodiscard]] T *AllocateUnsafe() KIT_NOEXCEPT
    {
        ++m_Allocations;
        if (m_FreeList)
            return fromNextFreeChunk();
        return fromFirstChunkOfNewBlock();
    }

    void DeallocateUnsafe(T *p_Ptr) KIT_NOEXCEPT
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

    usize BlockCount() const KIT_NOEXCEPT
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
        constexpr usize chunkSize = ChunkSize();
        constexpr usize alignment = ChunkAlignment();

        std::byte *data = static_cast<std::byte *>(AllocateAligned(this->BlockSize(), alignment));
        m_FreeList = reinterpret_cast<Chunk *>(data + chunkSize);

        const usize chunksPerBlock = this->ChunksPerBlock();
        for (usize i = 0; i < chunksPerBlock - 1; ++i)
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
    usize m_BlockSize;
    SpinMutex m_Mutex;
};

// TODO: Consider replacing all of this with a thread-safe version of the block allocator using a SpinMutex instead of a
// plain lock
template <typename T, usize ChunksPerBlock> BlockAllocator<T> &LocalBlockAllocatorInstance() KIT_NOEXCEPT
{
    static BlockAllocator<T> allocator{ChunksPerBlock};
    return allocator;
}

template <typename T, usize ChunksPerBlock> T *BAllocate() KIT_NOEXCEPT
{
    return LocalBlockAllocatorInstance<T, ChunksPerBlock>().Allocate();
}
template <typename T, usize ChunksPerBlock> void BDeallocate(T *p_Ptr) KIT_NOEXCEPT
{
    LocalBlockAllocatorInstance<T, ChunksPerBlock>().Deallocate(p_Ptr);
}

template <typename T, usize ChunksPerBlock> T *BAllocateUnsafe() KIT_NOEXCEPT
{
    return LocalBlockAllocatorInstance<T, ChunksPerBlock>().AllocateUnsafe();
}
template <typename T, usize ChunksPerBlock> void BDeallocateUnsafe(T *p_Ptr) KIT_NOEXCEPT
{
    LocalBlockAllocatorInstance<T, ChunksPerBlock>().DeallocateUnsafe(p_Ptr);
}

template <typename T, usize ChunksPerBlock, typename... Args> T *BCreate(Args &&...p_Args) KIT_NOEXCEPT
{
    return LocalBlockAllocatorInstance<T, ChunksPerBlock>().Create(std::forward<Args>(p_Args)...);
}
template <typename T, usize ChunksPerBlock> void BDestroy(T *p_Ptr) KIT_NOEXCEPT
{
    LocalBlockAllocatorInstance<T, ChunksPerBlock>().Destroy(p_Ptr);
}

template <typename T, usize ChunksPerBlock, typename... Args> T *BCreateUnsafe(Args &&...p_Args) KIT_NOEXCEPT
{
    return LocalBlockAllocatorInstance<T, ChunksPerBlock>().CreateUnsafe(std::forward<Args>(p_Args)...);
}
template <typename T, usize ChunksPerBlock> void BDestroyUnsafe(T *p_Ptr) KIT_NOEXCEPT
{
    LocalBlockAllocatorInstance<T, ChunksPerBlock>().DestroyUnsafe(p_Ptr);
}

KIT_NAMESPACE_END

#define KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)                                                             \
    static void *operator new([[maybe_unused]] usize p_Size)                                                           \
    {                                                                                                                  \
        KIT_ASSERT(p_Size == sizeof(p_ClassName),                                                                      \
                   "Trying to block allocate a derived class from a base class overloaded new/delete");                \
        return KIT_NAMESPACE_NAME::BAllocate<p_ClassName, p_ChunksPerBlock>();                                         \
    }                                                                                                                  \
    static void operator delete(void *p_Ptr)                                                                           \
    {                                                                                                                  \
        KIT_NAMESPACE_NAME::BDeallocate<p_ClassName, p_ChunksPerBlock>(static_cast<p_ClassName *>(p_Ptr));             \
    }

#ifndef KIT_OVERRIDE_NEW_DELETE
#    ifdef KIT_ENABLE_BLOCK_ALLOCATOR
#        define KIT_OVERRIDE_NEW_DELETE(p_ClassName, p_ChunksPerBlock)                                                 \
            KIT_BLOCK_ALLOCATED(p_ClassName, p_ChunksPerBlock)
#    else
#        define KIT_OVERRIDE_NEW_DELETE(...)
#    endif
#endif