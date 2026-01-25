#pragma once

#include "tkit/container/fixed_array.hpp"
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
    constexpr explicit Wide(const T *data)
    {
        Memory::ForwardCopy(m_Data.begin(), data, data + Lanes);
    }
    template <typename Callable>
        requires std::invocable<Callable, usize>
    constexpr explicit Wide(Callable &&callable)
    {
        for (usize i = 0; i < Lanes; ++i)
            m_Data[i] = static_cast<T>(std::forward<Callable>(callable)(i));
    }

    template <std::convertible_to<T> U> constexpr Wide(const U data)
    {
        for (usize i = 0; i < Lanes; ++i)
            m_Data[i] = static_cast<T>(data);
    }

    template <std::convertible_to<T> U> constexpr Wide &operator=(const U data)
    {
        for (usize i = 0; i < Lanes; ++i)
            m_Data[i] = static_cast<T>(data);
    }

    constexpr T At(const usize index) const
    {
        return m_Data[index];
    }
    template <usize Index> constexpr T At() const
    {
        static_assert(Index < Lanes, "[TOOLKIT][SIMD] Index exceeds lane count");
        return m_Data[Index];
    }
    constexpr T operator[](const usize index) const
    {
        return m_Data[index];
    }

    static constexpr Wide LoadAligned(const T *data)
    {
        return Wide{data};
    }
    static constexpr Wide LoadUnaligned(const T *data)
    {
        return Wide{data};
    }
    static constexpr Wide Gather(const T *pdata, const usize stride)
    {
        TKIT_ASSERT(stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}", stride,
                    sizeof(T));
        TKIT_LOG_WARNING_IF(
            stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous load", stride);
        Wide wide;
        const std::byte *data = reinterpret_cast<const std::byte *>(pdata);
        for (usize i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(&wide.m_Data[i], data + i * stride, sizeof(T));
        return wide;
    }
    constexpr void Scatter(T *pdata, const usize stride) const
    {
        TKIT_ASSERT(stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}", stride,
                    sizeof(T));
        TKIT_LOG_WARNING_IF(
            stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous store", stride);
        std::byte *data = reinterpret_cast<std::byte *>(pdata);
        for (usize i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(data + i * stride, &m_Data[i], sizeof(T));
    }

    template <usize Count>
        requires(Count > 1)
    static constexpr FixedArray<Wide, Count> Gather(const T *data)
    {
        FixedArray<Wide, Count> result;
        for (usize i = 0; i < Count; ++i)
            result[i] = Gather(data + i, Count * sizeof(T));
        return result;
    }
    template <usize Count>
        requires(Count > 1)
    static constexpr void Scatter(T *data, const FixedArray<Wide, Count> &wides)
    {
        for (usize i = 0; i < Count; ++i)
            wides[i].Scatter(data + i, Count * sizeof(T));
    }

    constexpr void StoreAligned(T *data) const
    {
        Memory::ForwardCopy(data, m_Data.begin(), m_Data.end());
    }
    constexpr void StoreUnaligned(T *data) const
    {
        Memory::ForwardCopy(data, m_Data.begin(), m_Data.end());
    }

#define CREATE_MIN_MAX(name, fun)                                                                                      \
    static constexpr Wide name(const Wide &left, const Wide &right)                                                    \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = fun(left.m_Data[i], right.m_Data[i]);                                                     \
        return wide;                                                                                                   \
    }

    CREATE_MIN_MAX(Min, std::min)
    CREATE_MIN_MAX(Max, std::max)

    static constexpr Wide Select(const Wide &left, const Wide &right, const Mask mask)
    {
        Wide wide;
        for (usize i = 0; i < Lanes; ++i)
            wide.m_Data[i] = (mask & (Mask{1} << i)) ? left.m_Data[i] : right.m_Data[i];
        return wide;
    }

#define CREATE_ARITHMETIC_OP(op, requires)                                                                             \
    friend constexpr Wide operator op(const Wide &left, const Wide &right) requires {                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = left.m_Data[i] op right.m_Data[i];                                                        \
        return wide;                                                                                                   \
    } template <std::convertible_to<T> U>                                                                              \
    friend constexpr Wide operator op(const Wide &left, const U right) requires {                                      \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = left.m_Data[i] op static_cast<T>(right);                                                  \
        return wide;                                                                                                   \
    } template <std::convertible_to<T> U>                                                                              \
    friend constexpr Wide operator op(const U left, const Wide &right) requires {                                      \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = static_cast<T>(left) op right.m_Data[i];                                                  \
        return wide;                                                                                                   \
    } constexpr Wide &operator op## = (const Wide &other) requires {                                                   \
        *this = *this op other;                                                                                        \
        return *this;                                                                                                  \
    }

    CREATE_ARITHMETIC_OP(+, )
    CREATE_ARITHMETIC_OP(-, )
    CREATE_ARITHMETIC_OP(*, )
    CREATE_ARITHMETIC_OP(/, )
    CREATE_ARITHMETIC_OP(&, requires(Integer<T>))
    CREATE_ARITHMETIC_OP(|, requires(Integer<T>))

    friend constexpr Wide operator-(const Wide &other)
    {
        Wide wide;
        for (usize i = 0; i < Lanes; ++i)
            wide.m_Data[i] = -other.m_Data[i];
        return wide;
    }

#define CREATE_BITSHIFT_OP(op)                                                                                         \
    template <std::convertible_to<T> U>                                                                                \
    friend constexpr Wide operator op(const Wide &left, const U shift)                                                 \
        requires(Integer<T>)                                                                                           \
    {                                                                                                                  \
        Wide wide;                                                                                                     \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
            wide.m_Data[i] = left.m_Data[i] op static_cast<T>(shift);                                                  \
        return wide;                                                                                                   \
    }                                                                                                                  \
    constexpr Wide &operator op## = (const Wide &other)                                                                \
    {                                                                                                                  \
        *this = *this op other;                                                                                        \
        return *this;                                                                                                  \
    }

    CREATE_BITSHIFT_OP(<<)
    CREATE_BITSHIFT_OP(>>)

#define CREATE_CMP_OP(op)                                                                                              \
    friend constexpr Mask operator op(const Wide &left, const Wide &right)                                             \
    {                                                                                                                  \
        Mask m = 0;                                                                                                    \
        for (usize i = 0; i < Lanes; ++i)                                                                              \
        {                                                                                                              \
            const Mask mustSet = static_cast<Mask>(left.m_Data[i] op right.m_Data[i]);                                 \
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

    static constexpr T Reduce(const Wide &wide)
    {
        T val{};
        for (usize i = 0; i < Lanes; ++i)
            val += wide[i];
        return val;
    }

    static constexpr BitMask PackMask(const Mask mask)
    {
        return mask;
    }
    static constexpr BitMask WidenMask(const BitMask mask)
    {
        return mask;
    }

    static constexpr bool NoneOf(const BitMask mask)
    {
        return mask == 0;
    }
    static constexpr bool AnyOf(const BitMask mask)
    {
        return mask != 0;
    }
    static constexpr bool AllOf(const BitMask mask)
    {
        return mask == Limits<BitMask>::Max();
    }

  private:
    FixedArray<T, Lanes> m_Data;
}; // namespace TKit::Simd
} // namespace TKit::Simd
TKIT_COMPILER_WARNING_IGNORE_POP()

#undef CREATE_MIN_MAX
#undef CREATE_CMP_OP
#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP
