#pragma once

#include "tkit/container/container.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
/**
 * @brief A circular container with a fixed capacity buffer designed for quick insertions at its front and back.
 *
 * It is meant to be used as a double ended queue.
 *
 * @tparam T The type of the elements in the deque.
 * @tparam Capacity The capacity of the deque.
 * @tparam Traits The traits of the deque, to define the types used for the iterators, size, etc.
 */
template <typename T, usize Capacity, typename Traits = Container::ArrayTraits<T>>
    requires(Capacity > 0)
class StaticDeque
{
  public:
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = typename Traits::Iterator;
    using ConstIterator = typename Traits::ConstIterator;
    using Tools = Container::ArrayTools<Traits>;

    constexpr StaticDeque() = default;

    template <typename... Args>
    constexpr explicit StaticDeque(const SizeType p_Size, const Args &...p_Args) : m_Size(p_Size)
    {
        TKIT_ASSERT(p_Size <= Capacity, "[TOOLKIT][STAT-DEQUE] Size is bigger than capacity");
        if constexpr (sizeof...(Args) > 0 || !std::is_trivially_default_constructible_v<T>)
            Memory::ConstructRange(GetData(), GetData() + m_Size, p_Args...);

        m_Back = m_Size;
    }

    template <std::input_iterator It>
    constexpr StaticDeque(const It p_Begin, const It p_End)
        requires(std::is_copy_constructible_v<T>)
    {
        m_Size = static_cast<SizeType>(std::distance(p_Begin, p_End));
        TKIT_ASSERT(m_Size <= Capacity, "[TOOLKIT][STAT-DEQUE] Size is bigger than capacity");
        Tools::CopyConstructFromRange(GetData(), p_Begin, p_End);
        m_Back = m_Size;
    }

    constexpr StaticDeque(const std::initializer_list<ValueType> p_List)
        requires(std::is_copy_constructible_v<T>)
        : m_Size(static_cast<SizeType>(p_List.size()))
    {
        TKIT_ASSERT(p_List.size() <= Capacity, "[TOOLKIT][STAT-DEQUE] Size is bigger than capacity");
        Tools::CopyConstructFromRange(GetData(), p_List.begin(), p_List.end());
        m_Back = m_Size;
    }

    // This constructor WONT include the case M == Capacity (ie, copy constructor)
    template <SizeType M> constexpr StaticDeque(const StaticDeque<ValueType, M, Traits> &p_Other)
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT][STAT-DEQUE] Size is bigger than capacity");
        }
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    // This constructor WONT include the case M == Capacity (ie, move constructor)
    template <SizeType M> constexpr StaticDeque(StaticDeque<ValueType, M, Traits> &&p_Other)
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT][STAT-DEQUE] Size is bigger than capacity");
        }
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    constexpr StaticDeque(const StaticDeque &p_Other)
    {
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    constexpr StaticDeque(StaticDeque &&p_Other)
    {
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    ~StaticDeque()
    {
        Clear();
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <SizeType M> constexpr StaticDeque &operator=(const StaticDeque<ValueType, M, Traits> &p_Other)
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT][STAT-DEQUE] Size is bigger than capacity");
        }
        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }
    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <SizeType M> constexpr StaticDeque &operator=(StaticDeque<ValueType, M, Traits> &&p_Other)
    {
        if constexpr (M == Capacity)
        {
            if (this == &p_Other)
                return *this;
        }
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT][STAT-DEQUE] Size is bigger than capacity");
        }

        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }

    constexpr StaticDeque &operator=(const StaticDeque &p_Other)
    {
        if (this == &p_Other)
            return *this;

        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }

    constexpr StaticDeque &operator=(StaticDeque &&p_Other)
    {
        if (this == &p_Other)
            return *this;

        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }

    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &PushFront(Args &&...p_Args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][STAT-DEQUE] Container is already full");
        ValueType &val = *Memory::ConstructFromIterator(GetData() + m_Front, std::forward<Args>(p_Args)...);
        m_Front = PrevIndex(m_Front);
        ++m_Size;
        return val;
    }

    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &PushBack(Args &&...p_Args)
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT][STAT-DEQUE] Container is already full");
        ValueType &val = *Memory::ConstructFromIterator(GetData() + m_Back, std::forward<Args>(p_Args)...);
        m_Back = NextIndex(m_Back);
        ++m_Size;
        return val;
    }

    constexpr void PopFront()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][STAT-DEQUE] Container is already empty");
        m_Front = NextIndex(m_Front);
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(GetData() + m_Front);
        --m_Size;
    }
    constexpr void PopBack()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][STAT-DEQUE] Container is already empty");
        m_Back = PrevIndex(m_Back);
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(GetData() + m_Back);
        --m_Size;
    }

    constexpr void Clear()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            while (!IsEmpty())
                PopBack();
        else
        {
            m_Size = 0;
            m_Front = Capacity - 1;
            m_Back = 0;
        }
    }

    constexpr const ValueType *GetData() const
    {
        return reinterpret_cast<const ValueType *>(&m_Data[0]);
    }
    constexpr ValueType *GetData()
    {
        return reinterpret_cast<ValueType *>(&m_Data[0]);
    }

    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr const ValueType &At(const SizeType p_Index) const
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][STAT-DEQUE] Cannot index into an empty queue");
        TKIT_ASSERT(p_Index < Capacity, "[TOOLKIT][STAT-DEQUE] Index is out of bounds: {} >= {}", p_Index, Capacity);
        return *(GetData() + p_Index);
    }
    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr ValueType &At(const SizeType p_Index)
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][STAT-DEQUE] Cannot index into an empty queue");
        TKIT_ASSERT(p_Index < Capacity, "[TOOLKIT][STAT-DEQUE] Index is out of bounds: {} >= {}", p_Index, Capacity);
        return *(GetData() + p_Index);
    }

    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr const ValueType &operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }

    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr ValueType &operator[](const SizeType p_Index)
    {
        return At(p_Index);
    }

    constexpr SizeType GetFrontIndex() const
    {
        return NextIndex(m_Front);
    }
    constexpr SizeType GetBackIndex() const
    {
        return PrevIndex(m_Back);
    }
    constexpr SizeType GetFrontEnd() const
    {
        return m_Front;
    }
    constexpr SizeType GetBackEnd() const
    {
        return m_Back;
    }

    constexpr static SizeType NextIndex(const SizeType p_Index)
    {
        if (p_Index == Capacity - 1)
            return 0;
        return p_Index + 1;
    }

    constexpr static SizeType PrevIndex(const SizeType p_Index)
    {
        if (p_Index == 0)
            return Capacity - 1;
        return p_Index - 1;
    }

    constexpr const ValueType &GetFront() const
    {
        return At(GetFrontIndex());
    }
    constexpr const ValueType &GetBack() const
    {
        return At(GetBackIndex());
    }
    constexpr ValueType &GetFront()
    {
        return At(GetFrontIndex());
    }
    constexpr ValueType &GetBack()
    {
        return At(GetBackIndex());
    }

    constexpr SizeType GetSize() const
    {
        return m_Size;
    }

    constexpr SizeType GetCapacity() const
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
    struct alignas(ValueType) Element
    {
        std::byte Data[sizeof(ValueType)];
    };

    static_assert(sizeof(Element) == sizeof(ValueType), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(ValueType), "Element alignment is not equal to T alignment");

    FixedArray<Element, Capacity> m_Data{};
    SizeType m_Size = 0;
    SizeType m_Front = Capacity - 1;
    SizeType m_Back = 0;
};
template <typename T> using StaticDeque4 = StaticDeque<T, 4>;
template <typename T> using StaticDeque8 = StaticDeque<T, 8>;
template <typename T> using StaticDeque16 = StaticDeque<T, 16>;
template <typename T> using StaticDeque32 = StaticDeque<T, 32>;
template <typename T> using StaticDeque64 = StaticDeque<T, 64>;
template <typename T> using StaticDeque128 = StaticDeque<T, 128>;
template <typename T> using StaticDeque196 = StaticDeque<T, 196>;
template <typename T> using StaticDeque256 = StaticDeque<T, 256>;
template <typename T> using StaticDeque384 = StaticDeque<T, 384>;
template <typename T> using StaticDeque512 = StaticDeque<T, 512>;
template <typename T> using StaticDeque768 = StaticDeque<T, 768>;
template <typename T> using StaticDeque1024 = StaticDeque<T, 1024>;
} // namespace TKit
