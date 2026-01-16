#pragma once

#include "tkit/container/container.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
/**
 * @brief A resizable array with a fixed capacity buffer.
 *
 * It is meant to be used when the user nows at compile time the maximum size the array may reach. This container does
 * not perform any heap allocations.
 *
 * @tparam T The type of the elements in the array.
 * @tparam Capacity The capacity of the array.
 */
template <typename T, usize Capacity>
    requires(Capacity > 0)
class StaticArray
{
  public:
    using ValueType = T;
    using Tools = Container::ArrayTools<T>;

    constexpr StaticArray() = default;

    template <typename... Args>
    constexpr explicit StaticArray(const usize p_Size, const Args &...p_Args) : m_Size(p_Size)
    {
        TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})", m_Size,
                    Capacity);
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), p_Args...);
    }

    template <std::input_iterator It>
    constexpr StaticArray(const It p_Begin, const It p_End)
        requires(std::is_copy_constructible_v<T>)
    {
        m_Size = static_cast<usize>(std::distance(p_Begin, p_End));
        TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})", m_Size,
                    Capacity);
        Tools::CopyConstructFromRange(begin(), p_Begin, p_End);
    }

    constexpr StaticArray(const std::initializer_list<T> p_List)
        requires(std::is_copy_constructible_v<T>)
        : m_Size(static_cast<usize>(p_List.size()))
    {
        TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})", m_Size,
                    Capacity);
        Tools::CopyConstructFromRange(begin(), p_List.begin(), p_List.end());
    }

    // This constructor WONT include the case M == Capacity (ie, copy constructor)
    template <usize M>
    constexpr StaticArray(const StaticArray<T, M> &p_Other)
        requires(std::is_copy_constructible_v<T>)
        : m_Size(p_Other.GetSize())
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})", m_Size,
                        Capacity);
        }
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    // This constructor WONT include the case M == Capacity (ie, move constructor)
    template <usize M>
    constexpr StaticArray(StaticArray<T, M> &&p_Other)
        requires(std::is_move_constructible_v<T>)
        : m_Size(p_Other.GetSize())
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})", m_Size,
                        Capacity);
        }
        Tools::MoveConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(const StaticArray &p_Other)
        requires(std::is_copy_constructible_v<T>)
        : m_Size(p_Other.GetSize())
    {
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(StaticArray &&p_Other)
        requires(std::is_move_constructible_v<T>)
        : m_Size(p_Other.GetSize())
    {
        Tools::MoveConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    ~StaticArray()
    {
        Clear();
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <usize M>
    constexpr StaticArray &operator=(const StaticArray<T, M> &p_Other)
        requires(std::is_copy_assignable_v<T>)
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})",
                        p_Other.GetSize(), Capacity);
        }
        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <usize M>
    constexpr StaticArray &operator=(StaticArray<T, M> &&p_Other)
        requires(std::is_move_assignable_v<T>)
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})",
                        p_Other.GetSize(), Capacity);
        }

        Tools::MoveAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    constexpr StaticArray &operator=(const StaticArray &p_Other)
    {
        if (this == &p_Other)
            return *this;

        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    constexpr StaticArray &operator=(StaticArray &&p_Other)
    {
        if (this == &p_Other)
            return *this;

        Tools::MoveAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T &Append(Args &&...p_Args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][STAT-ARRAY] Container is already full");
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(p_Args)...);
    }

    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][STAT-ARRAY] Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(end());
    }

    template <typename U>
        requires(std::convertible_to<std::remove_cvref_t<T>, std::remove_cvref_t<U>>)
    constexpr void Insert(T *p_Pos, U &&p_Value)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][STAT-ARRAY] Container is already full");
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][STAT-ARRAY] Iterator is out of bounds");
        Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
        ++m_Size;
    }

    template <std::input_iterator It> constexpr void Insert(T *p_Pos, It p_Begin, It p_End)
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT][STAT-ARRAY] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= Capacity,
                    "[TOOLKIT][STAT-ARRAY] New size exceeds capacity");

        m_Size += Tools::Insert(end(), p_Pos, p_Begin, p_End);
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
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][STAT-ARRAY] Iterator is out of bounds");
        Tools::RemoveOrdered(end(), p_Pos);
        --m_Size;
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
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT][STAT-ARRAY] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT][STAT-ARRAY] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT][STAT-ARRAY] Range overflows array");

        m_Size -= Tools::RemoveOrdered(end(), p_Begin, p_End);
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
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT][STAT-ARRAY] Iterator is out of bounds");
        Tools::RemoveUnordered(end(), p_Pos);
        --m_Size;
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
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr void Resize(const usize p_Size, const Args &...p_Args)
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT][STAT-ARRAY] Size ({}) is bigger than capacity ({})", p_Size,
                    Capacity);

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, p_Args...);

        m_Size = p_Size;
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
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][STAT-ARRAY] ");
        return *(begin() + p_Index);
    }
    constexpr T &At(const usize p_Index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, m_Size, "[TOOLKIT][STAT-ARRAY] ");
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
        return At(m_Size - 1);
    }
    constexpr T &GetBack()
    {
        return At(m_Size - 1);
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr const T *GetData() const
    {
        return reinterpret_cast<const T *>(&m_Data[0]);
    }

    constexpr T *GetData()
    {
        return reinterpret_cast<T *>(&m_Data[0]);
    }

    constexpr T *begin()
    {
        return GetData();
    }

    constexpr T *end()
    {
        return begin() + m_Size;
    }

    constexpr const T *begin() const
    {
        return GetData();
    }

    constexpr const T *end() const
    {
        return begin() + m_Size;
    }

    constexpr usize GetSize() const
    {
        return m_Size;
    }

    constexpr usize GetCapacity() const
    {
        return Capacity;
    }

    constexpr bool IsEmpty() const
    {
        return m_Size == 0;
    }

    constexpr bool IsFull() const
    {
        return m_Size >= Capacity;
    }

  private:
    struct alignas(T) Element
    {
        std::byte Data[sizeof(T)];
    };

    static_assert(sizeof(Element) == sizeof(T), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(T), "Element alignment is not equal to T alignment");

    FixedArray<Element, Capacity> m_Data{};
    usize m_Size = 0;
};

template <typename T> using StaticArray4 = StaticArray<T, 4>;
template <typename T> using StaticArray8 = StaticArray<T, 8>;
template <typename T> using StaticArray16 = StaticArray<T, 16>;
template <typename T> using StaticArray32 = StaticArray<T, 32>;
template <typename T> using StaticArray64 = StaticArray<T, 64>;
template <typename T> using StaticArray128 = StaticArray<T, 128>;
template <typename T> using StaticArray196 = StaticArray<T, 196>;
template <typename T> using StaticArray256 = StaticArray<T, 256>;
template <typename T> using StaticArray384 = StaticArray<T, 384>;
template <typename T> using StaticArray512 = StaticArray<T, 512>;
template <typename T> using StaticArray768 = StaticArray<T, 768>;
template <typename T> using StaticArray1024 = StaticArray<T, 1024>;
} // namespace TKit
