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
    constexpr Array(Args &&...p_Args) : m_State(std::forward<Args>(p_Args)...)
    {
    }

    template <std::convertible_to<usize> U, typename... Args>
    constexpr explicit Array(const U p_Size, Args &&...p_Args) : m_State(std::forward<Args>(p_Args)...)
    {
        const usize size = static_cast<usize>(p_Size);
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
            Memory::ConstructRange(begin(), end());
    }

    template <std::input_iterator It, typename... Args>
    constexpr Array(const It p_Begin, const It p_End, Args &&...p_Args) : m_State(std::forward<Args>(p_Args)...)
    {
        const usize size = static_cast<usize>(std::distance(p_Begin, p_End));
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
        Tools::CopyConstructFromRange(begin(), p_Begin, p_End);
    }

    template <typename... Args>
    constexpr Array(const std::initializer_list<T> p_List, Args &&...p_Args) : m_State(std::forward<Args>(p_Args)...)
    {
        const usize size = static_cast<usize>(p_List.size());
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
        Tools::CopyConstructFromRange(begin(), p_List.begin(), p_List.end());
    }

    constexpr Array(const Array &p_Other)
    {
        const usize otherSize = p_Other.m_State.Size;
        if constexpr (Type == Array_Dynamic)
            m_State.GrowCapacityIf(otherSize > 0, otherSize);
        else if constexpr (Type == Array_Arena || Type == Array_Stack || Type == Array_Tier)
        {
            m_State.Allocator = p_Other.m_State.Allocator;
            if constexpr (Type == Array_Tier)
                m_State.GrowCapacityIf(otherSize > 0, otherSize);
            else
                m_State.Allocate(p_Other.m_State.Capacity);
        }
        else
        {
            TKIT_ASSERT(otherSize <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        otherSize, m_State.GetCapacity());
        }

        m_State.Size = otherSize;
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    template <typename OtherAlloc> constexpr Array(const Array<T, OtherAlloc> &p_Other)
    {
        const usize otherSize = p_Other.m_State.Size;
        if constexpr (Type == Array_Dynamic)
            m_State.GrowCapacityIf(otherSize > 0, otherSize);
        else if constexpr (Type == Array_Arena || Type == Array_Stack || Type == Array_Tier)
        {
            if constexpr (Type == Array_Tier)
                m_State.GrowCapacityIf(otherSize > 0, otherSize);
            else
                m_State.Allocate(p_Other.m_State.Capacity);
        }
        else
        {
            TKIT_ASSERT(otherSize <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        otherSize, m_State.GetCapacity());
        }

        m_State.Size = otherSize;
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr Array(Array &&p_Other)
    {
        if constexpr (Type == Array_Static)
        {
            m_State.Size = p_Other.m_State.Size;
            Tools::MoveConstructFromRange(begin(), p_Other.begin(), p_Other.end());
        }
        else
            m_State = std::move(p_Other.m_State);
    }

    ~Array()
    {
        Clear();
        if constexpr (Type == Array_Dynamic || Type == Array_Stack || Type == Array_Tier)
            m_State.Deallocate();
    }

    constexpr Array &operator=(const Array &p_Other)
    {
        if (this == &p_Other)
            return *this;

        const usize otherSize = p_Other.m_State.Size;
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
                    m_State.Allocator = p_Other.m_State.Allocator;
                }
                if (!m_State.Data)
                    m_State.Allocate(p_Other.m_State.Capacity);
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
        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_State.Size = otherSize;
        return *this;
    }

    template <typename OtherAlloc> constexpr Array &operator=(const Array<T, OtherAlloc> &p_Other)
    {
        if (this == &p_Other)
            return *this;

        const usize otherSize = p_Other.m_State.Size;
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(otherSize > m_State.Capacity, otherSize);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(p_Other.m_State.Capacity);
            }
            TKIT_ASSERT(otherSize <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        otherSize, m_State.GetCapacity());
        }

        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_State.Size = otherSize;
        return *this;
    }

    constexpr Array &operator=(Array &&p_Other)
    {
        if (this == &p_Other)
            return *this;

        if constexpr (Type != Array_Static)
        {
            Clear();
            if constexpr (Type != Array_Arena)
                m_State.Deallocate();
            m_State = std::move(p_Other.m_State);
        }
        else
        {
            Tools::MoveAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
            m_State.Size = p_Other.m_State.Size;
        }
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &Append(Args &&...p_Args)
    {
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            const usize newSize = m_State.Size + 1;
            m_State.GrowCapacityIf(newSize > m_State.GetCapacity(), newSize);
            m_State.Size = newSize;
            return *Memory::ConstructFromIterator(end() - 1, std::forward<Args>(p_Args)...);
        }
        else
        {
            TKIT_ASSERT(!IsFull(), "[TOOLKIT][ARRAY] Container is already at capacity of {}", m_State.GetCapacity());
            return *Memory::ConstructFromIterator(begin() + m_State.Size++, std::forward<Args>(p_Args)...);
        }
    }

    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][ARRAY] Cannot Pop(). Container is already empty");
        --m_State.Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(end());
    }

    template <typename U>
        requires(std::convertible_to<std::remove_cvref_t<T>, std::remove_cvref_t<U>>)
    constexpr void Insert(T *p_Pos, U &&p_Value)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            const usize newSize = m_State.Size + 1;
            if (newSize > m_State.GetCapacity())
            {
                const usize pos = static_cast<usize>(std::distance(begin(), p_Pos));
                m_State.GrowCapacity(newSize);
                p_Pos = begin() + pos;
            }

            Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
            m_State.Size = newSize;
        }
        else
        {
            TKIT_ASSERT(!IsFull(), "[TOOLKIT][ARRAY] Container is already full");
            Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
            ++m_State.Size;
        }
    }

    template <std::input_iterator It> constexpr void Insert(T *p_Pos, It p_Begin, It p_End)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            const usize newSize = m_State.Size + static_cast<usize>(std::distance(p_Begin, p_End));
            if (newSize > m_State.GetCapacity())
            {
                const usize pos = static_cast<usize>(std::distance(begin(), p_Pos));
                m_State.GrowCapacity(newSize);
                p_Pos = begin() + pos;
            }

            Tools::Insert(end(), p_Pos, p_Begin, p_End);
            m_State.Size = newSize;
        }
        else
        {
            TKIT_ASSERT(std::distance(p_Begin, p_End) + m_State.Size <= m_State.GetCapacity(),
                        "[TOOLKIT][ARRAY] New size exceeds capacity");

            m_State.Size += Tools::Insert(end(), p_Pos, p_Begin, p_End);
        }
    }

    constexpr void Insert(T *p_Pos, const std::initializer_list<T> p_Elements)
    {
        Insert(p_Pos, p_Elements.begin(), p_Elements.end());
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * All elements "to the right" of `p_Pos` are shifted, and the order is preserved.
     *
     * @param p_Pos The position to remove the element at.
     */
    constexpr void RemoveOrdered(T *p_Pos)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        Tools::RemoveOrdered(end(), p_Pos);
        --m_State.Size;
    }

    /**
     * @brief Remove a range of elements.
     *
     * All elements "to the right" of the range are shifted, and the order is preserved.
     *
     * @param p_Begin The beginning of the range to erase.
     * @param p_End The end of the range to erase.
     */
    constexpr void RemoveOrdered(T *p_Begin, T *p_End)
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT][ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT][ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_State.Size >= std::distance(p_Begin, p_End), "[TOOLKIT][ARRAY] Range overflows array");

        m_State.Size -= Tools::RemoveOrdered(end(), p_Begin, p_End);
    }

    /**
     * @brief Remove the element at the specified position.
     *
     * The last element is moved into the position of the removed element, which is then popped. The order is not
     * preserved.
     *
     * @param p_Pos The position to remove the element at.
     */
    constexpr void RemoveUnordered(T *p_Pos)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        Tools::RemoveUnordered(end(), p_Pos);
        --m_State.Size;
    }

    constexpr const T &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr T &operator[](const usize p_Index)
    {
        return At(p_Index);
    }
    constexpr const T &At(const usize p_Index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_State.Size, "[TOOLKIT][ARRAY] ");
        return *(begin() + p_Index);
    }
    constexpr T &At(const usize p_Index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_State.Size, "[TOOLKIT][ARRAY] ");
        return *(begin() + p_Index);
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
                Memory::DestructRange(begin(), end());
        m_State.Size = 0;
    }

    /**
     * @brief Resize the array.
     *
     * If the new size is smaller than the current size, the elements are destroyed. If the new size is bigger than the
     * current size, the elements are constructed in place.
     *
     * @param p_Size The new size of the array.
     * @param args The arguments to pass to the constructor of `T` (only used if the new size is bigger than the current
     * size.)
     */
    template <typename... Args> constexpr void Resize(const usize p_Size, const Args &...p_Args)
    {
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(p_Size > m_State.GetCapacity(), p_Size);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(p_Size);
            }
            TKIT_ASSERT(p_Size <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        p_Size, m_State.GetCapacity());
        }

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_State.Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_State.Size)
                Memory::ConstructRange(begin() + m_State.Size, begin() + p_Size, p_Args...);

        m_State.Size = p_Size;
    }

    constexpr void Reserve(const usize p_Capacity)
        requires(Type != Array_Static)
    {
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            if (p_Capacity > m_State.Capacity)
                m_State.ModifyCapacity(p_Capacity);
        }
        else if (p_Capacity != 0)
            m_State.Allocate(p_Capacity);
    }

    constexpr void Allocate(const usize p_Capacity)
        requires(Type != Array_Static)
    {
        m_State.Allocate(p_Capacity);
    }
    template <typename... Args>
    constexpr void Create(const usize p_Size, Args &&...p_Args)
        requires(Type != Array_Static)
    {
        m_State.Allocate(p_Size);
        m_State.Size = p_Size;
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), std::forward<Args>(p_Args)...);
    }
    template <typename Allocator>
    constexpr void Allocate(Allocator *p_Allocator, const usize p_Capacity)
        requires(Type != Array_Static && Type != Array_Dynamic)
    {
        TKIT_ASSERT(!m_State.Allocator, "[TOOLKIT][ARRAY] Array state has already an active allocator and cannot be "
                                        "replaced. Use the Allocate() overload that does not accept an allocator");
        m_State.Allocator = p_Allocator;
        m_State.Allocate(p_Capacity);
    }
    template <typename Allocator, typename... Args>
    constexpr void Create(Allocator *p_Allocator, const usize p_Size, Args &&...p_Args)
        requires(Type != Array_Static && Type != Array_Dynamic)
    {
        TKIT_ASSERT(!m_State.Allocator, "[TOOLKIT][ARRAY] Array state has already an active allocator and cannot be "
                                        "replaced. Use the Allocate() overload that does not accept an allocator");
        m_State.Allocator = p_Allocator;
        m_State.Allocate(p_Size);
        m_State.Size = p_Size;
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), std::forward<Args>(p_Args)...);
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
        const T *ptr = reinterpret_cast<const T *>(&m_State.Data[0]);
        TKIT_ASSERT(ptr || m_State.Size == 0, "[TOOLKIT][ARRAY] If the data of an array is null, it size must be zero");
        TKIT_ASSERT(ptr || m_State.GetCapacity() == 0,
                    "[TOOLKIT][ARRAY] If the dat of an array is null, its capacity must be zero");
        TKIT_ASSERT(!ptr || m_State.GetCapacity() != 0,
                    "[TOOLKIT][ARRAY] If the data of an array is not null, its capacity must be greater than 0");
        return ptr;
    }

    constexpr T *GetData()
    {
        T *ptr = reinterpret_cast<T *>(&m_State.Data[0]);
        TKIT_ASSERT(ptr || m_State.Size == 0, "[TOOLKIT][ARRAY] If the data of an array is null, it size must be zero");
        TKIT_ASSERT(ptr || m_State.GetCapacity() == 0,
                    "[TOOLKIT][ARRAY] If the dat of an array is null, its capacity must be zero");
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
