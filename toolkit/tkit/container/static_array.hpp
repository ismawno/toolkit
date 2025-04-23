#pragma once

#include "tkit/container/container.hpp"
#include "tkit/container/array.hpp"
#include "tkit/utils/concepts.hpp"
#include "tkit/utils/logging.hpp"

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
 * @tparam Traits The traits of the array, to define the types used for the iterators, size, etc.
 */
template <typename T, usize Capacity, typename Traits = Container::ArrayTraits<T>>
    requires(Capacity > 0)
class StaticArray
{
  public:
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = typename Traits::Iterator;
    using ConstIterator = typename Traits::ConstIterator;
    using Tools = Container::ArrayTools<Traits>;

    constexpr StaticArray() noexcept = default;

    template <typename... Args>
    constexpr StaticArray(const SizeType p_Size, const Args &...p_Args) noexcept : m_Size(p_Size)
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(begin(), end(), p_Args...);
    }

    template <std::input_iterator It> constexpr StaticArray(const It p_Begin, const It p_End) noexcept
    {
        m_Size = static_cast<SizeType>(std::distance(p_Begin, p_End));
        TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        Tools::CopyConstructFromRange(begin(), p_Begin, p_End);
    }

    // This constructor WONT include the case M == Capacity (ie, copy constructor)
    template <SizeType M>
    explicit(false) constexpr StaticArray(const StaticArray<ValueType, M, Traits> &p_Other) noexcept
        : m_Size(p_Other.GetSize())
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    // This constructor WONT include the case M == Capacity (ie, move constructor)
    template <SizeType M>
    explicit(false) constexpr StaticArray(StaticArray<ValueType, M, Traits> &&p_Other) noexcept
        : m_Size(p_Other.GetSize())
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        Tools::MoveConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(const StaticArray &p_Other) noexcept : m_Size(p_Other.GetSize())
    {
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(StaticArray &&p_Other) noexcept : m_Size(p_Other.GetSize())
    {
        Tools::MoveConstructFromRange(begin(), p_Other.begin(), p_Other.end());
    }

    constexpr StaticArray(const std::initializer_list<ValueType> p_List) noexcept
        : m_Size(static_cast<SizeType>(p_List.size()))
    {
        TKIT_ASSERT(p_List.size() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        Tools::CopyConstructFromRange(begin(), p_List.begin(), p_List.end());
    }

    ~StaticArray() noexcept
    {
        Clear();
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <SizeType M> constexpr StaticArray &operator=(const StaticArray<ValueType, M, Traits> &p_Other) noexcept
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <SizeType M> constexpr StaticArray &operator=(StaticArray<ValueType, M, Traits> &&p_Other) noexcept
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }

        Tools::MoveAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    constexpr StaticArray &operator=(const StaticArray &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        Tools::CopyAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    constexpr StaticArray &operator=(StaticArray &&p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        Tools::MoveAssignFromRange(begin(), end(), p_Other.begin(), p_Other.end());
        m_Size = p_Other.GetSize();
        return *this;
    }

    /**
     * @brief Insert a new element at the end of the array.
     *
     * The element is constructed in place using the provided arguments.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &Append(Args &&...p_Args) noexcept
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT] Container is already full");
        return *Memory::ConstructFromIterator(begin() + m_Size++, std::forward<Args>(p_Args)...);
    }

    /**
     * @brief Remove the last element from the array.
     *
     */
    constexpr void Pop() noexcept
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT] Container is already empty");
        --m_Size;
        if constexpr (!std::is_trivially_destructible_v<ValueType>)
            Memory::DestructFromIterator(end());
    }

    /**
     * @brief Insert a new element at the specified position.
     *
     * The element is copied or moved into the array.
     *
     * @param p_Pos The position to insert the element at.
     * @param p_Value The value to insert.
     */
    template <typename U>
        requires(std::is_convertible_v<NoCVRef<ValueType>, NoCVRef<U>>)
    constexpr void Insert(const Iterator p_Pos, U &&p_Value) noexcept
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT] Container is already full");
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        Tools::Insert(end(), p_Pos, std::forward<U>(p_Value));
        ++m_Size;
    }

    /**
     * @brief Insert a range of elements at the specified position.
     *
     * The elements are copied into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Begin The beginning of the range to insert.
     * @param p_End The end of the range to insert.
     */
    template <std::input_iterator It> constexpr void Insert(const Iterator p_Pos, It p_Begin, It p_End) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos <= end(), "[TOOLKIT] Iterator is out of bounds");
        TKIT_ASSERT(std::distance(p_Begin, p_End) + m_Size <= Capacity, "[TOOLKIT] New size exceeds capacity");

        m_Size += Tools::Insert(end(), p_Pos, p_Begin, p_End);
    }

    /**
     * @brief Insert a range of elements at the specified position.
     *
     * The elements are copied into the array.
     *
     * @param p_Pos The position to insert the elements at.
     * @param p_Elements The initializer list of elements to insert.
     */
    constexpr void Insert(const Iterator p_Pos, const std::initializer_list<ValueType> p_Elements) noexcept
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
    constexpr void RemoveOrdered(const Iterator p_Pos) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT] Iterator is out of bounds");
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
    constexpr void RemoveOrdered(const Iterator p_Begin, const Iterator p_End) noexcept
    {
        TKIT_ASSERT(p_Begin >= begin() && p_Begin <= end(), "[TOOLKIT] Begin iterator is out of bounds");
        TKIT_ASSERT(p_End >= begin() && p_End <= end(), "[TOOLKIT] End iterator is out of bounds");
        TKIT_ASSERT(m_Size >= std::distance(p_Begin, p_End), "[TOOLKIT] New size is negative");

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
    constexpr void RemoveUnordered(const Iterator p_Pos) noexcept
    {
        TKIT_ASSERT(p_Pos >= begin() && p_Pos < end(), "[TOOLKIT] Iterator is out of bounds");
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
        requires std::constructible_from<ValueType, Args...>
    constexpr void Resize(const SizeType p_Size, const Args &...p_Args) noexcept
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT] Size is bigger than capacity");

        if constexpr (!std::is_trivially_destructible_v<T>)
            if (p_Size < m_Size)
                Memory::DestructRange(begin() + p_Size, end());

        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            if (p_Size > m_Size)
                Memory::ConstructRange(begin() + m_Size, begin() + p_Size, p_Args...);

        m_Size = p_Size;
    }

    constexpr const ValueType &operator[](const SizeType p_Index) const noexcept
    {
        return At(p_Index);
    }
    constexpr ValueType &operator[](const SizeType p_Index) noexcept
    {
        return At(p_Index);
    }
    constexpr const ValueType &At(const SizeType p_Index) const noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }
    constexpr ValueType &At(const SizeType p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < m_Size, "[TOOLKIT] Index is out of bounds");
        return *(begin() + p_Index);
    }

    constexpr const ValueType &GetFront() const noexcept
    {
        return At(0);
    }
    constexpr ValueType &GetFront() noexcept
    {
        return At(0);
    }

    constexpr const ValueType &GetBack() const noexcept
    {
        return At(m_Size - 1);
    }
    constexpr ValueType &GetBack() noexcept
    {
        return At(m_Size - 1);
    }

    /**
     * @brief Clear the array and set its size to 0.
     *
     * The elements are destroyed if not trivially destructible. The memory is not deallocated.
     *
     */
    constexpr void Clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructRange(begin(), end());
        m_Size = 0;
    }

    constexpr const ValueType *GetData() const noexcept
    {
        return reinterpret_cast<const ValueType *>(&m_Data[0]);
    }

    constexpr ValueType *GetData() noexcept
    {
        return reinterpret_cast<ValueType *>(&m_Data[0]);
    }

    constexpr Iterator begin() noexcept
    {
        return GetData();
    }

    constexpr Iterator end() noexcept
    {
        return begin() + m_Size;
    }

    constexpr ConstIterator begin() const noexcept
    {
        return GetData();
    }

    constexpr ConstIterator end() const noexcept
    {
        return begin() + m_Size;
    }

    constexpr SizeType GetSize() const noexcept
    {
        return m_Size;
    }

    constexpr SizeType GetCapacity() const noexcept
    {
        return Capacity;
    }

    constexpr bool IsEmpty() const noexcept
    {
        return m_Size == 0;
    }

    constexpr bool IsFull() const noexcept
    {
        return m_Size >= Capacity;
    }

  private:
    struct alignas(ValueType) Element
    {
        std::byte Data[sizeof(ValueType)];
    };

    static_assert(sizeof(Element) == sizeof(ValueType), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(ValueType), "Element alignment is not equal to T alignment");

    Array<Element, Capacity> m_Data{};
    SizeType m_Size = 0;
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