#pragma once

#include "tkit/container/container.hpp"
#include <ostream>

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
    static constexpr ArrayType Type = AllocState::Type;
    static constexpr bool IsString = std::is_same_v<T, char>;

    using ValueType = T;
    using Tools = Container::ArrayTools<T>;

    constexpr Array() = default;
    template <typename... Args>
        requires(sizeof...(Args) > 1 || ((!std::is_same_v<Array, std::remove_cvref_t<Args>> &&
                                          !std::is_same_v<std::remove_cvref_t<char *>, std::remove_cvref_t<Args>>) &&
                                         ... && true))
    constexpr explicit Array(Args &&...args) : m_State(std::forward<Args>(args)...)
    {
    }

    template <std::convertible_to<usize> U, typename... Args>
    constexpr explicit Array(const U psize, Args &&...args) : m_State(std::forward<Args>(args)...)
    {
        const usize size = addOneIfString(usize(psize));
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(size != 0, size);
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
        writeNullTerminatorIfString();
    }

    template <std::convertible_to<usize> U, typename... Args>
    constexpr Array(const T *str, const U psize, Args &&...args)
        requires(IsString)
        : m_State(std::forward<Args>(args)...)
    {
        const usize size = addOneIfString(usize(psize));
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacity(size);
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
        Tools::CopyConstructFromRange(begin(), str, str + subOneIfString(size));
        writeNullTerminatorIfString();
    }
    template <typename... Args>
    constexpr Array(const T *str, Args &&...args)
        requires(IsString)
        : Array(str, usize(std::char_traits<T>::length(str)), std::forward<Args>(args)...)
    {
    }

    template <std::input_iterator It, typename... Args>
    constexpr Array(const It pbegin, const It pend, Args &&...args) : m_State(std::forward<Args>(args)...)
    {
        const usize size = addOneIfString(usize(std::distance(pbegin, pend)));
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(size != 0, size);
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
        writeNullTerminatorIfString();
    }

    template <typename... Args>
    constexpr Array(const std::initializer_list<T> list, Args &&...args) : m_State(std::forward<Args>(args)...)
    {
        const usize size = addOneIfString(usize(list.size()));
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(size != 0, size);
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
        writeNullTerminatorIfString();
    }

    constexpr Array(const Array &other)
    {
        const usize otherSize = other.m_State.Size;
        if constexpr (Type == Array_Dynamic)
            m_State.GrowCapacityIf(otherSize != 0, otherSize);
        else if constexpr (Type == Array_Arena || Type == Array_Stack || Type == Array_Tier)
        {
            m_State.Allocator = other.m_State.Allocator;
            if constexpr (Type == Array_Tier)
                m_State.GrowCapacityIf(otherSize != 0, otherSize);
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
        writeNullTerminatorIfString();
    }

    template <typename OtherAlloc> constexpr Array(const Array<T, OtherAlloc> &other)
    {
        const usize otherSize = other.m_State.Size;
        if constexpr (Type == Array_Dynamic)
            m_State.GrowCapacityIf(otherSize != 0, otherSize);
        else if constexpr (Type == Array_Arena || Type == Array_Stack || Type == Array_Tier)
        {
            if constexpr (Type == Array_Tier)
                m_State.GrowCapacityIf(otherSize != 0, otherSize);
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
        writeNullTerminatorIfString();
    }

    constexpr Array(Array &&other)
    {
        if constexpr (Type == Array_Static)
        {
            m_State.Size = other.m_State.Size;
            Tools::MoveConstructFromRange(begin(), other.begin(), other.end());
            writeNullTerminatorIfString();
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
        writeNullTerminatorIfString();
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
        writeNullTerminatorIfString();
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
            writeNullTerminatorIfString();
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
            writeNullTerminatorIfString();
            return *ConstructFromIterator(end() - 1, std::forward<Args>(args)...);
        }
        else
        {
            TKIT_ASSERT(!IsFull(), "[TOOLKIT][ARRAY] Container is already at capacity of {}", m_State.GetCapacity());
            ++m_State.Size;
            writeNullTerminatorIfString();
            return *ConstructFromIterator(end() - 1, std::forward<Args>(args)...);
        }
    }

    constexpr void Pop()
    {
        TKIT_ASSERT(!IsEmpty(), "[TOOLKIT][ARRAY] Cannot Pop(). Container is already empty");
        --m_State.Size;
        if constexpr (!std::is_trivially_destructible_v<T>)
            DestructFromIterator(end());
        writeNullTerminatorIfString();
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
                const usize pos = usize(std::distance(begin(), ppos));
                m_State.GrowCapacity(newSize);
                ppos = begin() + pos;
            }

            Tools::Insert(end(), ppos, std::forward<U>(value));
            m_State.Size = newSize;
            writeNullTerminatorIfString();
        }
        else
        {
            TKIT_ASSERT(!IsFull(), "[TOOLKIT][ARRAY] Container is already full");
            Tools::Insert(end(), ppos, std::forward<U>(value));
            ++m_State.Size;
            writeNullTerminatorIfString();
        }
    }

    template <std::input_iterator It> constexpr void Insert(T *ppos, const It pbegin, const It pend)
    {
        TKIT_ASSERT(ppos >= begin() && ppos <= end(), "[TOOLKIT][ARRAY] Iterator is out of bounds");
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            const usize newSize = m_State.Size + usize(std::distance(pbegin, pend));
            if (newSize > m_State.GetCapacity())
            {
                const usize pos = usize(std::distance(begin(), ppos));
                m_State.GrowCapacity(newSize);
                ppos = begin() + pos;
            }

            Tools::Insert(end(), ppos, pbegin, pend);
            m_State.Size = newSize;
            writeNullTerminatorIfString();
        }
        else
        {
            TKIT_ASSERT(std::distance(pbegin, pend) + m_State.Size <= m_State.GetCapacity(),
                        "[TOOLKIT][ARRAY] New size ({}) exceeds capacity of {}",
                        std::distance(pbegin, pend) + m_State.Size, m_State.GetCapacity());

            m_State.Size += Tools::Insert(end(), ppos, pbegin, pend);
            writeNullTerminatorIfString();
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
        writeNullTerminatorIfString();
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
        writeNullTerminatorIfString();
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
        writeNullTerminatorIfString();
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
        TKIT_CHECK_OUT_OF_BOUNDS(index, GetSize(), "[TOOLKIT][ARRAY] ");
        return *(begin() + index);
    }
    constexpr T &At(const usize index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, GetSize(), "[TOOLKIT][ARRAY] ");
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
        return At(GetSize() - 1);
    }
    constexpr T &GetBack()
    {
        return At(GetSize() - 1);
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
        const usize rsize = addOneIfString(size);
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
            m_State.GrowCapacityIf(rsize > m_State.GetCapacity(), rsize);
        else
        {
            if constexpr (Type != Array_Static)
            {
                if (!m_State.Data)
                    m_State.Allocate(rsize);
            }
            TKIT_ASSERT(rsize <= m_State.GetCapacity(), "[TOOLKIT][ARRAY] Size ({}) is bigger than capacity ({})",
                        rsize, m_State.GetCapacity());
        }

        // these two will never fire for strings so remain unhandled
        if constexpr (!std::is_trivially_destructible_v<T>)
            if (rsize < m_State.Size)
                DestructRange(begin() + rsize, end());

        if constexpr (sizeof...(Args) != 0 || !std::is_trivially_default_constructible_v<T>)
            if (rsize > m_State.Size)
                ConstructRange(begin() + m_State.Size, begin() + rsize, args...);
        //

        m_State.Size = rsize;
        writeNullTerminatorIfString();
    }

    constexpr void Reserve(const usize capacity)
        requires(Type != Array_Static)
    {
        const usize c = addOneIfString(capacity);
        if constexpr (Type == Array_Dynamic || Type == Array_Tier)
        {
            if (c > m_State.Capacity)
                m_State.ModifyCapacity(c);
        }
        else if (c != 0)
            m_State.Allocate(c);
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
        const usize csize = addOneIfString(size);
        m_State.Allocate(csize);
        m_State.Size = csize;
        if constexpr (sizeof...(Args) != 0 || !std::is_trivially_default_constructible_v<T>)
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

        const usize csize = addOneIfString(size);
        m_State.Allocator = allocator;
        m_State.Allocate(csize);
        m_State.Size = csize;
        if constexpr (sizeof...(Args) != 0 || !std::is_trivially_default_constructible_v<T>)
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

    constexpr auto GetAllocator() const
        requires(Type != Array_Static && Type != Array_Dynamic)
    {
        return m_State.Allocator;
    }

    constexpr const T *GetData() const
    {
        const T *ptr = rcast<const T *>(std::addressof(m_State.Data[0]));
        TKIT_ASSERT(ptr || m_State.Size == 0,
                    "[TOOLKIT][ARRAY] If the data of an array is null, its size must be zero, but is {}", m_State.Size);
        TKIT_ASSERT(ptr || m_State.GetCapacity() == 0,
                    "[TOOLKIT][ARRAY] If the data of an array is null, its capacity must be zero, but is {}",
                    m_State.GetCapacity());
        TKIT_ASSERT(!ptr || m_State.GetCapacity() != 0,
                    "[TOOLKIT][ARRAY] If the data of an array is not null, its capacity must be greater than 0");
        return ptr;
    }

    constexpr T *GetData()
    {
        T *ptr = rcast<T *>(std::addressof(m_State.Data[0]));
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
        return begin() + GetSize();
    }

    constexpr const T *begin() const
    {
        return GetData();
    }

    constexpr const T *end() const
    {
        return begin() + GetSize();
    }

    constexpr usize GetSize() const
    {
        return subOneIfString(m_State.Size);
    }
    constexpr usz GetBytes() const
    {
        return GetSize() * sizeof(T);
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

    template <typename... Args> static constexpr Array Format(const fmt::format_string<Args...> str, Args &&...args)
    {
        const usize fsize = usize(fmt::formatted_size(str, std::forward<Args>(args)...));
        Array fstr{fsize};
        fmt::format_to(fstr.begin(), str, std::forward<Args>(args)...);
        return fstr;
    }

    // these are ia generated. wasnt feeling like implementing string methods

    // === Querying ===

    constexpr bool StartsWith(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(str));
        return GetSize() >= len && std::char_traits<T>::compare(begin(), str, len) == 0;
    }

    constexpr bool StartsWith(const T ch) const
        requires(IsString)
    {
        return !IsEmpty() && *begin() == ch;
    }

    constexpr bool EndsWith(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(str));
        return GetSize() >= len && std::char_traits<T>::compare(end() - len, str, len) == 0;
    }

    constexpr bool EndsWith(const T ch) const
        requires(IsString)
    {
        return !IsEmpty() && *(end() - 1) == ch;
    }

    constexpr bool Contains(const T *str) const
        requires(IsString)
    {
        return Find(str) != npos;
    }

    constexpr bool Contains(const T ch) const
        requires(IsString)
    {
        return Find(ch) != npos;
    }

    // === Searching ===

    static constexpr usize npos = ~usize(0);

    constexpr usize Find(const T ch, const usize from = 0) const
        requires(IsString)
    {
        for (usize i = from; i < GetSize(); ++i)
            if (*(begin() + i) == ch)
                return i;
        return npos;
    }

    constexpr usize Find(const T *str, const usize from = 0) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(str));
        if (len == 0)
            return from <= GetSize() ? from : npos;
        if (len > GetSize())
            return npos;
        for (usize i = from; i <= GetSize() - len; ++i)
            if (std::char_traits<T>::compare(begin() + i, str, len) == 0)
                return i;
        return npos;
    }

    constexpr usize FindLast(const T ch) const
        requires(IsString)
    {
        for (usize i = GetSize(); i > 0; --i)
            if (*(begin() + i - 1) == ch)
                return i - 1;
        return npos;
    }

    constexpr usize FindLast(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(str));
        if (len == 0)
            return GetSize();
        if (len > GetSize())
            return npos;
        for (usize i = GetSize() - len + 1; i > 0; --i)
            if (std::char_traits<T>::compare(begin() + i - 1, str, len) == 0)
                return i - 1;
        return npos;
    }

    constexpr usize FindFirstOf(const T *chars, const usize from = 0) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(chars));
        for (usize i = from; i < GetSize(); ++i)
            for (usize j = 0; j < len; ++j)
                if (*(begin() + i) == chars[j])
                    return i;
        return npos;
    }

    constexpr usize FindLastOf(const T *chars) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(chars));
        for (usize i = GetSize(); i > 0; --i)
            for (usize j = 0; j < len; ++j)
                if (*(begin() + i - 1) == chars[j])
                    return i - 1;
        return npos;
    }

    constexpr usize FindFirstNotOf(const T *chars, const usize from = 0) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(chars));
        for (usize i = from; i < GetSize(); ++i)
        {
            bool found = false;
            for (usize j = 0; j < len; ++j)
                if (*(begin() + i) == chars[j])
                {
                    found = true;
                    break;
                }
            if (!found)
                return i;
        }
        return npos;
    }

    // === Substrings / Slicing ===

    constexpr Array SubString(const usize pos, const usize count = npos) const
        requires(IsString)
    {
        TKIT_ASSERT(pos <= GetSize(), "[TOOLKIT][ARRAY] SubString position out of bounds");
        const usize len = (count == npos || pos + count > GetSize()) ? GetSize() - pos : count;
        return createWithAllocator(begin() + pos, len);
    }

    // === Comparison ===

    constexpr i32 Compare(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(str));
        const usize minLen = GetSize() < len ? GetSize() : len;
        const i32 result = std::char_traits<T>::compare(begin(), str, minLen);
        if (result != 0)
            return result;
        if (GetSize() < len)
            return -1;
        if (GetSize() > len)
            return 1;
        return 0;
    }
    constexpr i32 Compare(const Array &other) const
        requires(IsString)
    {
        const usize minLen = GetSize() < other.GetSize() ? GetSize() : other.GetSize();
        const i32 result = std::char_traits<T>::compare(begin(), other.GetData(), minLen);
        if (result != 0)
            return result;
        if (GetSize() < other.GetSize())
            return -1;
        if (GetSize() > other.GetSize())
            return 1;
        return 0;
    }

    constexpr bool operator==(const T *str) const
        requires(IsString)
    {
        return Compare(str) == 0;
    }
    constexpr bool operator==(const Array &other) const
        requires(IsString)
    {
        return Compare(other) == 0;
    }
    constexpr bool operator!=(const T *str) const
        requires(IsString)
    {
        return Compare(str) != 0;
    }
    constexpr bool operator!=(const Array &other) const
        requires(IsString)
    {
        return Compare(other) != 0;
    }

    // === Trimming ===

    constexpr Array &TrimLeft(const T *chars = " \t\n\r")
        requires(IsString)
    {
        const usize pos = FindFirstNotOf(chars);
        if (pos == npos)
            Resize(0);
        else if (pos > 0)
            RemoveOrdered(begin(), begin() + pos);
        return *this;
    }

    constexpr Array TrimLeft(const T *chars = " \t\n\r") const
        requires(IsString)
    {
        Array copy = *this;
        return copy.TrimLeft(chars);
    }

    constexpr Array &TrimRight(const T *chars = " \t\n\r")
        requires(IsString)
    {
        usize i = GetSize();
        const usize len = usize(std::char_traits<T>::length(chars));
        while (i > 0)
        {
            bool isWhitespace = false;
            for (usize j = 0; j < len; ++j)
                if (*(begin() + i - 1) == chars[j])
                {
                    isWhitespace = true;
                    break;
                }
            if (!isWhitespace)
                break;
            --i;
        }
        Resize(i);
        return *this;
    }

    constexpr Array TrimRight(const T *chars = " \t\n\r") const
        requires(IsString)
    {
        Array copy = *this;
        return copy.TrimRight(chars);
    }

    constexpr Array &Trim(const T *chars = " \t\n\r")
        requires(IsString)
    {
        TrimRight(chars);
        TrimLeft(chars);
        return *this;
    }

    constexpr Array Trim(const T *chars = " \t\n\r") const
    {
        Array copy = *this;
        return copy.Trim(chars);
    }

    // === Concatenation ===

    constexpr Array &operator+=(const T *str)
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(str));
        Insert(end(), str, str + len);
        return *this;
    }

    constexpr Array &operator+=(const T ch)
        requires(IsString)
    {
        Append(ch);
        return *this;
    }

    constexpr Array &operator+=(const Array &arr)
    {
        Insert(end(), arr.begin(), arr.end());
        return *this;
    }

    // Array + Array
    friend constexpr Array operator+(const Array &lhs, const Array &rhs)
        requires(IsString)
    {
        const usize totalLen = lhs.GetSize() + rhs.GetSize();
        Array result = lhs.createWithAllocator(totalLen);
        std::char_traits<T>::copy(result.begin(), lhs.begin(), lhs.GetSize());
        std::char_traits<T>::copy(result.begin() + lhs.GetSize(), rhs.begin(), rhs.GetSize());
        return result;
    }

    // Array + const T*
    friend constexpr Array operator+(const Array &lhs, const T *rhs)
        requires(IsString)
    {
        const usize rhsLen = usize(std::char_traits<T>::length(rhs));
        const usize totalLen = lhs.GetSize() + rhsLen;
        Array result = lhs.createWithAllocator(totalLen);
        std::char_traits<T>::copy(result.begin(), lhs.begin(), lhs.GetSize());
        std::char_traits<T>::copy(result.begin() + lhs.GetSize(), rhs, rhsLen);
        return result;
    }

    // const T* + Array
    friend constexpr Array operator+(const T *lhs, const Array &rhs)
        requires(IsString)
    {
        const usize lhsLen = usize(std::char_traits<T>::length(lhs));
        const usize totalLen = lhsLen + rhs.GetSize();
        Array result = rhs.createWithAllocator(totalLen);
        std::char_traits<T>::copy(result.begin(), lhs, lhsLen);
        std::char_traits<T>::copy(result.begin() + lhsLen, rhs.begin(), rhs.GetSize());
        return result;
    }

    // === Replace ===

    // Replace all occurrences of `from` with `to`, in place
    constexpr Array &Replace(const T from, const T to)
        requires(IsString)
    {
        for (usize i = 0; i < GetSize(); ++i)
            if (*(begin() + i) == from)
                *(begin() + i) = to;
        return *this;
    }

    constexpr Array Replace(const T from, const T to) const
    {
        Array copy = *this;
        return copy.Replace(from, to);
    }

    // Replace all occurrences of `from` with `to` (potentially different lengths), returns new string
    constexpr Array Replace(const T *from, const T *to) const
        requires(IsString)
    {
        const usize fromLen = usize(std::char_traits<T>::length(from));
        const usize toLen = usize(std::char_traits<T>::length(to));

        if (fromLen == 0)
            return *this;

        // First pass: count occurrences
        usize count = 0;
        usize pos = Find(from, 0);
        while (pos != npos)
        {
            ++count;
            pos = Find(from, pos + fromLen);
        }

        if (count == 0)
            return *this;

        const usize newSize = GetSize() - count * fromLen + count * toLen;
        Array result = createWithAllocator(newSize);

        usize srcPos = 0;
        usize dstPos = 0;
        pos = Find(from, 0);
        while (pos != npos)
        {
            // Copy everything before the match
            const usize chunk = pos - srcPos;
            if (chunk > 0)
            {
                std::char_traits<T>::copy(result.begin() + dstPos, begin() + srcPos, chunk);
                dstPos += chunk;
            }
            // Copy the replacement
            std::char_traits<T>::copy(result.begin() + dstPos, to, toLen);
            dstPos += toLen;
            srcPos = pos + fromLen;
            pos = Find(from, srcPos);
        }

        // Copy the remainder
        const usize remaining = GetSize() - srcPos;
        if (remaining > 0)
            std::char_traits<T>::copy(result.begin() + dstPos, begin() + srcPos, remaining);

        return result;
    }

    // === Split ===

    constexpr Array<Array, typename AllocState::template Rebind<Array>> Split(const T delimiter) const
        requires(IsString)
    {
        Array<Array, typename AllocState::template Rebind<Array>> parts;
        if constexpr (Type != Array_Static && Type != Array_Dynamic)
            parts.ResetAllocator(m_State.Allocator);
        usize start = 0;

        for (usize i = 0; i < GetSize(); ++i)
        {
            if (*(begin() + i) == delimiter)
            {
                parts.Append(SubString(start, i - start));
                start = i + 1;
            }
        }
        parts.Append(SubString(start, GetSize() - start));
        return parts;
    }

    constexpr Array<Array, typename AllocState::template Rebind<Array>> Split(const T *delimiter) const
        requires(IsString)
    {
        const usize delimLen = usize(std::char_traits<T>::length(delimiter));
        Array<Array, typename AllocState::template Rebind<Array>> parts;
        if constexpr (Type != Array_Static && Type != Array_Dynamic)
            parts.ResetAllocator(m_State.Allocator);

        if (delimLen == 0)
        {
            parts.Append(*this);
            return parts;
        }

        usize start = 0;
        usize pos = Find(delimiter, 0);
        while (pos != npos)
        {
            parts.Append(SubString(start, pos - start));
            start = pos + delimLen;
            pos = Find(delimiter, start);
        }
        parts.Append(SubString(start, GetSize() - start));
        return parts;
    }

    // === Count ===

    constexpr usize Count(const T ch) const
        requires(IsString)
    {
        usize count = 0;
        for (usize i = 0; i < GetSize(); ++i)
            if (*(begin() + i) == ch)
                ++count;
        return count;
    }

    constexpr usize Count(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<T>::length(str));
        if (len == 0 || len > GetSize())
            return 0;
        usize count = 0;
        usize pos = Find(str, 0);
        while (pos != npos)
        {
            ++count;
            pos = Find(str, pos + len);
        }
        return count;
    }

    friend std::ostream &operator<<(std::ostream &os, const Array &s)
        requires(IsString)
    {
        return os.write(s.begin(), s.GetSize());
    }

  private:
    AllocState m_State{};

    void writeNullTerminatorIfString()
    {
        if constexpr (IsString)
            if (m_State.Size != 0)
                *end() = 0;
    }

    static constexpr usize addOneIfString(const usize size)
    {
        if constexpr (IsString)
            return size + 1;
        else
            return size;
    }
    static constexpr usize subOneIfString(const usize size)
    {
        if constexpr (IsString)
        {
            if (size != 0)
                return size - 1;
            return 0;
        }
        else
            return size;
    }

    template <typename... Args> constexpr Array createWithAllocator(Args &&...args) const
    {
        if constexpr (Type == Array_Static || Type == Array_Dynamic)
            return Array{std::forward<Args>(args)...};
        else
            return Array{std::forward<Args>(args)..., m_State.Allocator};
    }

    template <typename U, typename OtherAlloc> friend class Array;
};
} // namespace TKit

template <typename AllocState> struct fmt::formatter<TKit::Array<char, AllocState>> : fmt::formatter<fmt::string_view>
{
    auto format(const TKit::Array<char, AllocState> &s, fmt::format_context &ctx) const
    {
        return fmt::formatter<fmt::string_view>::format(fmt::string_view(s.begin(), s.GetSize()), ctx);
    }
};
