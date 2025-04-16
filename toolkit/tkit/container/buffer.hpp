#pragma once

#include "tkit/container/alias.hpp"
#include "tkit/memory/memory.hpp"
#include "tkit/utils/logging.hpp"

namespace TKit
{

/**
 * @brief A simple class that represents a raw buffer of memory consisting of a number of instances, each one aligned to
 * a specific value.
 *
 * This data structure is useful in use cases where aignment requirements are strict, such as in graphics APIs where
 * elements in an array must be individually aligned to a certain value.
 *
 * @tparam SizeType The type of the size of the buffer.
 */
template <typename SizeType = usize>
    requires(std::integral<SizeType>)
class RawBuffer
{
  public:
    using size_type = SizeType;

    constexpr RawBuffer() noexcept = default;

    constexpr RawBuffer(const size_type p_InstanceCount, const size_type p_InstanceSize,
                        const size_type p_InstanceAlignment = 1) noexcept
        : m_InstanceCount(p_InstanceCount), m_InstanceSize(p_InstanceSize),
          m_InstanceAlignedSize(alignedSize(p_InstanceSize, p_InstanceAlignment)),
          m_Size(p_InstanceCount * m_InstanceAlignedSize)
    {
        if (m_Size > 0)
        {
            m_Data = Memory::AllocateAligned(m_Size, p_InstanceAlignment);
            TKIT_ASSERT(m_Data, "[TOOLKIT] Failed to allocate memory");
        }
    }

    constexpr RawBuffer(const RawBuffer &p_Other) noexcept
        : m_InstanceCount(p_Other.m_InstanceCount), m_InstanceSize(p_Other.m_InstanceSize),
          m_InstanceAlignedSize(p_Other.m_InstanceAlignedSize), m_Size(p_Other.m_Size)
    {
        if (m_Size > 0)
        {
            m_Data = Memory::AllocateAligned(m_Size, m_InstanceAlignedSize);
            TKIT_ASSERT(m_Data, "[TOOLKIT] Failed to allocate memory");
            Memory::Copy(m_Data, p_Other.m_Data, m_Size);
        }
    }

    constexpr RawBuffer(RawBuffer &&p_Other) noexcept
        : m_Data(p_Other.m_Data), m_InstanceCount(p_Other.m_InstanceCount), m_InstanceSize(p_Other.m_InstanceSize),
          m_InstanceAlignedSize(p_Other.m_InstanceAlignedSize), m_Size(p_Other.m_Size)
    {
        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_InstanceCount = 0;
        p_Other.m_InstanceAlignedSize = 0;
        p_Other.m_InstanceSize = 0;
    }

    constexpr RawBuffer &operator=(const RawBuffer &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        if (m_Data)
            Memory::DeallocateAligned(m_Data);

        m_InstanceCount = p_Other.m_InstanceCount;
        m_InstanceSize = p_Other.m_InstanceSize;
        m_InstanceAlignedSize = p_Other.m_InstanceAlignedSize;
        m_Size = p_Other.m_Size;

        if (p_Other.m_Data && m_Size > 0)
        {
            m_Data = Memory::AllocateAligned(m_Size, m_InstanceAlignedSize);
            TKIT_ASSERT(m_Data, "[TOOLKIT] Failed to allocate memory");
            Memory::Copy(m_Data, p_Other.m_Data, m_Size);
        }
        else
            m_Data = nullptr;
        return *this;
    }

    constexpr RawBuffer &operator=(RawBuffer &&p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        if (m_Data)
            Memory::DeallocateAligned(m_Data);

        m_Data = p_Other.m_Data;
        m_InstanceCount = p_Other.m_InstanceCount;
        m_InstanceSize = p_Other.m_InstanceSize;
        m_InstanceAlignedSize = p_Other.m_InstanceAlignedSize;
        m_Size = p_Other.m_Size;

        p_Other.m_Data = nullptr;
        p_Other.m_Size = 0;
        p_Other.m_InstanceCount = 0;
        p_Other.m_InstanceAlignedSize = 0;
        p_Other.m_InstanceSize = 0;

        return *this;
    }

    /**
     * @brief Writes data into the buffer, up to the buffer size.
     *
     * Be mindful of the alignment requirements. `p_Data` should be compatible with the alignment of the buffer.
     *
     * @param p_Data A pointer to the data to write into the buffer.
     */
    constexpr void Write(const void *p_Data) noexcept
    {
        Memory::Copy(m_Data, p_Data, m_Size);
    }

    /**
     * @brief Writes data into the buffer, up to the specified size, which must not exceed the buffer's.
     *
     * Be mindful of the alignment requirements. `p_Data` should be compatible with the alignment of the buffer.
     *
     * @param p_Data A pointer to the data to write into the buffer.
     * @param p_Size The size of the data to write into the buffer.
     */
    constexpr void Write(const void *p_Data, const size_type p_Size) noexcept
    {
        TKIT_ASSERT(p_Size <= m_Size, "[TOOLKIT] Size is out of bounds");
        Memory::Copy(m_Data, p_Data, p_Size);
    }

    /**
     * @brief Writes data into the buffer, offsetted and up to the specified size, which must not exceed the buffer's.
     *
     * Be mindful of the alignment requirements. `p_Data` should be compatible with the alignment of the buffer.
     *
     * @param p_Data A pointer to the data to write into the buffer.
     * @param p_Size The size of the data to write into the buffer.
     * @param p_Offset The offset in the buffer to write the data into.
     */
    constexpr void Write(const void *p_Data, const size_type p_Size, const size_type p_Offset) noexcept
    {
        TKIT_ASSERT(p_Offset + p_Size <= m_Size, "[TOOLKIT] Size + offset is out of bounds");
        std::byte *offset = static_cast<std::byte *>(m_Data) + p_Offset;
        Memory::Copy(offset, p_Data, p_Size);
    }

    /**
     * @brief Writes data into the buffer at a specific index.
     *
     * As only one element is written, no special consideration is needed for the alignment.
     *
     * @param p_Index The index in the buffer to write the data into.
     * @param p_Data A pointer to the data to write into the buffer.
     */
    constexpr void WriteAt(const size_type p_Index, const void *p_Data) noexcept
    {
        TKIT_ASSERT(p_Index < m_InstanceCount, "[TOOLKIT] Index is out of bounds");
        std::byte *offset = static_cast<std::byte *>(m_Data) + p_Index * m_InstanceAlignedSize;
        Memory::Copy(offset, p_Data, m_InstanceSize);
    }

    /**
     * @brief Reads data from the buffer at a specific index.
     *
     * @param p_Index The index in the buffer to read the data from.
     * @return A pointer to the data at the specified index.
     */
    constexpr const void *ReadAt(const size_type p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_InstanceCount, "[TOOLKIT] Index is out of bounds");
        const std::byte *offset = static_cast<const std::byte *>(m_Data) + p_Index * m_InstanceAlignedSize;
        return offset;
    }

    /**
     * @brief Reads data from the buffer at a specific index.
     *
     * @param p_Index The index in the buffer to read the data from.
     * @return A pointer to the data at the specified index.
     */
    constexpr void *ReadAt(const size_type p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < m_InstanceCount, "[TOOLKIT] Index is out of bounds");
        std::byte *offset = static_cast<std::byte *>(m_Data) + p_Index * m_InstanceAlignedSize;
        return offset;
    }

    /**
     * @brief Allocates more memory to accommodate a new instance count.
     *
     * This method will copy the existing data to the new memory location and deallocate the old one.
     *
     * @param p_InstanceCount The new instance count to allocate memory for.
     */
    constexpr void Grow(const size_type p_InstanceCount) noexcept
    {
        TKIT_ASSERT(p_InstanceCount > m_InstanceCount, "[TOOLKIT] Cannot shrink buffer");
        TKIT_ASSERT(m_InstanceSize > 0, "[TOOLKIT] Cannot grow buffer whose instances have zero elements");

        const size_type newSize = p_InstanceCount * m_InstanceAlignedSize;
        void *newData = Memory::AllocateAligned(newSize, m_InstanceAlignedSize);
        TKIT_ASSERT(newData, "[TOOLKIT] Failed to allocate memory");

        if (m_Data)
        {
            Memory::Copy(newData, m_Data, m_Size);
            Memory::DeallocateAligned(m_Data);
        }
        m_Data = newData;
        m_Size = newSize;
        m_InstanceCount = p_InstanceCount;
    }

    constexpr const void *GetData() const noexcept
    {
        return m_Data;
    }
    constexpr void *GetData() noexcept
    {
        return m_Data;
    }

    constexpr size_type GetSize() const noexcept
    {
        return m_Size;
    }

    /* compatible with STL */

    constexpr const void *data() const noexcept
    {
        return m_Data;
    }
    constexpr void *data() noexcept
    {
        return m_Data;
    }

    constexpr size_type size() const noexcept
    {
        return m_Size;
    }

    constexpr size_type GetInstanceCount() const noexcept
    {
        return m_InstanceCount;
    }
    constexpr size_type GetInstanceSize() const noexcept
    {
        return m_InstanceSize;
    }
    constexpr size_type GetInstanceAlignedSize() const noexcept
    {
        return m_InstanceAlignedSize;
    }

    constexpr operator bool() const noexcept
    {
        return m_Data != nullptr;
    }

  private:
    static constexpr size_type alignedSize(const size_type p_Size, const size_type p_Alignment) noexcept
    {
        return (p_Size + p_Alignment - 1) & ~(p_Alignment - 1);
    }

    void *m_Data = nullptr;
    size_type m_InstanceCount = 0;
    size_type m_InstanceSize = 0;
    size_type m_InstanceAlignedSize = 0;
    size_type m_Size = 0;
};

template <typename T, typename Traits = std::allocator_traits<Memory::DefaultAllocator<T>>>
    requires(std::is_trivial_v<T>)
class Buffer
{
  public:
    using value_type = T;
    using size_type = typename Traits::size_type;
    using pointer = typename Traits::pointer;
    using const_pointer = typename Traits::const_pointer;
    using reference = value_type &;
    using const_reference = const value_type &;

    constexpr Buffer() noexcept : m_Buffer(0, sizeof(T), alignof(T))
    {
    }
    constexpr Buffer(const size_type p_InstanceCount, const size_type p_InstanceAlignment = alignof(T)) noexcept
        : m_Buffer(p_InstanceCount, sizeof(T), p_InstanceAlignment)
    {
    }

    /**
     * @brief Writes data into the buffer, up to the buffer size.
     *
     * Be mindful of the alignment requirements. `p_Data` should be compatible with the alignment of the buffer.
     *
     * @param p_Data A pointer to the data to write into the buffer.
     */
    constexpr void Write(const_pointer p_Data) noexcept
    {
        m_Buffer.Write(p_Data);
    }

    /**
     * @brief Writes data into the buffer, up to the specified size, which must not exceed the buffer's.
     *
     * Be mindful of the alignment requirements. `p_Data` should be compatible with the alignment of the buffer.
     *
     * @param p_Data A pointer to the data to write into the buffer.
     * @param p_Size The size of the data to write into the buffer.
     */
    constexpr void Write(const_pointer p_Data, const size_type p_Size) noexcept
    {
        m_Buffer.Write(p_Data, p_Size);
    }

    /**
     * @brief Writes data into the buffer, offsetted and up to the specified size, which must not exceed the buffer's.
     *
     * Be mindful of the alignment requirements. `p_Data` should be compatible with the alignment of the buffer.
     *
     * @param p_Data A pointer to the data to write into the buffer.
     * @param p_Size The size of the data to write into the buffer.
     * @param p_Offset The offset in the buffer to write the data into.
     */
    constexpr void Write(const_pointer p_Data, const size_type p_Size, const size_type p_Offset) noexcept
    {
        m_Buffer.Write(p_Data, p_Size, p_Offset);
    }

    /**
     * @brief Writes data into the buffer at a specific index.
     *
     * As only one element is written, no special consideration is needed for the alignment.
     *
     * @param p_Index The index in the buffer to write the data into.
     * @param p_Data A pointer to the data to write into the buffer.
     */
    constexpr void WriteAt(const size_type p_Index, const_pointer p_Data) noexcept
    {
        m_Buffer.WriteAt(p_Index, p_Data);
    }

    /**
     * @brief Reads data from the buffer at a specific index.
     *
     * @param p_Index The index in the buffer to read the data from.
     * @return A pointer to the data at the specified index.
     */
    constexpr const_pointer ReadAt(const size_type p_Index) const noexcept
    {
        return reinterpret_cast<const_pointer>(m_Buffer.ReadAt(p_Index));
    }

    /**
     * @brief Reads data from the buffer at a specific index.
     *
     * @param p_Index The index in the buffer to read the data from.
     * @return A pointer to the data at the specified index.
     */
    constexpr pointer ReadAt(const size_type p_Index) noexcept
    {
        return reinterpret_cast<pointer>(m_Buffer.ReadAt(p_Index));
    }

    /**
     * @brief Allocates more memory to accommodate a new instance count.
     *
     * This method will copy the existing data to the new memory location and deallocate the old one.
     *
     * @param p_InstanceCount The new instance count to allocate memory for.
     */
    constexpr void Grow(const size_type p_InstanceCount) noexcept
    {
        m_Buffer.Grow(p_InstanceCount);
    }

    constexpr const_reference operator[](const size_type p_Index) const noexcept
    {
        return *ReadAt(p_Index);
    }
    constexpr reference operator[](const size_type p_Index) noexcept
    {
        return *ReadAt(p_Index);
    }

    constexpr const_pointer GetData() const noexcept
    {
        return reinterpret_cast<const_pointer>(m_Buffer.GetData());
    }
    constexpr pointer GetData() noexcept
    {
        return reinterpret_cast<pointer>(m_Buffer.GetData());
    }

    constexpr size_type GetSize() const noexcept
    {
        return m_Buffer.GetSize();
    }

    /* compatible with STL */

    constexpr const_pointer data() const noexcept
    {
        return reinterpret_cast<const_pointer>(m_Buffer.GetData());
    }
    constexpr pointer data() noexcept
    {
        return reinterpret_cast<pointer>(m_Buffer.GetData());
    }

    constexpr size_type size() const noexcept
    {
        return m_Buffer.size();
    }

    constexpr size_type GetInstanceCount() const noexcept
    {
        return m_Buffer.GetInstanceCount();
    }
    constexpr size_type GetInstanceSize() const noexcept
    {
        return m_Buffer.GetInstanceSize();
    }
    constexpr size_type GetInstanceAlignedSize() const noexcept
    {
        return m_Buffer.GetInstanceAlignedSize();
    }

    constexpr operator bool() const noexcept
    {
        return m_Buffer;
    }

  private:
    RawBuffer<size_type> m_Buffer{};
};

} // namespace TKit