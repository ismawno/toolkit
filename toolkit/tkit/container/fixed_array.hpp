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
 * @tparam Capacity The number of elements in the array.
 */
template <typename T, usize Capacity> struct FixedArray
{
    using ValueType = T;
    using Tools = Container::ArrayTools<T>;

    constexpr FixedArray() = default;

    constexpr FixedArray(const std::initializer_list<T> p_Elements)
    {
        TKIT_ASSERT(p_Elements.size() <= Capacity, "[TOOLKIT][FIXED-ARRAY] Size ({}) is bigger than capacity ({})",
                    p_Elements.size(), Capacity);
        Tools::CopyConstructFromRange(begin(), p_Elements.begin(), p_Elements.end());
    }

    template <usize OtherSize>
        requires(OtherSize == Capacity - 1)
    constexpr FixedArray(const FixedArray<T, OtherSize> &p_Other, const T &p_Value)
    {
        Tools::CopyConstructFromRange(begin(), p_Other.begin(), p_Other.end());
        Elements[Capacity - 1] = p_Value;
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
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, Capacity, "[TOOLKIT][ARRAY] ");
        return Elements[p_Index];
    }
    constexpr T &At(const usize p_Index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(p_Index, Capacity, "[TOOLKIT][ARRAY] ");
        return Elements[p_Index];
    }

    constexpr usize GetSize() const
    {
        return Capacity;
    }

    constexpr const T *GetData() const
    {
        return Elements;
    }
    constexpr T *GetData()
    {
        return Elements;
    }

    constexpr T *begin()
    {
        return Elements;
    }
    constexpr T *end()
    {
        return Elements + Capacity;
    }

    constexpr const T *begin() const
    {
        return Elements;
    }
    constexpr const T *end() const
    {
        return Elements + Capacity;
    }

    T Elements[Capacity];
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
