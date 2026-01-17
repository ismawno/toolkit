#include "tkit/core/pch.hpp"
#include "tkit/memory/block_allocator.hpp"
#include "block_allocator.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
BlockAllocator::BlockAllocator(const usize p_BufferSize, const usize p_AllocationSize, const usize p_Alignment)
    : m_BufferSize(p_BufferSize), m_AllocationSize(p_AllocationSize), m_Provided(false)
{
    TKIT_ASSERT(p_AllocationSize >= sizeof(Allocation),
                "[TOOLKIT][BLOCK-ALLOC] The allocation size must be at least {} bytes", sizeof(Allocation));
    TKIT_ASSERT(p_BufferSize % p_Alignment == 0, "[TOOLKIT][BLOCK-ALLOC] The buffer size must be a multiple of the "
                                                 "alignment to ensure every block of memory is aligned to it");
    TKIT_ASSERT(
        p_BufferSize % p_AllocationSize == 0,
        "[TOOLKIT][BLOCK-ALLOC] The buffer size must be a multiple of the allocation size to guarantee a tight fit");
    TKIT_ASSERT(p_AllocationSize % p_Alignment == 0, "[TOOLKIT][BLOCK-ALLOC] The allocation size must be a multiple of "
                                                     "the alignment to ensure every block of memory is aligned to it");

    m_Buffer = static_cast<std::byte *>(Memory::AllocateAligned(p_BufferSize, p_Alignment));
    TKIT_ASSERT(m_Buffer, "[TOOLKIT][BLOCK-ALLOC] Failed to allocate memory");
    setupMemoryLayout();
}

BlockAllocator::BlockAllocator(void *p_Buffer, const usize p_BufferSize, const usize p_AllocationSize)
    : m_Buffer(static_cast<std::byte *>(p_Buffer)), m_BufferSize(p_BufferSize), m_AllocationSize(p_AllocationSize),
      m_Provided(true)
{
    TKIT_ASSERT(
        p_BufferSize % p_AllocationSize == 0,
        "[TOOLKIT][BLOCK-ALLOC] The buffer size must be a multiple of the allocation size to guarantee a tight fit");
    setupMemoryLayout();
}

BlockAllocator::~BlockAllocator()
{
    deallocateBuffer();
}

BlockAllocator::BlockAllocator(BlockAllocator &&p_Other)
    : m_Buffer(p_Other.m_Buffer), m_FreeList(p_Other.m_FreeList), m_BufferSize(p_Other.m_BufferSize),
      m_AllocationSize(p_Other.m_AllocationSize), m_Provided(p_Other.m_Provided)
{
    p_Other.m_Buffer = nullptr;
    p_Other.m_FreeList = nullptr;
    p_Other.m_BufferSize = 0;
    p_Other.m_AllocationSize = 0;
    m_Provided = false;
}

BlockAllocator &BlockAllocator::operator=(BlockAllocator &&p_Other)
{
    if (this != &p_Other)
    {
        deallocateBuffer();
        m_Buffer = p_Other.m_Buffer;
        m_FreeList = p_Other.m_FreeList;
        m_BufferSize = p_Other.m_BufferSize;
        m_AllocationSize = p_Other.m_AllocationSize;
        m_Provided = p_Other.m_Provided;

        p_Other.m_Buffer = nullptr;
        p_Other.m_FreeList = nullptr;
        p_Other.m_BufferSize = 0;
        p_Other.m_AllocationSize = 0;
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

void BlockAllocator::Deallocate(void *p_Ptr)
{
    TKIT_ASSERT(p_Ptr, "[TOOLKIT][BLOCK-ALLOC] Cannot deallocate a null pointer");
    TKIT_ASSERT(Belongs(p_Ptr),
                "[TOOLKIT][BLOCK-ALLOC] Cannot deallocate a pointer that does not belong to the allocator");

    Allocation *alloc = static_cast<Allocation *>(p_Ptr);
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

    Memory::DeallocateAligned(m_Buffer);
}

} // namespace TKit
