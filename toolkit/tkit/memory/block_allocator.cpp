#include "tkit/core/pch.hpp"
#include "tkit/memory/block_allocator.hpp"
#include "block_allocator.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
BlockAllocator::BlockAllocator(const usize bufferSize, const usize allocationSize, const usize alignment)
    : m_BufferSize(bufferSize), m_AllocationSize(allocationSize), m_Provided(false)
{
    TKIT_ASSERT(allocationSize >= sizeof(Allocation),
                "[TOOLKIT][BLOCK-ALLOC] The allocation size must be at least {:L} bytes", sizeof(Allocation));
    TKIT_ASSERT(bufferSize % alignment == 0, "[TOOLKIT][BLOCK-ALLOC] The buffer size must be a multiple of the "
                                             "alignment to ensure every block of memory is aligned to it");
    TKIT_ASSERT(bufferSize % allocationSize == 0,
                "[TOOLKIT][BLOCK-ALLOC] The buffer size must be a multiple of the allocation size to guarantee a tight "
                "fit, but got {:L}",
                bufferSize);
    TKIT_ASSERT(allocationSize % alignment == 0,
                "[TOOLKIT][BLOCK-ALLOC] The allocation size must be a multiple of "
                "the alignment to ensure every block of memory is aligned to it, but got {:L}",
                allocationSize);

    m_Buffer = static_cast<std::byte *>(AllocateAligned(bufferSize, alignment));
    TKIT_ASSERT(m_Buffer, "[TOOLKIT][BLOCK-ALLOC] Failed to allocate {:L} bytes of memory aligned to {:L} bytes",
                bufferSize, alignment);
    setupMemoryLayout();
}

BlockAllocator::BlockAllocator(void *buffer, const usize bufferSize, const usize allocationSize)
    : m_Buffer(static_cast<std::byte *>(buffer)), m_BufferSize(bufferSize), m_AllocationSize(allocationSize),
      m_Provided(true)
{
    TKIT_ASSERT(bufferSize % allocationSize == 0,
                "[TOOLKIT][BLOCK-ALLOC] The buffer size ({:L}) must be a multiple of the allocation size to guarantee "
                "a tight fit",
                bufferSize);
    setupMemoryLayout();
}

BlockAllocator::~BlockAllocator()
{
    deallocateBuffer();
}

BlockAllocator::BlockAllocator(BlockAllocator &&other)
    : m_Buffer(other.m_Buffer), m_FreeList(other.m_FreeList), m_BufferSize(other.m_BufferSize),
      m_AllocationSize(other.m_AllocationSize), m_Provided(other.m_Provided)
{
    other.m_Buffer = nullptr;
    other.m_FreeList = nullptr;
    other.m_BufferSize = 0;
    other.m_AllocationSize = 0;
    m_Provided = false;
}

BlockAllocator &BlockAllocator::operator=(BlockAllocator &&other)
{
    if (this != &other)
    {
        deallocateBuffer();
        m_Buffer = other.m_Buffer;
        m_FreeList = other.m_FreeList;
        m_BufferSize = other.m_BufferSize;
        m_AllocationSize = other.m_AllocationSize;
        m_Provided = other.m_Provided;

        other.m_Buffer = nullptr;
        other.m_FreeList = nullptr;
        other.m_BufferSize = 0;
        other.m_AllocationSize = 0;
        m_Provided = false;
    }
    return *this;
}

void *BlockAllocator::Allocate()
{
    // TKIT_ASSERT(m_FreeList, "The allocator is full");
    if (!m_FreeList)
    {
        TKIT_LOG_WARNING("[TOOLKIT][BLOCK-ALLOC] Allocator ran out of slots when trying to perform an allocation");
        return nullptr;
    }

    Allocation *alloc = m_FreeList;
    m_FreeList = m_FreeList->Next;
    return alloc;
}

void BlockAllocator::Deallocate(void *ptr)
{
    TKIT_ASSERT(ptr, "[TOOLKIT][BLOCK-ALLOC] Cannot deallocate a null pointer");
    TKIT_ASSERT(Belongs(ptr),
                "[TOOLKIT][BLOCK-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");

    Allocation *alloc = static_cast<Allocation *>(ptr);
    alloc->Next = m_FreeList;
    m_FreeList = alloc;
}

void BlockAllocator::Reset()
{
    setupMemoryLayout();
}

void BlockAllocator::setupMemoryLayout()
{
    const usize count = GetAllocationCapacityCount();
    m_FreeList = reinterpret_cast<Allocation *>(m_Buffer);

    Allocation *next = nullptr;
    for (usize i = count - 1; i < count; --i)
    {
        Allocation *alloc = reinterpret_cast<Allocation *>(m_Buffer + i * m_AllocationSize);
        alloc->Next = next;
        next = alloc;
    }
}

void BlockAllocator::deallocateBuffer()
{
    if (!m_Buffer || m_Provided)
        return;
    // TKIT_LOG_WARNING_IF(
    //     !IsEmpty(),
    //     "[TOOLKIT][BLOCK-ALLOC] Deallocating a block allocator with active allocations. If the elements are not "
    //     "trivially destructible, you will have to call "
    //     "Destroy() for each element to avoid undefined behaviour (this deallocation will not call the destructor)");

    DeallocateAligned(m_Buffer);
}

} // namespace TKit
