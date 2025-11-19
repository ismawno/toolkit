#pragma once

#include "tkit/container/array.hpp"
#include "tkit/simd/utils.hpp"
#include "tkit/utils/limits.hpp"
#include <algorithm>

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4146)
namespace TKit::Simd
{
template <typename T, usize L>
    requires(L > 0 && Arithmetic<T>)
class Wide
{
  public:
    using ValueType = T;

    static constexpr usize Lanes = L;
    static constexpr usize Alignment = alignof(T);

    using Mask = u<MaskSize<Lanes>()>;
    using BitMask = Mask;

    constexpr Wide() = default;
    constexpr explicit Wide(const T *p_Data)
    {
        Memory::ForwardCopy(m_Data.begin(), p_Data, p_Data + Lanes);
    }
    template <typename Callable>
        requires std::invocable<Callable, usize>
    constexpr explicit Wide(Callable &&p_Callable)
    {
        for (usize i = 0; i < Lanes; ++i)
            m_Data[i] = static_cast<T>(std::forward<Callable>(p_Callable)(i));
    }

    template <std::convertible_to<T> U> constexpr explicit Wide(const U p_Data)
    {
        for (usize i = 0; i < Lanes; ++i)
            m_Data[i] = static_cast<T>(p_Data);
    }

    constexpr T At(const usize p_Index) const
    {
        return m_Data[p_Index];
    }
    template <usize Index> constexpr T At() const
    {
        static_assert(Index < Lanes, "[TOOLKIT][SIMD] Index exceeds lane count");
        return m_Data[Index];
    }
    constexpr T operator[](const usize p_Index) const
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
    static constexpr Wide Gather(const T *p_Data, const usize p_Stride)
    {
        TKIT_ASSERT(p_Stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}",
                    p_Stride, sizeof(T));
        TKIT_LOG_WARNING_IF(
            p_Stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous load", p_Stride);
        Wide wide;
        const std::byte *data = reinterpret_cast<const std::byte *>(p_Data);
        for (usize i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(&wide.m_Data[i], data + i * p_Stride, sizeof(T));
        return wide;
    }
    constexpr void Scatter(T *p_Data, const usize p_Stride) const
    {
        TKIT_ASSERT(p_Stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}",
                    p_Stride, sizeof(T));
        TKIT_LOG_WARNING_IF(
            p_Stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous store", p_Stride);
        std::byte *data = reinterpret_cast<std::byte *>(p_Data);
        for (usize i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(data + i * p_Stride, &m_Data[i], sizeof(T));
    }

    template <usize Count>
        requires(Count > 1)
    static constexpr Array<Wide, Count> Gather(const T *p_Data)
    {
        Array<Wide, Count> result;
        for (usize i = 0; i < Count; ++i)
            result[i] = Gather(p_Data + i, Count * sizeof(T));
        return result;
    }
    template <usize Count>
        requires(Count > 1)
    static constexpr void Scatter(T *p_Data, const Array<Wide, Count> &p_Wides)
    {
        for (usize i = 0; i < Count; ++i)
            p_Wides[i].Scatter(p_Data + i, Count * sizeof(T));
    }

    constexpr void StoreAligned(T *p_Data) const
    {
        Memory::ForwardCopy(p_Data, m_Data.begin(), m_Data.end());
    }
    constexpr void StoreUnaligned(T *p_Data) const
    {
        Memory::ForwardCopy(p_Data, m_Data.begin(), m_Data.end());
    }

#define CREATE_MIN_MAX(p_Name, p_Fun)                                                                                  \
    static constexpr Wide p_Name(const Wide &p_Left, const Wide &p_Right)                                              \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = p_Fun(p_Left.m_Data[i], p_Right.m_Data[i]);                                               \
        return wide;                                                                                                   \
    }

    CREATE_MIN_MAX(Min, std::min)
    CREATE_MIN_MAX(Max, std::max)

    static constexpr Wide Select(const Wide &p_Left, const Wide &p_Right, const Mask p_Mask)
    {
        Wide wide;
        for (usize i = 0; i < Lanes; ++i)
            wide.m_Data[i] = (p_Mask & (Mask{1} << i)) ? p_Left.m_Data[i] : p_Right.m_Data[i];
        return wide;
    }

#define CREATE_ARITHMETIC_OP(p_Op, p_Requires)                                                                         \
    friend constexpr Wide operator p_Op(const Wide &p_Left, const Wide &p_Right) p_Requires                            \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = p_Left.m_Data[i] p_Op p_Right.m_Data[i];                                                  \
        return wide;                                                                                                   \
    }                                                                                                                  \
    template <std::convertible_to<T> U>                                                                                \
    friend constexpr Wide operator p_Op(const Wide &p_Left, const U p_Right) p_Requires                                \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = p_Left.m_Data[i] p_Op static_cast<T>(p_Right);                                            \
        return wide;                                                                                                   \
    }                                                                                                                  \
    template <std::convertible_to<T> U>                                                                                \
    friend constexpr Wide operator p_Op(const U p_Left, const Wide &p_Right) p_Requires                                \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = static_cast<T>(p_Left) p_Op p_Right.m_Data[i];                                            \
        return wide;                                                                                                   \
    }                                                                                                                  \
    constexpr Wide &operator p_Op## = (const Wide &p_Other)p_Requires                                                  \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_ARITHMETIC_OP(+, )
    CREATE_ARITHMETIC_OP(-, )
    CREATE_ARITHMETIC_OP(*, )
    CREATE_ARITHMETIC_OP(/, )
    CREATE_ARITHMETIC_OP(&, requires(Integer<T>))
    CREATE_ARITHMETIC_OP(|, requires(Integer<T>))

    friend constexpr Wide operator-(const Wide &p_Other)
    {
        Wide wide;
        for (usize i = 0; i < Lanes; ++i)
            wide.m_Data[i] = -p_Other.m_Data[i];
        return wide;
    }

#define CREATE_BITSHIFT_OP(p_Op)                                                                                       \
    template <std::convertible_to<T> U>                                                                                \
    friend constexpr Wide operator p_Op(const Wide &p_Left, const U p_Shift)                                           \
        requires(Integer<T>)                                                                                           \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = p_Left.m_Data[i] p_Op static_cast<T>(p_Shift);                                            \
        return wide;                                                                                                   \
    }                                                                                                                  \
    constexpr Wide &operator p_Op## = (const Wide &p_Other)                                                            \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_BITSHIFT_OP(<<)
    CREATE_BITSHIFT_OP(>>)

#define CREATE_CMP_OP(p_Op)                                                                                            \
    friend constexpr Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                       \
    {                                                                                                                  \
        Mask m = 0;                                                                                                    \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
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
        for (usize i = 0; i < Lanes; ++i)
            val += p_Wide[i];
        return val;
    }

    static constexpr BitMask PackMask(const Mask p_Mask)
    {
        return p_Mask;
    }
    static constexpr BitMask WidenMask(const BitMask p_Mask)
    {
        return p_Mask;
    }

    static constexpr bool NoneOf(const BitMask p_Mask)
    {
        return p_Mask == 0;
    }
    static constexpr bool AnyOf(const BitMask p_Mask)
    {
        return p_Mask != 0;
    }
    static constexpr bool AllOf(const BitMask p_Mask)
    {
        return p_Mask == Limits<BitMask>::Max();
    }

  private:
    Array<T, Lanes> m_Data;
}; // namespace TKit::Simd
} // namespace TKit::Simd
TKIT_COMPILER_WARNING_IGNORE_POP()

#undef CREATE_MIN_MAX
#undef CREATE_CMP_OP
#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP
