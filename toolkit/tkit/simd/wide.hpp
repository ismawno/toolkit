#pragma once

#include "tkit/container/array.hpp"
#include "tkit/simd/utils.hpp"
#include <algorithm>

namespace TKit::Detail
{
template <typename T, usize L, typename Traits = Container::ArrayTraits<T>>
    requires(L > 0 && (Float<T> || Integer<T>))
class Wide
{
  public:
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = L;
    static constexpr SizeType Alignment = alignof(T);

    using Mask = u<MaskSize<Lanes>()>;
    using BitMask = Mask;

    static constexpr BitMask PackMask(const Mask &p_Mask)
    {
        return p_Mask;
    }
    static constexpr BitMask WidenMask(const BitMask &p_Mask)
    {
        return p_Mask;
    }

    constexpr Wide() = default;
    constexpr Wide(const T *p_Data)
    {
        Memory::ForwardCopy(m_Data.begin(), p_Data, p_Data + Lanes);
    }
    constexpr Wide(const T *p_Data, const SizeType p_Stride)
    {
        const std::byte *data = reinterpret_cast<const std::byte *>(p_Data);
        for (SizeType i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(&m_Data[i], data + i * p_Stride, sizeof(T));
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

    static constexpr Wide LoadAligned(const T *p_Data)
    {
        return Wide{p_Data};
    }
    static constexpr Wide LoadUnaligned(const T *p_Data)
    {
        return Wide{p_Data};
    }
    static constexpr Wide Gather(const T *p_Data, const SizeType p_Stride)
    {
        return Wide{p_Data, p_Stride};
    }

    constexpr void StoreAligned(T *p_Data) const
    {
        Memory::ForwardCopy(p_Data, m_Data.begin(), m_Data.end());
    }
    constexpr void StoreUnaligned(T *p_Data) const
    {
        Memory::ForwardCopy(p_Data, m_Data.begin(), m_Data.end());
    }
    constexpr void Scatter(T *p_Data, const SizeType p_Stride) const
    {
        std::byte *data = reinterpret_cast<std::byte *>(p_Data);
        for (SizeType i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(data + i * p_Stride, &m_Data[i], sizeof(T));
    }

#define CREATE_MIN_MAX(p_Name, p_Fun)                                                                                  \
    static constexpr Wide p_Name(const Wide &p_Left, const Wide &p_Right)                                              \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (SizeType i = 0; i < Lanes; ++i)                                                                           \
            wide.m_Data[i] = p_Fun(p_Left.m_Data[i], p_Right.m_Data[i]);                                               \
        return wide;                                                                                                   \
    }

    CREATE_MIN_MAX(Min, std::min)
    CREATE_MIN_MAX(Max, std::max)

    static constexpr Wide Select(const Wide &p_Left, const Wide &p_Right, const Mask p_Mask)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = (p_Mask & (Mask{1} << i)) ? p_Left.m_Data[i] : p_Right.m_Data[i];
        return wide;
    }

#define CREATE_ARITHMETIC_OP(p_Op)                                                                                     \
    friend constexpr Wide operator p_Op(const Wide &p_Left, const Wide &p_Right)                                       \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (SizeType i = 0; i < Lanes; ++i)                                                                           \
            wide.m_Data[i] = p_Left.m_Data[i] p_Op p_Right.m_Data[i];                                                  \
        return wide;                                                                                                   \
    }                                                                                                                  \
    friend constexpr Wide operator p_Op(const Wide &p_Left, const T p_Right)                                           \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (SizeType i = 0; i < Lanes; ++i)                                                                           \
            wide.m_Data[i] = p_Left.m_Data[i] p_Op p_Right;                                                            \
        return wide;                                                                                                   \
    }                                                                                                                  \
    friend constexpr Wide operator p_Op(const T p_Left, const Wide &p_Right)                                           \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (SizeType i = 0; i < Lanes; ++i)                                                                           \
            wide.m_Data[i] = p_Left p_Op p_Right.m_Data[i];                                                            \
        return wide;                                                                                                   \
    }

    CREATE_ARITHMETIC_OP(+)
    CREATE_ARITHMETIC_OP(-)
    CREATE_ARITHMETIC_OP(*)
    CREATE_ARITHMETIC_OP(/)
    CREATE_ARITHMETIC_OP(&)
    CREATE_ARITHMETIC_OP(|)

    friend constexpr Wide operator-(const Wide &p_Other)
    {
        Wide wide;
        for (SizeType i = 0; i < Lanes; ++i)
            wide.m_Data[i] = -p_Other.m_Data[i];
        return wide;
    }

#define CREATE_BITSHIFT_OP(p_Op)                                                                                       \
    friend constexpr Wide operator p_Op(const Wide &p_Left, const i32 p_Shift)                                         \
        requires(Integer<T>)                                                                                           \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (SizeType i = 0; i < Lanes; ++i)                                                                           \
            wide.m_Data[i] = p_Left.m_Data[i] p_Op p_Shift;                                                            \
        return wide;                                                                                                   \
    }

    CREATE_BITSHIFT_OP(<<)
    CREATE_BITSHIFT_OP(>>)

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

    static constexpr T Reduce(const Wide &p_Wide)
    {
        T val{};
        for (SizeType i = 0; i < Lanes; ++i)
            val += p_Wide[i];
        return val;
    }

  private:
    Array<T, Lanes, Traits> m_Data;
}; // namespace TKit::Detail
} // namespace TKit::Detail

#undef CREATE_MIN_MAX
#undef CREATE_CMP_OP
#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP
