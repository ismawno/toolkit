#pragma once

#include "tkit/container/container.hpp"
#include "tkit/container/array.hpp"
#include "tkit/utils/logging.hpp"

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

    constexpr StaticDeque() noexcept = default;

    // This constructor WONT include the case M == Capacity (ie, copy constructor)
    template <SizeType M>
    explicit(false) constexpr StaticDeque(const StaticDeque<ValueType, M, Traits> &p_Other) noexcept
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    // This constructor WONT include the case M == Capacity (ie, move constructor)
    template <SizeType M> explicit(false) constexpr StaticDeque(StaticDeque<ValueType, M, Traits> &&p_Other) noexcept
    {
        if constexpr (M > Capacity)
        {
            TKIT_ASSERT(p_Other.GetSize() <= Capacity, "[TOOLKIT] Size is bigger than capacity");
        }
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    constexpr StaticDeque(const StaticDeque &p_Other) noexcept
    {
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    constexpr StaticDeque(StaticDeque &&p_Other) noexcept
    {
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
    }

    ~StaticDeque() noexcept
    {
        Clear();
    }

    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <SizeType M> constexpr StaticDeque &operator=(const StaticDeque<ValueType, M, Traits> &p_Other) noexcept
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
        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }
    // Same goes for assignment. It wont include `M == Capacity`, and use the default assignment operator
    template <SizeType M> constexpr StaticDeque &operator=(StaticDeque<ValueType, M, Traits> &&p_Other) noexcept
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

        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }

    constexpr StaticDeque &operator=(const StaticDeque &p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }

    constexpr StaticDeque &operator=(StaticDeque &&p_Other) noexcept
    {
        if (this == &p_Other)
            return *this;

        Clear();
        for (u32 i = p_Other.GetFrontIndex(), j = 0; j < p_Other.GetSize(); i = NextIndex(i), ++j)
            PushBack(p_Other.At(i));
        return *this;
    }

    /**
     * @brief Insert a new element at the beginning of the queue.
     *
     * The element is constructed in place using the provided arguments.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &PushFront(Args &&...p_Args) noexcept
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT] Container is already full");
        ValueType &val = *Memory::ConstructFromIterator(GetData() + m_Front, std::forward<Args>(p_Args)...);
        m_Front = PrevIndex(m_Front);
        ++m_Size;
        return val;
    }

    /**
     * @brief Insert a new element at the end of the queue.
     *
     * The element is constructed in place using the provided arguments.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    template <typename... Args>
        requires std::constructible_from<ValueType, Args...>
    constexpr ValueType &PushBack(Args &&...p_Args) noexcept
    {
        TKIT_ASSERT(!IsFull(), "[TOOLKIT] Container is already full");
        ValueType &val = *Memory::ConstructFromIterator(GetData() + m_Back, std::forward<Args>(p_Args)...);
        m_Back = NextIndex(m_Back);
        ++m_Size;
        return val;
    }

    /**
     * @brief Erase the element at the beginning of the queue.
     *
     * The destructor will only be called if not trivially destructible.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    constexpr void PopFront() noexcept
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT] Container is already empty");
        m_Front = NextIndex(m_Front);
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(GetData() + m_Front);
        --m_Size;
    }
    /**
     * @brief Erase the element at the end of the queue.
     *
     * The destructor will only be called if not trivially destructible.
     *
     * @param p_Args The arguments to pass to the constructor of `T`.
     * @return A reference to the newly constructed element.
     */
    constexpr void PopBack() noexcept
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT] Container is already empty");
        m_Back = PrevIndex(m_Back);
        if constexpr (!std::is_trivially_destructible_v<T>)
            Memory::DestructFromIterator(GetData() + m_Back);
        --m_Size;
    }

    /**
     * @brief Clear the deque and set its size to 0.
     *
     * The elements are destroyed if not trivially destructible. The memory is not deallocated.
     *
     */
    constexpr void Clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            while (!IsEmpty())
                PopBack();
        else
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

    /**
     * @brief Access the i-th element of the circular buffer, at your own risk.
     *
     * This data-structure uses a circular buffer and not all allocated entries are guaranteed to be valid. Make sure
     * that you are using this accessors bounded with the index method of this class.
     *
     * @param p_Index The index of the element.
     * @return A reference to the element.
     */
    constexpr const ValueType &At(const SizeType p_Index) const noexcept
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT] Cannot index into an empty queue");
        TKIT_ASSERT(p_Index < Capacity, "[TOOLKIT] Index is out of bounds");
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
    constexpr ValueType &At(const SizeType p_Index) noexcept
    {
        TKIT_ASSERT(p_Index < Capacity, "[TOOLKIT] Index is out of bounds");
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
    constexpr const ValueType &operator[](const SizeType p_Index) const noexcept
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
    constexpr ValueType &operator[](const SizeType p_Index) noexcept
    {
        return At(p_Index);
    }

    /**
     * @brief Get the index of the front element.
     *
     * Useful as an iteration starting point.
     *
     */
    constexpr SizeType GetFrontIndex() const noexcept
    {
        return NextIndex(m_Front);
    }
    /**
     * @brief Get the index of the back element.
     *
     * Useful as a reverse iteration starting point.
     *
     */
    constexpr SizeType GetBackIndex() const noexcept
    {
        return PrevIndex(m_Back);
    }
    /**
     * @brief Get the bound located at the front of the queue.
     *
     * Useful as a reverse iteration stop condition.
     *
     */
    constexpr SizeType GetFrontEnd() const noexcept
    {
        return m_Front;
    }
    /**
     * @brief Get the bound located at the back of the queue.
     *
     * Useful as an iteration stop condition.
     *
     */
    constexpr SizeType GetBackEnd() const noexcept
    {
        return m_Back;
    }

    /**
     * @brief Get the next buffer index, cycling if necessary.
     *
     * Useful when iterating.
     *
     * @return The next index.
     */
    constexpr static SizeType NextIndex(const SizeType p_Index) noexcept
    {
        if (p_Index == Capacity - 1)
            return 0;
        return p_Index + 1;
    }

    /**
     * @brief Get the previous buffer index, cycling if necessary.
     *
     * Useful when iterating.
     *
     * @return The previous index.
     */
    constexpr static SizeType PrevIndex(const SizeType p_Index) noexcept
    {
        if (p_Index == 0)
            return Capacity - 1;
        return p_Index - 1;
    }

    constexpr const ValueType &GetFront() const noexcept
    {
        return At(GetFrontIndex());
    }
    constexpr const ValueType &GetBack() const noexcept
    {
        return At(GetBackIndex());
    }
    constexpr ValueType &GetFront() noexcept
    {
        return At(GetFrontIndex());
    }
    constexpr ValueType &GetBack() noexcept
    {
        return At(GetBackIndex());
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
    constexpr Iterator beginContiguous() noexcept
    {
        return GetData() + m_Front + 1;
    }
    constexpr Iterator endContiguous() noexcept
    {
        return GetData() + m_Back;
    }

    constexpr Iterator beginFront() noexcept
    {
        return beginContiguous();
    }
    constexpr Iterator beginBack() noexcept
    {
        return GetData();
    }

    constexpr Iterator endFront() noexcept
    {
        return GetData() + Capacity;
    }
    constexpr Iterator endBack() noexcept
    {
        return endContiguous();
    }

    struct alignas(ValueType) Element
    {
        std::byte Data[sizeof(ValueType)];
    };

    static_assert(sizeof(Element) == sizeof(ValueType), "Element size is not equal to T size");
    static_assert(alignof(Element) == alignof(ValueType), "Element alignment is not equal to T alignment");

    Array<Element, Capacity> m_Data{};
    SizeType m_Size = 0;
    SizeType m_Front = Capacity - 1;
    SizeType m_Back = 0;
};
} // namespace TKit
