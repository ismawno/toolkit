#pragma once

#include "tkit/container/fixed_array.hpp"
#include "tkit/container/array.hpp"
#include "tkit/container/array.hpp"
#include <span>
#include <vector>
#include <array>

namespace TKit
{
/**
 * @brief A view with a dynamic extent over a contiguous sequence of objects.
 *
 * It is meant to be used as a drop-in replacement for `std::span`, especially for personal use and to be able to
 * have a bit more control.
 *
 * @tparam T The type of the elements in the span.
 */
template <typename T> class Span
{
  public:
    using ValueType = T;
    using ElementType = std::remove_cvref_t<T>;
    static constexpr bool IsString = std::is_same_v<ElementType, char>;

    constexpr Span() : m_Data(nullptr), m_Size(0)
    {
    }
    constexpr Span(T &data) : Span(std::addressof(data), 1)
    {
    }
    constexpr Span(T &&data) = delete;

    constexpr Span(T *data, const usize size) : m_Data(data), m_Size(size)
    {
    }
    constexpr Span(T *data)
        requires(IsString)
        : m_Data(data), m_Size(usize(std::char_traits<ElementType>::length(data)))
    {
    }

    template <usize Extent>
    constexpr Span(const FixedArray<ElementType, Extent> &array) : m_Data(array.GetData()), m_Size(Extent)
    {
    }
    template <usize Extent>
    constexpr Span(FixedArray<ElementType, Extent> &array) : m_Data(array.GetData()), m_Size(Extent)
    {
    }

    template <typename AllocState>
    constexpr Span(const Array<ElementType, AllocState> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }
    template <typename AllocState>
    constexpr Span(Array<ElementType, AllocState> &array) : m_Data(array.GetData()), m_Size(array.GetSize())
    {
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const Span<ElementType> &other) : m_Data(other.GetData()), m_Size(other.GetSize())
    {
    }

    template <typename ElementType>
        requires(std::convertible_to<ElementType *, T *> &&
                 std::same_as<std::remove_cvref_t<ElementType>, std::remove_cvref_t<T>>)
    constexpr Span(const std::span<ElementType> &other) : m_Data(other.data()), m_Size(usize(other.size()))
    {
    }

    constexpr Span(const std::vector<ElementType> &array) : m_Data(array.data()), m_Size(usize(array.size()))
    {
    }
    template <usize Extent>
    constexpr Span(const std::array<ElementType, Extent> &array) : m_Data(array.data()), m_Size(Extent)
    {
    }
    constexpr Span(const std::string_view view)
        requires(IsString)
        : m_Data(view.data()), m_Size(usize(view.size()))
    {
    }

    constexpr T *GetData() const
    {
        return m_Data;
    }
    constexpr usize GetSize() const
    {
        return m_Size;
    }
    constexpr usz GetBytes() const
    {
        return GetSize() * sizeof(T);
    }

    constexpr T &operator[](const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, GetSize(), "[TOOLKIT][SPAN] ");
        return m_Data[index];
    }
    constexpr T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, GetSize(), "[TOOLKIT][SPAN] ");
        return m_Data[index];
    }

    constexpr T &GetFront() const
    {
        return At(0);
    }
    constexpr T &GetBack() const
    {
        return At(m_Size - 1);
    }

    constexpr T *begin() const
    {
        return m_Data;
    }
    constexpr T *end() const
    {
        return m_Data + GetSize();
    }

    constexpr bool IsEmpty() const
    {
        return m_Size == 0;
    }

    constexpr operator bool() const
    {
        return m_Data != nullptr;
    }

    // these are ia generated. wasnt feeling like implementing string methods

    // === Querying ===

    constexpr bool StartsWith(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<ElementType>::length(str));
        return GetSize() >= len && std::char_traits<ElementType>::compare(begin(), str, len) == 0;
    }

    constexpr bool StartsWith(const T ch) const
        requires(IsString)
    {
        return !IsEmpty() && *begin() == ch;
    }

    constexpr bool EndsWith(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<ElementType>::length(str));
        return GetSize() >= len && std::char_traits<ElementType>::compare(end() - len, str, len) == 0;
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
        const usize len = usize(std::char_traits<ElementType>::length(str));
        if (len == 0)
            return from <= GetSize() ? from : npos;
        if (len > GetSize())
            return npos;
        for (usize i = from; i <= GetSize() - len; ++i)
            if (std::char_traits<ElementType>::compare(begin() + i, str, len) == 0)
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
        const usize len = usize(std::char_traits<ElementType>::length(str));
        if (len == 0)
            return GetSize();
        if (len > GetSize())
            return npos;
        for (usize i = GetSize() - len + 1; i > 0; --i)
            if (std::char_traits<ElementType>::compare(begin() + i - 1, str, len) == 0)
                return i - 1;
        return npos;
    }

    constexpr usize FindFirstOf(const T *chars, const usize from = 0) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<ElementType>::length(chars));
        for (usize i = from; i < GetSize(); ++i)
            for (usize j = 0; j < len; ++j)
                if (*(begin() + i) == chars[j])
                    return i;
        return npos;
    }

    constexpr usize FindLastOf(const T *chars) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<ElementType>::length(chars));
        for (usize i = GetSize(); i > 0; --i)
            for (usize j = 0; j < len; ++j)
                if (*(begin() + i - 1) == chars[j])
                    return i - 1;
        return npos;
    }

    constexpr usize FindFirstNotOf(const T *chars, const usize from = 0) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<ElementType>::length(chars));
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

    constexpr Span SubString(const usize pos, const usize count = npos) const
        requires(IsString)
    {
        TKIT_ASSERT(pos <= GetSize(), "[TOOLKIT][ARRAY] SubString position out of bounds");
        const usize len = (count == npos || pos + count > GetSize()) ? GetSize() - pos : count;
        return Span(begin() + pos, len);
    }

    // === Comparison ===

    constexpr i32 Compare(const Span<const ElementType> &other) const
        requires(IsString)
    {
        const usize minLen = GetSize() < other.GetSize() ? GetSize() : other.GetSize();
        const i32 result = std::char_traits<ElementType>::compare(begin(), other.begin(), minLen);
        if (result != 0)
            return result;
        if (GetSize() < other.GetSize())
            return -1;
        if (GetSize() > other.GetSize())
            return 1;
        return 0;
    }

    constexpr bool operator==(const Span<const ElementType> &other) const
        requires(IsString)
    {
        return Compare(other) == 0;
    }

    constexpr bool operator!=(const Span<const ElementType> &other) const
        requires(IsString)
    {
        return Compare(other) != 0;
    }

    constexpr i32 Compare(const T *str) const
        requires(IsString)
    {
        const usize len = usize(std::char_traits<ElementType>::length(str));
        const usize minLen = GetSize() < len ? GetSize() : len;
        const i32 result = std::char_traits<ElementType>::compare(begin(), str, minLen);
        if (result != 0)
            return result;
        if (GetSize() < len)
            return -1;
        if (GetSize() > len)
            return 1;
        return 0;
    }

    constexpr bool operator==(const T *str) const
        requires(IsString)
    {
        return Compare(str) == 0;
    }
    constexpr bool operator!=(const T *str) const
        requires(IsString)
    {
        return Compare(str) != 0;
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
        const usize len = usize(std::char_traits<ElementType>::length(str));
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

    // === Trimming ===

    constexpr Span &TrimLeft(const T *chars = " \t\n\r")
        requires(IsString)
    {
        const usize pos = FindFirstNotOf(chars);
        if (pos == npos)
        {
            m_Data += GetSize();
            m_Size = 0;
        }
        else if (pos > 0)
        {
            m_Data += pos;
            m_Size -= pos;
        }
        return *this;
    }

    constexpr Span TrimLeft(const T *chars = " \t\n\r") const
        requires(IsString)
    {
        Span copy = *this;
        return copy.TrimLeft(chars);
    }

    constexpr Span &TrimRight(const T *chars = " \t\n\r")
        requires(IsString)
    {
        const usize len = usize(std::char_traits<ElementType>::length(chars));
        usize i = GetSize();
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
        m_Size = i;
        return *this;
    }

    constexpr Span TrimRight(const T *chars = " \t\n\r") const
        requires(IsString)
    {
        Span copy = *this;
        return copy.TrimRight(chars);
    }

    constexpr Span &Trim(const T *chars = " \t\n\r")
        requires(IsString)
    {
        TrimRight(chars);
        TrimLeft(chars);
        return *this;
    }

    constexpr Span Trim(const T *chars = " \t\n\r") const
        requires(IsString)
    {
        Span copy = *this;
        return copy.Trim(chars);
    }

    // === Replace (mutable spans only) ===

    constexpr Span &Replace(const T from, const T to)
        requires(IsString && !std::is_const_v<T>)
    {
        for (usize i = 0; i < GetSize(); ++i)
            if (*(begin() + i) == from)
                *(begin() + i) = to;
        return *this;
    }

    // === Friend operators: Span <-> Array ===

    template <typename AllocState>
    friend constexpr bool operator==(const Span &lhs, const Array<ElementType, AllocState> &rhs)
        requires(IsString)
    {
        return lhs == Span<const ElementType>(rhs);
    }

    template <typename AllocState>
    friend constexpr bool operator==(const Array<ElementType, AllocState> &lhs, const Span &rhs)
        requires(IsString)
    {
        return Span<const ElementType>(lhs) == rhs;
    }

    template <typename AllocState>
    friend constexpr bool operator!=(const Span &lhs, const Array<ElementType, AllocState> &rhs)
        requires(IsString)
    {
        return !(lhs == rhs);
    }

    template <typename AllocState>
    friend constexpr bool operator!=(const Array<ElementType, AllocState> &lhs, const Span &rhs)
        requires(IsString)
    {
        return !(lhs == rhs);
    }

    constexpr bool operator<(const Span<const ElementType> &other) const
        requires(IsString)
    {
        return Compare(other.begin()) < 0;
    }

    constexpr bool operator>(const Span<const ElementType> &other) const
        requires(IsString)
    {
        return Compare(other.begin()) > 0;
    }

    constexpr bool operator<=(const Span<const ElementType> &other) const
        requires(IsString)
    {
        return Compare(other.begin()) <= 0;
    }

    constexpr bool operator>=(const Span<const ElementType> &other) const
        requires(IsString)
    {
        return Compare(other.begin()) >= 0;
    }

    // === Friend concatenation: Span + Array / Array + Span ===

    template <typename AllocState>
    friend constexpr Array<ElementType, AllocState> operator+(const Array<ElementType, AllocState> &lhs,
                                                              const Span &rhs)
        requires(IsString)
    {
        const usize totalLen = lhs.GetSize() + rhs.GetSize();
        Array<ElementType, AllocState> result{totalLen};
        std::char_traits<ElementType>::copy(result.begin(), lhs.begin(), lhs.GetSize());
        std::char_traits<ElementType>::copy(result.begin() + lhs.GetSize(), rhs.begin(), rhs.GetSize());
        return result;
    }

    template <typename AllocState>
    friend constexpr Array<ElementType, AllocState> operator+(const Span &lhs,
                                                              const Array<ElementType, AllocState> &rhs)
        requires(IsString)
    {
        const usize totalLen = lhs.GetSize() + rhs.GetSize();
        Array<ElementType, AllocState> result{totalLen};
        std::char_traits<ElementType>::copy(result.begin(), lhs.begin(), lhs.GetSize());
        std::char_traits<ElementType>::copy(result.begin() + lhs.GetSize(), rhs.begin(), rhs.GetSize());
        return result;
    }

    friend std::ostream &operator<<(std::ostream &os, const Span &s)
        requires(IsString)
    {
        return os.write(s.begin(), s.GetSize());
    }
    operator std::span<T>() const
    {
        return std::span<T>{m_Data, m_Size};
    }
    operator std::string_view()
        requires(IsString)
    {
        return std::string_view{m_Data, m_Size};
    }

  private:
    T *m_Data;
    usize m_Size;
};

using StringView = Span<const char>;

} // namespace TKit

template <> struct fmt::formatter<TKit::Span<const char>> : fmt::formatter<fmt::string_view>
{
    auto format(const TKit::Span<const char> &s, fmt::format_context &ctx) const
    {
        return fmt::formatter<fmt::string_view>::format(fmt::string_view(s.begin(), s.GetSize()), ctx);
    }
};
template <> struct std::hash<TKit::Span<const char>>
{
    std::size_t operator()(const TKit::Span<const char> &s) const noexcept
    {
        return std::hash<std::string_view>{}(std::string_view(s.begin(), s.GetSize()));
    }
};
