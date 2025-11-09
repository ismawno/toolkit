#pragma once

#include "tkit/container/container.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit
{
/**
 * @brief A plain C array wrapper.
 *
 * Can be used as a drop-in replacement for `std::array`, accounting for method name differences. It is here to
 * provide a bit more control.
 *
 * @tparam T The type of the elements.
 * @tparam Size The number of elements in the array.
 */
template <typename T, usize Size, typename Traits = Container::ArrayTraits<T>> struct Array
{
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = typename Traits::Iterator;
    using ConstIterator = typename Traits::ConstIterator;
    using Tools = Container::ArrayTools<Traits>;

    constexpr Array() = default;

    constexpr Array(const std::initializer_list<ValueType> p_Elements)
    {
        TKIT_ASSERT(p_Elements.size() <= Size, "[TOOLKIT][ARRAY] Size is bigger than capacity");
        Tools::CopyConstructFromRange(begin(), p_Elements.begin(), p_Elements.end());
    }

    template <usize OtherSize>
        requires(OtherSize == Size - 1)
    constexpr Array(const Array<T, OtherSize> &p_Other, const T &p_Value)
    {
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
        Elements[Size - 1] = p_Value;
    }

    constexpr const ValueType &operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }
    constexpr ValueType &operator[](const SizeType p_Index)
    {
        return At(p_Index);
    }

    constexpr const ValueType &At(const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < Size, "[TOOLKIT][ARRAY] Index is out of bounds");
        return Elements[p_Index];
    }
    constexpr ValueType &At(const SizeType p_Index)
    {
        TKIT_ASSERT(p_Index < Size, "[TOOLKIT][ARRAY] Index is out of bounds");
        return Elements[p_Index];
    }

    constexpr SizeType GetSize() const
    {
        return Size;
    }

    constexpr const ValueType *GetData() const
    {
        return Elements;
    }
    constexpr ValueType *GetData()
    {
        return Elements;
    }

    constexpr Iterator begin()
    {
        return Elements;
    }
    constexpr Iterator end()
    {
        return Elements + Size;
    }

    constexpr ConstIterator begin() const
    {
        return Elements;
    }
    constexpr ConstIterator end() const
    {
        return Elements + Size;
    }

    ValueType Elements[Size];
};

template <typename T> using Array4 = Array<T, 4>;
template <typename T> using Array8 = Array<T, 8>;
template <typename T> using Array16 = Array<T, 16>;
template <typename T> using Array32 = Array<T, 32>;
template <typename T> using Array64 = Array<T, 64>;
template <typename T> using Array128 = Array<T, 128>;
template <typename T> using Array196 = Array<T, 196>;
template <typename T> using Array256 = Array<T, 256>;
template <typename T> using Array384 = Array<T, 384>;
template <typename T> using Array512 = Array<T, 512>;
template <typename T> using Array768 = Array<T, 768>;
template <typename T> using Array1024 = Array<T, 1024>;

#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP

} // namespace TKit
