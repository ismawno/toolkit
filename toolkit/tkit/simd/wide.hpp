#pragma once

#include "tkit/container/array.hpp"
#include <algorithm>

namespace TKit
{
namespace Detail
{
template <typename T>
concept Float = std::same_as<T, f32> || std::same_as<T, f64>;

template <typename T>
concept UnsignedInteger = std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32> || std::same_as<T, u64>;
template <typename T>
concept SignedInteger = std::same_as<T, i8> || std::same_as<T, i16> || std::same_as<T, i32> || std::same_as<T, i64>;

template <typename T>
concept Integer = UnsignedInteger<T> || SignedInteger<T>;

} // namespace Detail
template <typename T, usize L = TKIT_SIMD_SIZE / sizeof(T), typename Traits = Container::ArrayTraits<T>> class Wide
{
    static_assert(L > 0, "[TOOLKIT][SIMD] The amount of lanes must be greater than zero");

  public:
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = L;
    static constexpr SizeType Alignment = alignof(T);

    using Mask = u<Lanes>;

    constexpr Wide() = default;
    constexpr Wide(const T *p_Data)
    {
        Memory::ForwardCopy(m_Data.begin(), p_Data, p_Data + Lanes);
    }
    template <typename Callable>
        requires std::invocable<Callable, SizeType>
    constexpr Wide(Callable &&p_Callable)
    {
        for (SizeType i = 0; i < Lanes; ++i)
            m_Data[i] = static_cast<f32>(p_Callable(i));
    }
    constexpr Wide(const T p_Data)
    {
        for (SizeType i = 0; i < Lanes; ++i)
            m_Data[i] = p_Data;
    }

    constexpr ValueType At(const SizeType p_Index) const
    {
        return m_Data[p_Index];
    }
    constexpr ValueType operator[](const SizeType p_Index) const
    {
        return m_Data[p_Index];
    }

    constexpr void Store(T *p_Data) const
    {
        Memory::ForwardCopy(p_Data, m_Data.begin(), m_Data.end());
    }

    friend constexpr Wide Min(const Wide &p_Left, const Wide &p_Right)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = std::min(p_Left.m_Data[i], p_Right.m_Data[i]);
        return wide;
    }
    friend constexpr Wide Max(const Wide &p_Left, const Wide &p_Right)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = std::max(p_Left.m_Data[i], p_Right.m_Data[i]);
        return wide;
    }

    friend constexpr Wide Select(const Mask p_Mask, const Wide &p_Left, const Wide &p_Right)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = (p_Mask & (Mask{1} << i)) ? p_Left.m_Data[i] : p_Right.m_Data[i];
        return wide;
    }

    friend constexpr T ReduceAdd(const Wide &p_Wide)
    {
        T val{};
        for (SizeType i = 0; i < Lanes; ++i)
            val += p_Wide[i];
        return val;
    }

    friend constexpr Wide operator+(const Wide &p_Left, const Wide &p_Right)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = p_Left.m_Data[i] + p_Right.m_Data[i];
        return wide;
    }
    friend constexpr Wide operator-(const Wide &p_Left, const Wide &p_Right)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = p_Left.m_Data[i] - p_Right.m_Data[i];
        return wide;
    }
    friend constexpr Wide operator*(const Wide &p_Left, const Wide &p_Right)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = p_Left.m_Data[i] * p_Right.m_Data[i];
        return wide;
    }
    friend constexpr Wide operator/(const Wide &p_Left, const Wide &p_Right)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = p_Left.m_Data[i] / p_Right.m_Data[i];
        return wide;
    }
    friend constexpr Wide operator-(const Wide &p_Other)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = -p_Other.m_Data[i];
        return wide;
    }

#define CREATE_CMP_OP(p_Op)                                                                                            \
    friend constexpr Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                       \
    {                                                                                                                  \
        Mask m = 0;                                                                                                    \
        for (SizeType i = 0; i < Lanes; ++i)                                                                           \
        {                                                                                                              \
            const Mask mustSet = static_cast<Mask>(p_Left.m_Data[i] p_Op p_Right.m_Data[i]);                           \
            m |= mustSet << i;                                                                                         \
        }                                                                                                              \
        return m;                                                                                                      \
    }

    CREATE_CMP_OP(==)
    CREATE_CMP_OP(!=)
    CREATE_CMP_OP(<)
    CREATE_CMP_OP(>)
    CREATE_CMP_OP(<=)
    CREATE_CMP_OP(>=)

#undef CREATE_CMP_OP

  private:
    Array<T, Lanes, Traits> m_Data;
}; // namespace TKit
} // namespace TKit
