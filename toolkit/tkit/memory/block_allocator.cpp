#include "tkit/core/pch.hpp"
#include "tkit/memory/block_allocator.hpp"
#include "block_allocator.hpp"
#include "tkit/utils/logging.hpp"

namespace TKit
{
BlockAllocator::BlockAllocator(const usize p_BufferSize, const usize p_AllocationSize, const usize p_Alignment) noexcept
    : m_BufferSize(p_BufferSize), m_AllocationSize(p_AllocationSize), m_Provided(false)
{
    TKIT_ASSERT(p_AllocationSize >= sizeof(Allocation), "The allocation size must be at least {} bytes",
                sizeof(Allocation));
    TKIT_ASSERT(p_BufferSize % p_Alignment == 0,
                "The buffer size must be a multiple of the alignment to ensure every block of memory is aligned to it");
    TKIT_ASSERT(p_BufferSize % p_AllocationSize == 0,
                "The buffer size must be a multiple of the allocation size to guarantee a tight fit");
    TKIT_ASSERT(
        p_AllocationSize % p_Alignment == 0,
        "The allocation size must be a multiple of the alignment to ensure every block of memory is aligned to it");

    m_Buffer = static_cast<std::byte *>(Memory::AllocateAligned(p_BufferSize, p_Alignment));
    TKIT_ASSERT(m_Buffer, "[TOOLKIT][BLOCK-ALLOC] Failed to allocate memory");
    setupMemoryLayout();
}

BlockAllocator::BlockAllocator(void *p_Buffer, const usize p_BufferSize, const usize p_AllocationSize) noexcept
    : m_Buffer(static_cast<std::byte *>(p_Buffer)), m_BufferSize(p_BufferSize), m_AllocationSize(p_AllocationSize),
      m_Provided(true)
{
    TKIT_ASSERT(p_BufferSize % p_AllocationSize == 0,
                "The buffer size must be a multiple of the allocation size to guarantee a tight fit");
    setupMemoryLayout();
}

BlockAllocator::~BlockAllocator() noexcept
{
    deallocateBuffer();
}

BlockAllocator::BlockAllocator(BlockAllocator &&p_Other) noexcept
    : m_Buffer(p_Other.m_Buffer), m_FreeList(p_Other.m_FreeList), m_Allocations(p_Other.m_Allocations),
      m_Provided(p_Other.m_Provided)
{
    p_Other.m_Buffer = nullptr;
    p_Other.m_FreeList = nullptr;
    p_Other.m_BufferSize = 0;
    p_Other.m_AllocationSize = 0;
    p_Other.m_Allocations = 0;
    m_Provided = false;
}

BlockAllocator &BlockAllocator::operator=(BlockAllocator &&p_Other) noexcept
{
    if (this != &p_Other)
    {
        deallocateBuffer();
        m_Buffer = p_Other.m_Buffer;
        m_FreeList = p_Other.m_FreeList;
        m_Allocations = p_Other.m_Allocations;
        m_Provided = p_Other.m_Provided;

        p_Other.m_Buffer = nullptr;
        p_Other.m_FreeList = nullptr;
        p_Other.m_BufferSize = 0;
        p_Other.m_AllocationSize = 0;
        p_Other.m_Allocations = 0;
        m_Provided = false;
    }
    return *this;
}

void *BlockAllocator::Allocate() noexcept
{
    TKIT_ASSERT(m_FreeList, "The allocator is full");

    ++m_Allocations;
    Allocation *alloc = m_FreeList;
    m_FreeList = m_FreeList->Next;
    return alloc;
}

void BlockAllocator::Deallocate(void *p_Ptr) noexcept
{
    TKIT_ASSERT(!IsEmpty(), "Cannot deallocate from an empty allocator");

    --m_Allocations;
    Allocation *alloc = static_cast<Allocation *>(p_Ptr);
    alloc->Next = m_FreeList;
    m_FreeList = alloc;
}

void BlockAllocator::Reset() noexcept
{
    TKIT_ASSERT(IsEmpty(),
                "The allocator still has active allocations. Resetting it will mangle the memory and corrupt it");

    setupMemoryLayout();
}

bool BlockAllocator::Belongs(const void *p_Ptr) const noexcept
{
    const std::byte *ptr = static_cast<const std::byte *>(p_Ptr);
    return ptr >= m_Buffer && ptr < m_Buffer + m_BufferSize;
}
bool BlockAllocator::IsEmpty() const noexcept
{
    return m_Allocations == 0;
}
bool BlockAllocator::IsFull() const noexcept
{
    return m_Allocations == GetAllocationCapacityCount();
}

usize BlockAllocator::GetBufferSize() const noexcept
{
    return m_BufferSize;
}
usize BlockAllocator::GetAllocationSize() const noexcept
{
    return m_AllocationSize;
}

usize BlockAllocator::GetAllocationCount() const noexcept
{
    return m_Allocations;
}
usize BlockAllocator::GetRemainingCount() const noexcept
{
    return m_BufferSize / m_AllocationSize - m_Allocations;
}
usize BlockAllocator::GetAllocationCapacityCount() const noexcept
{
    return m_BufferSize / m_AllocationSize;
}

void BlockAllocator::setupMemoryLayout() noexcept
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

void BlockAllocator::deallocateBuffer() noexcept
{
    if (!m_Buffer || m_Provided)
        return;
    TKIT_LOG_WARNING_IF(
        !IsEmpty(),
        "[TOOLKIT][BLOCK-ALLOC] Deallocating a block allocator with active allocations. If the elements are not "
        "trivially destructible, you will have to call "
        "Destroy() for each element to avoid undefined behaviour (this deallocation will not call the destructor)");

    Memory::DeallocateAligned(m_Buffer);
    m_Buffer = nullptr;
    m_Allocations = 0;
    m_FreeList = nullptr;
}

} // namespace TKit
