#pragma once

#include "tkit/container/container.hpp"

namespace TKit
{
enum ArrayType : u8
{
    Array_Static,
    Array_Dynamic,
    Array_Arena,
    Array_Stack,
    Array_Tier
};

template <typename T, typename AllocState> class Array
{
  public:
    static constexpr bool Safeguard = true;
    static constexpr ArrayType Type = AllocState::Type;

    using ValueType = T;
    using Tools = Container::ArrayTools<T>;

    template <typename... Args>
        requires(sizeof...(Args) > 1 || ((!std::is_same_v<Array, std::remove_cvref_t<Args>>) && ... && true))
    constexpr Array(Args &&...args) : m_State(std::forward<Args>(args)...)
    {
    }

    template <std::convertible_to<usize> U, typename... Args>
    constexpr explicit Array(const U psize, Args &&...args) : m_State(std::forward<Args>(args)...)
    {
        const usize size = static_cast<usize>(psize);
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(size > 0, size);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(size);
            }
            TKIT_ASSERT(size <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})", size,
                        m_State.GetCapacity());
        }
        m_State.Size = size;
        if constexpr (!std::is_trivially_default_constructible_v<T>)
            ConstructRange(begin(), end());
    }

    template <std::input_iterator It, typename... Args>
    constexpr Array(const It pbegin, const It pend, Args &&...args) : m_State(std::forward<Args>(args)...)
    {
        const usize size = static_cast<usize>(std::distance(pbegin, pend));
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(size > 0, size);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(size);
            }
            TKIT_ASSERT(size <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})", size,
                        m_State.GetCapacity());
        }
        m_State.Size = size;
        Tools::CopyConstructFromRange(begin(), pbegin, pend);
    }

    template <typename... Args>
    constexpr Array(const std::initializer_list<T> list, Args &&...args) : m_State(std::forward<Args>(args)...)
    {
        const usize size = static_cast<usize>(list.size());
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(size > 0, size);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(size);
            }
            TKIT_ASSERT(size <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})", size,
                        m_State.GetCapacity());
        }
        m_State.Size = size;
        Tools::CopyConstructFromRange(begin(), list.begin(), list.end());
    }

    constexpr Array(const Array &other)
    {
        const usize otherSize = other.m_State.Size;
        if constexpr (Type == Array_Dynamic)
            m_State.GrowCapacityIf(otherSize > 0, otherSize);
        else if constexpr (Type == Array_Arena || Type == Array_Stack || Type == Array_Tier)
        {
            m_State.Allocator = other.m_State.Allocator;
            if constexpr (Type == Array_Tier)
                m_State.GrowCapacityIf(otherSize > 0, otherSize);
            else
                m_State.Allocate(other.m_State.Capacity);
        }
        else
        {
            TKIT_ASSERT(otherSize <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        otherSize, m_State.GetCapacity());
        }

        m_State.Size = otherSize;
        Tools::CopyConstructFromRange(begin(), other.begin(), other.end());
    }

    template <typename OtherAlloc> constexpr Array(const Array<T, OtherAlloc> &other)
    {
        const usize otherSize = other.m_State.Size;
        if constexpr (Type == Array_Dynamic)
            m_State.GrowCapacityIf(otherSize > 0, otherSize);
        else if constexpr (Type == Array_Arena || Type == Array_Stack || Type == Array_Tier)
        {
            if constexpr (Type == Array_Tier)
                m_State.GrowCapacityIf(otherSize > 0, otherSize);
            else
                m_State.Allocate(other.m_State.Capacity);
        }
        else
        {
            TKIT_ASSERT(otherSize <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        otherSize, m_State.GetCapacity());
        }

        m_State.Size = otherSize;
        Tools::CopyConstructFromRange(begin(), other.begin(), other.end());
    }

    constexpr Array(Array &&other)
    {
        if constexpr (Type == Array_Static)
        {
            m_State.Size = other.m_State.Size;
            Tools::MoveConstructFromRange(begin(), other.begin(), other.end());
        }
        else
            m_State = std::move(other.m_State);
    }

    ~Array()
    {
        Clear();
        if constexpr (Type == Array_Dynamic || Type == Array_Stack || Type == Array_Tier)
            m_State.Deallocate();
    }

    constexpr Array &operator=(const Array &other)
    {
        if (this == &other)
            return *this;

        const usize otherSize = other.m_State.Size;
        if constexpr (Type == Array_Dynamic)
            m_State.GrowCapacityIf(otherSize > m_State.Capacity, otherSize);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Allocator)
                {
                    TKIT_ASSERT(!m_State.Data,
                                "[TOOLKIT][ARRAY] If the array has no allocator, it should not be allowed "
                                "to have an active allocation");
                    TKIT_ASSERT(m_State.Capacity == 0,
                                "[TOOLKIT][ARRAY] If the array has no allocator, it should not be allowed "
                                "to have an active capacity");
                    m_State.Allocator = other.m_State.Allocator;
                }
                if (!m_State.Data)
                    m_State.Allocate(other.m_State.Capacity);
                else if constexpr (Type == Array_Tier)
                    m_State.GrowCapacityIf(otherSize > m_State.Capacity, otherSize);
            }
            if constexpr (Type != Array_Tier)
            {
                TKIT_ASSERT(otherSize <= m_State.GetCapacity(),
                            "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})", otherSize,
                            m_State.GetCapacity());
            }
        }
        Tools::CopyAssignFromRange(begin(), end(), other.begin(), other.end());
        m_State.Size = otherSize;
        return *this;
    }

    template <typename OtherAlloc> constexpr Array &operator=(const Array<T, OtherAlloc> &other)
    {
        const usize otherSize = other.m_State.Size;
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(otherSize > m_State.Capacity, otherSize);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(other.m_State.Capacity);
            }
            TKIT_ASSERT(otherSize <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        otherSize, m_State.GetCapacity());
        }

        Tools::CopyAssignFromRange(begin(), end(), other.begin(), other.end());
        m_State.Size = otherSize;
        return *this;
    }

    constexpr Array &operator=(Array &&other)
    {
        if (this == &other)
            return *this;

        if constexpr (Type != Array_Static)
        {
            Clear();
            if constexpr (Type != Array_Arena)
                m_State.Deallocate();
            m_State = std::move(other.m_State);
        }
        else
        {
            Tools::MoveAssignFromRange(begin(), end(), other.begin(), other.end());
            m_State.Size = other.m_State.Size;
        }
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &Append(Args &&...args)
    {
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            const usize newSize = m_State.Size + 1;
            m_State.GrowCapacityIf(newSize > m_State.GetCapacity(), newSize);
            m_State.Size = newSize;
            return *ConstructFromIterator(end() - 1, std::forward<Args>(args)...);
        }
        else
        {
            TKIT_ASSERT(!IsFull(), "[TOOLKIT][ARRAY] Container is already at capacity of {}", m_State.GetCapacity());
            return *ConstructFromIterator(begin() + m_State.Size++, std::forward<Args>(args)...);
        }
    }

    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][ARRAY] Cannot Pop(). Container is already empty");
        --m_State.Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
            DestructFromIterator(end());
    }

    template <typename U>
        requires(std::convertible_to<std::remove_cvref_t<T>, std::remove_cvref_t<U>>)
    constexpr void Insert(T *ppos, U &&value)
    {
        TKIT_ASSERT(ppos >= begin() && ppos <= end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            const usize newSize = m_State.Size + 1;
            if (newSize > m_State.GetCapacity())
            {
                const usize pos = static_cast<usize>(std::distance(begin(), ppos));
                m_State.GrowCapacity(newSize);
                ppos = begin() + pos;
            }

            Tools::Insert(end(), ppos, std::forward<U>(value));
            m_State.Size = newSize;
        }
        else
        {
            TKIT_ASSERT(!IsFull(), "[TOOLKIT][ARRAY] Container is already full");
            Tools::Insert(end(), ppos, std::forward<U>(value));
            ++m_State.Size;
        }
    }

    template <std::input_iterator It> constexpr void Insert(T *ppos, const It pbegin, const It pend)
    {
        TKIT_ASSERT(ppos >= begin() && ppos <= end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            const usize newSize = m_State.Size + static_cast<usize>(std::distance(pbegin, pend));
            if (newSize > m_State.GetCapacity())
            {
                const usize pos = static_cast<usize>(std::distance(begin(), ppos));
                m_State.GrowCapacity(newSize);
                ppos = begin() + pos;
            }

            Tools::Insert(end(), ppos, pbegin, pend);
            m_State.Size = newSize;
        }
        else
        {
            TKIT_ASSERT(std::distance(pbegin, pend) + m_State.Size <= m_State.GetCapacity(),
                        "[TOOLKIT][ARRAY] New size exceeds capacity");

            m_State.Size += Tools::Insert(end(), ppos, pbegin, pend);
        }
    }

    constexpr void Insert(T *pos, const std::initializer_list<T> elements)
    {
        Insert(pos, elements.begin(), elements.end());
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * All elements "to the right" of `pos` are shifted, and the order is preserved.
     *
     * @param pos The position to remove the element at.
     */
    constexpr void RemoveOrdered(T *pos)
    {
        TKIT_ASSERT(pos >= begin() && pos < end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        Tools::RemoveOrdered(end(), pos);
        --m_State.Size;
    }

    /**
     * @brief Remove a range of elements.
     *
     * All elements "to the right" of the range are shifted, and the order is preserved.
     *
     * @param begin The beginning of the range to erase.
     * @param end The end of the range to erase.
     */
    constexpr void RemoveOrdered(T *pbegin, T *pend)
    {
        TKIT_ASSERT(pbegin >= begin() && pbegin <= end(), "[TOOLKIT][ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(pend >= begin() && pend <= end(), "[TOOLKIT][ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_State.Size >= std::distance(pbegin, pend), "[TOOLKIT][ARRAY] Range overflows array");

        m_State.Size -= Tools::RemoveOrdered(end(), pbegin, pend);
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * The last element is moved into the position of the removed element, which is then popped. The order is not
     * preserved.
     *
     * @param pos The position to remove the element at.
     */
    constexpr void RemoveUnordered(T *pos)
    {
        TKIT_ASSERT(pos >= begin() && pos < end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        Tools::RemoveUnordered(end(), pos);
        --m_State.Size;
    }

    constexpr const T &operator[](const usize index) const
    {
        return At(index);
    }
    constexpr T &operator[](const usize index)
    {
        return At(index);
    }
    constexpr const T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_State.Size, "[TOOLKIT][ARRAY] ");
        return *(begin() + index);
    }
    constexpr T &At(const usize index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, m_State.Size, "[TOOLKIT][ARRAY] ");
        return *(begin() + index);
    }

    constexpr const T &GetFront() const
    {
        return At(0);
    }
    constexpr T &GetFront()
    {
        return At(0);
    }

    constexpr const T &GetBack() const
    {
        return At(m_State.Size - 1);
    }
    constexpr T &GetBack()
    {
        return At(m_State.Size - 1);
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            if (GetData())
                DestructRange(begin(), end());
        m_State.Size = 0;
    }

    /**
     * @brief Resize the array.
     *
     * If the new size is smaller than the current size, the elements are destroyed. If the new size is bigger than the
     * current size, the elements are constructed in place.
     *
     * @param size The new size of the array.
     * @param args The arguments to pass to the constructor of `T` (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args> constexpr void Resize(const usize size, const Args &...args)
    {
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(size > m_State.GetCapacity(), size);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(size);
            }
            TKIT_ASSERT(size <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})", size,
                        m_State.GetCapacity());
        }

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (size < m_State.Size)
                DestructRange(begin() + size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (size > m_State.Size)
                ConstructRange(begin() + m_State.Size, begin() + size, args...);

        m_State.Size = size;
    }

    constexpr void Reserve(const usize capacity)
        requires(Type != Array_Static)
    {
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            if (capacity > m_State.Capacity)
                m_State.ModifyCapacity(capacity);
        }
        else if (capacity != 0)
            m_State.Allocate(capacity);
    }

    // nullptr will grab the current pushed allocator on next Allocate()
    template <typename Allocator> constexpr void ResetAllocator(Allocator *allocator = nullptr)
    {
        if (m_State.Allocator)
            m_State.Deallocate();
        m_State.Allocator = allocator;
    }
    constexpr void Allocate(const usize capacity)
        requires(Type != Array_Static)
    {
        m_State.Allocate(capacity);
    }
    template <typename... Args>
    constexpr void Create(const usize size, Args &&...args)
        requires(Type != Array_Static)
    {
        m_State.Allocate(size);
        m_State.Size = size;
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            ConstructRange(begin(), end(), std::forward<Args>(args)...);
    }
    template <typename Allocator>
    constexpr void Allocate(Allocator *allocator, const usize capacity)
        requires(Type != Array_Static && Type != Array_Dynamic)
    {
        // TKIT_ASSERT(!m_State.Allocator, "[TOOLKIT][ARRAY] Array state has already an active allocator and cannot be "
        //                                 "replaced. Use the Allocate() overload that does not accept an allocator");
        ResetAllocator(allocator);

        m_State.Allocator = allocator;
        m_State.Allocate(capacity);
    }
    template <typename Allocator, typename... Args>
    constexpr void Create(Allocator *allocator, const usize size, Args &&...args)
        requires(Type != Array_Static && Type != Array_Dynamic)
    {
        // TKIT_ASSERT(!m_State.Allocator, "[TOOLKIT][ARRAY] Array state has already an active allocator and cannot be "
        //                                 "replaced. Use the Allocate() overload that does not accept an allocator");
        ResetAllocator(allocator);

        m_State.Allocator = allocator;
        m_State.Allocate(size);
        m_State.Size = size;
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            ConstructRange(begin(), end(), std::forward<Args>(args)...);
    }

    constexpr void Shrink()
        requires(Type == Array_Dynamic || Type == Array_Tier)
    {
        if (m_State.Size == 0)
            m_State.Deallocate();
        else
            m_State.ModifyCapacity(m_State.Size);
    }

    constexpr void Deallocate()
        requires(Type != Array_Static && Type != Array_Arena)
    {
        m_State.Deallocate();
    }

    constexpr const T *GetData() const
    {
        const T *ptr = reinterpret_cast<const T *>(std::addressof(m_State.Data[0]));
        TKIT_ASSERT(ptr || m_State.Size == 0,
                    "[TOOLKIT][ARRAY] If the data of an array is null, its size must be zero");
        TKIT_ASSERT(ptr || m_State.GetCapacity() == 0,
                    "[TOOLKIT][ARRAY] If the data of an array is null, its capacity must be zero");
        TKIT_ASSERT(!ptr || m_State.GetCapacity() != 0,
                    "[TOOLKIT][ARRAY] If the data of an array is not null, its capacity must be greater than 0");
        return ptr;
    }

    constexpr T *GetData()
    {
        T *ptr = reinterpret_cast<T *>(std::addressof(m_State.Data[0]));
        TKIT_ASSERT(ptr || m_State.Size == 0,
                    "[TOOLKIT][ARRAY] If the data of an array is null, its size must be zero");
        TKIT_ASSERT(ptr || m_State.GetCapacity() == 0,
                    "[TOOLKIT][ARRAY] If the data of an array is null, its capacity must be zero");
        TKIT_ASSERT(!ptr || m_State.GetCapacity() != 0,
                    "[TOOLKIT][ARRAY] If the data of an array is not null, its capacity must be greater than 0");
        return ptr;
    }

    constexpr T *begin()
    {
        return GetData();
    }

    constexpr T *end()
    {
        return begin() + m_State.Size;
    }

    constexpr const T *begin() const
    {
        return GetData();
    }

    constexpr const T *end() const
    {
        return begin() + m_State.Size;
    }

    constexpr usize GetSize() const
    {
        return m_State.Size;
    }

    constexpr usize GetCapacity() const
    {
        return m_State.GetCapacity();
    }

    constexpr bool IsEmpty() const
    {
        return m_State.Size == 0;
    }

    constexpr bool IsFull() const
    {
        return m_State.Size == m_State.GetCapacity();
    }

  private:
    AllocState m_State{};

    template <typename U, typename OtherAlloc> friend class Array;
};
} // namespace TKit
