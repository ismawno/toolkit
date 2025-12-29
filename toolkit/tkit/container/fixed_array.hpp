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
template <typename T, usize Size, typename Traits = Container::ArrayTraits<T>> struct FixedArray
{
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;
    using Iterator = typename Traits::Iterator;
    using ConstIterator = typename Traits::ConstIterator;
    using Tools = Container::ArrayTools<Traits>;

    constexpr FixedArray() = default;

    constexpr FixedArray(const std::initializer_list<ValueType> p_Elements)
    {
        TKIT_ASSERT(p_Elements.size() <= Size, "[TOOLKIT][ARRAY] Size is bigger than capacity");
        Tools::CopyConstructFromRange(begin(), p_Elements.begin(), p_Elements.end());
    }

    template <usize OtherSize>
        requires(OtherSize == Size - 1)
    constexpr FixedArray(const FixedArray<T, OtherSize> &p_Other, const T &p_Value)
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
        TKIT_ASSERT(p_Index < Size, "[TOOLKIT][ARRAY] Index is out of bounds: {} >= {}", p_Index, Size);
        return Elements[p_Index];
    }
    constexpr ValueType &At(const SizeType p_Index)
    {
        TKIT_ASSERT(p_Index < Size, "[TOOLKIT][ARRAY] Index is out of bounds: {} >= {}", p_Index, Size);
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

template <typename T> using FixedArray4 = FixedArray<T, 4>;
template <typename T> using FixedArray8 = FixedArray<T, 8>;
template <typename T> using FixedArray16 = FixedArray<T, 16>;
template <typename T> using FixedArray32 = FixedArray<T, 32>;
template <typename T> using FixedArray64 = FixedArray<T, 64>;
template <typename T> using FixedArray128 = FixedArray<T, 128>;
template <typename T> using FixedArray196 = FixedArray<T, 196>;
template <typename T> using FixedArray256 = FixedArray<T, 256>;
template <typename T> using FixedArray384 = FixedArray<T, 384>;
template <typename T> using FixedArray512 = FixedArray<T, 512>;
template <typename T> using FixedArray768 = FixedArray<T, 768>;
template <typename T> using FixedArray1024 = FixedArray<T, 1024>;

#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP

} // namespace TKit
