#pragma once

#include "tkit/preprocessor/system.hpp"
#ifdef TKIT_SIMD_NEON
#    include "tkit/container/fixed_array.hpp"
#    include "tkit/simd/utils.hpp"
#    include <arm_neon.h>

namespace TKit::Simd::NEON
{
template <Arithmetic T> struct TypeSelector;

template <> struct TypeSelector<f32>
{
    using wide1_t = float32x4_t;
    using wide2_t = float32x4x2_t;
    using wide3_t = float32x4x3_t;
    using wide4_t = float32x4x4_t;
};
template <> struct TypeSelector<f64>
{
#    ifdef TKIT_AARCH64
    using wide1_t = float64x2_t;
    using wide2_t = float64x2x2_t;
    using wide3_t = float64x2x3_t;
    using wide4_t = float64x2x4_t;
#    else
    using wide1_t = void;
    using wide2_t = void;
    using wide3_t = void;
    using wide4_t = void;
#    endif
};

template <> struct TypeSelector<u8>
{
    using wide1_t = uint8x16_t;
    using wide2_t = uint8x16x2_t;
    using wide3_t = uint8x16x3_t;
    using wide4_t = uint8x16x4_t;
};
template <> struct TypeSelector<i8>
{
    using wide1_t = int8x16_t;
    using wide2_t = int8x16x2_t;
    using wide3_t = int8x16x3_t;
    using wide4_t = int8x16x4_t;
};
template <> struct TypeSelector<u16>
{
    using wide1_t = uint16x8_t;
    using wide2_t = uint16x8x2_t;
    using wide3_t = uint16x8x3_t;
    using wide4_t = uint16x8x4_t;
};
template <> struct TypeSelector<i16>
{
    using wide1_t = int16x8_t;
    using wide2_t = int16x8x2_t;
    using wide3_t = int16x8x3_t;
    using wide4_t = int16x8x4_t;
};
template <> struct TypeSelector<u32>
{
    using wide1_t = uint32x4_t;
    using wide2_t = uint32x4x2_t;
    using wide3_t = uint32x4x3_t;
    using wide4_t = uint32x4x4_t;
};
template <> struct TypeSelector<i32>
{
    using wide1_t = int32x4_t;
    using wide2_t = int32x4x2_t;
    using wide3_t = int32x4x3_t;
    using wide4_t = int32x4x4_t;
};
template <> struct TypeSelector<u64>
{
    using wide1_t = uint64x2_t;
    using wide2_t = uint64x2x2_t;
    using wide3_t = uint64x2x3_t;
    using wide4_t = uint64x2x4_t;
};
template <> struct TypeSelector<i64>
{
    using wide1_t = int64x2_t;
    using wide2_t = int64x2x2_t;
    using wide3_t = int64x2x3_t;
    using wide4_t = int64x2x4_t;
};

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wmaybe-uninitialized")
template <Arithmetic T> class Wide
{
    using wide1_t = typename TypeSelector<T>::wide1_t;
    using wide2_t = typename TypeSelector<T>::wide2_t;
    using wide3_t = typename TypeSelector<T>::wide3_t;
    using wide4_t = typename TypeSelector<T>::wide4_t;

    static_assert(!std::is_same_v<wide1_t, void>,
                  "[TOOLKIT][NEON] This build target doesn't support the requested element type in NEON.");

    template <typename E> static constexpr bool s_Equals = std::is_same_v<T, E>;

  public:
    using ValueType = T;

    static constexpr usize Lanes = TKIT_SIMD_NEON_SIZE / sizeof(T);
    static constexpr usize Alignment = 16;

    using Mask = typename TypeSelector<u<8 * sizeof(T)>>::wide1_t;
    using BitMask = u<MaskSize<Lanes>()>;

#    define CREATE_BAD_BRANCH()                                                                                        \
        else                                                                                                           \
        {                                                                                                              \
            TKIT_UNREACHABLE();                                                                                        \
        }

    constexpr Wide() = default;
    constexpr Wide(const wide1_t data) : m_Data(data)
    {
    }
    template <std::convertible_to<T> U> constexpr Wide(const U data) : m_Data(set(static_cast<T>(data)))
    {
    }
    constexpr explicit Wide(const T *data) : m_Data(load1(data))
    {
    }

    template <typename Callable>
        requires std::invocable<Callable, usize>
    constexpr explicit Wide(Callable &&callable)
        : m_Data(makeIntrinsic(std::forward<Callable>(callable), std::make_integer_sequence<usize, Lanes>{}))
    {
    }

    template <std::convertible_to<T> U> constexpr Wide &operator=(const U data)
    {
        m_Data = set(static_cast<T>(data));
    }

    static constexpr Wide LoadAligned(const T *data)
    {
        return Wide{load1(data)};
    }
    static constexpr Wide LoadUnaligned(const T *data)
    {
        return Wide{load1(data)};
    }
    static constexpr Wide Gather(const T *data, const usize stride)
    {
        TKIT_ASSERT(stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}", stride,
                    sizeof(T));
        TKIT_LOG_WARNING_IF(
            stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous load", stride);

        alignas(Alignment) T dst[Lanes];
        const std::byte *src = reinterpret_cast<const std::byte *>(data);

        for (usize i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(dst + i, src + i * stride, sizeof(T));
        return Wide{load1(dst)};
    }
    constexpr void Scatter(T *data, const usize stride) const
    {
        TKIT_ASSERT(stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}", stride,
                    sizeof(T));
        TKIT_LOG_WARNING_IF(
            stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous store", stride);
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        std::byte *dst = reinterpret_cast<std::byte *>(data);
        for (usize i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(dst + i * stride, &tmp[i], sizeof(T));
    }

    template <usize Count>
        requires(Count > 1)
    static constexpr FixedArray<Wide, Count> Gather(const T *data)
    {
        FixedArray<Wide, Count> result;
        if constexpr (Count > 4)
        {
            for (usize i = 0; i < Count; ++i)
                result[i] = Gather(data + i, Count * sizeof(T));
            return result;
        }
        else if constexpr (Count == 1)
            return {Wide{load1(data)}};
        else if constexpr (Count == 2)
        {
            const wide2_t packed = load2(data);
            return {Wide{packed.val[0]}, Wide{packed.val[1]}};
        }
        else if constexpr (Count == 3)
        {
            const wide3_t packed = load3(data);
            return {Wide{packed.val[0]}, Wide{packed.val[1]}, Wide{packed.val[2]}};
        }
        else
        {
            const wide4_t packed = load4(data);
            return {Wide{packed.val[0]}, Wide{packed.val[1]}, Wide{packed.val[2]}, Wide{packed.val[3]}};
        }
    }
    template <usize Count>
        requires(Count > 1)
    static constexpr void Scatter(T *data, const FixedArray<Wide, Count> &wides)
    {
        if constexpr (Count > 4)
            for (usize i = 0; i < Count; ++i)
                wides[i].Scatter(data, Count * sizeof(T));
        else if constexpr (Count == 1)
            store1(data, wides[0].m_Data);
        else if constexpr (Count == 2)
        {
            wide2_t wide;
            wide.val[0] = wides[0].m_Data;
            wide.val[1] = wides[1].m_Data;
            store2(data, wide);
        }
        else if constexpr (Count == 3)
        {
            wide3_t wide;
            wide.val[0] = wides[0].m_Data;
            wide.val[1] = wides[1].m_Data;
            wide.val[2] = wides[2].m_Data;
            store3(data, wide);
        }
        else
        {
            wide4_t wide;
            wide.val[0] = wides[0].m_Data;
            wide.val[1] = wides[1].m_Data;
            wide.val[2] = wides[2].m_Data;
            wide.val[3] = wides[3].m_Data;
            store4(data, wide);
        }
    }

    constexpr void StoreAligned(T *data) const
    {
        TKIT_ASSERT(Memory::IsAligned(data, Alignment),
                    "[TOOLKIT][NEON] Data must be aligned to {} bytes to use the NEON SIMD set", Alignment);
        store1(data, m_Data);
    }
    constexpr void StoreUnaligned(T *data) const
    {
        store1(data, m_Data);
    }

    constexpr T At(const usize index) const
    {
        TKIT_ASSERT(index < Lanes, "[TOOLKIT][NEON] Index exceeds lane count: {} >= {}", index, Lanes);
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        return tmp[index];
    }
    template <usize Index> constexpr T At() const
    {
        return getLane<Index>();
    }
    constexpr T operator[](const usize index) const
    {
        return At(index);
    }

#    define CREATE_METHOD_INT(func, prefix, suffix, ...)                                                               \
        if constexpr (s_Equals<u64>)                                                                                   \
            prefix func##_u64(__VA_ARGS__) suffix;                                                                     \
        else if constexpr (s_Equals<i64>)                                                                              \
            prefix func##_s64(__VA_ARGS__) suffix;                                                                     \
        else if constexpr (s_Equals<u32>)                                                                              \
            prefix func##_u32(__VA_ARGS__) suffix;                                                                     \
        else if constexpr (s_Equals<i32>)                                                                              \
            prefix func##_s32(__VA_ARGS__) suffix;                                                                     \
        else if constexpr (s_Equals<u16>)                                                                              \
            prefix func##_u16(__VA_ARGS__) suffix;                                                                     \
        else if constexpr (s_Equals<i16>)                                                                              \
            prefix func##_s16(__VA_ARGS__) suffix;                                                                     \
        else if constexpr (s_Equals<u8>)                                                                               \
            prefix func##_u8(__VA_ARGS__) suffix;                                                                      \
        else if constexpr (s_Equals<i8>)                                                                               \
            prefix func##_s8(__VA_ARGS__) suffix;                                                                      \
        CREATE_BAD_BRANCH()

#    ifdef TKIT_AARCH64
#        define CREATE_METHOD(func, prefix, suffix, ...)                                                               \
            if constexpr (s_Equals<f32>)                                                                               \
                prefix func##_f32(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<f64>)                                                                          \
                prefix func##_f64(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u64>)                                                                          \
                prefix func##_u64(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<i64>)                                                                          \
                prefix func##_s64(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u32>)                                                                          \
                prefix func##_u32(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<i32>)                                                                          \
                prefix func##_s32(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u16>)                                                                          \
                prefix func##_u16(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<i16>)                                                                          \
                prefix func##_s16(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u8>)                                                                           \
                prefix func##_u8(__VA_ARGS__) suffix;                                                                  \
            else if constexpr (s_Equals<i8>)                                                                           \
                prefix func##_s8(__VA_ARGS__) suffix;                                                                  \
            CREATE_BAD_BRANCH()
#    else
#        define CREATE_METHOD(func, prefix, suffix, ...)                                                               \
            if constexpr (s_Equals<f32>)                                                                               \
                prefix func##_f32(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<f64>)                                                                          \
                prefix func##_f64(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u64>)                                                                          \
                prefix func##_u64(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<i64>)                                                                          \
                prefix func##_s64(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u32>)                                                                          \
                prefix func##_u32(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<i32>)                                                                          \
                prefix func##_s32(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u16>)                                                                          \
                prefix func##_u16(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<i16>)                                                                          \
                prefix func##_s16(__VA_ARGS__) suffix;                                                                 \
            else if constexpr (s_Equals<u8>)                                                                           \
                prefix func##_u8(__VA_ARGS__) suffix;                                                                  \
            else if constexpr (s_Equals<i8>)                                                                           \
                prefix func##_s8(__VA_ARGS__) suffix;                                                                  \
            CREATE_BAD_BRANCH()
#    endif

    static constexpr Wide Select(const Wide &left, const Wide &right, const Mask &mask)
    {
        CREATE_METHOD(vbslq,
                      return Wide{
                          ,
                      },
                      mask, left.m_Data, right.m_Data);
    }

    static constexpr Wide Min(const Wide &left, const Wide &right)
    {
        CREATE_METHOD(vminq,
                      return Wide{
                          ,
                      },
                      left.m_Data, right.m_Data);
    }
    static constexpr Wide Max(const Wide &left, const Wide &right)
    {
        CREATE_METHOD(vmaxq,
                      return Wide{
                          ,
                      },
                      left.m_Data, right.m_Data);
    }

    static constexpr T Reduce(const Wide &wide)
    {
        CREATE_METHOD(vaddvq, return, , wide.m_Data);
    }

#    define CREATE_ARITHMETIC_OP(op, opName)                                                                           \
        friend constexpr Wide operator op(const Wide &left, const Wide &right)                                         \
        {                                                                                                              \
            CREATE_METHOD(v##opName##q,                                                                                \
                          return Wide{                                                                                 \
                              ,                                                                                        \
                          },                                                                                           \
                          left.m_Data, right.m_Data);                                                                  \
        }
#    define CREATE_ARITHMETIC_OP_INT(op, opName)                                                                       \
        friend constexpr Wide operator op(const Wide &left, const Wide &right)                                         \
            requires(Integer<T>)                                                                                       \
        {                                                                                                              \
            CREATE_METHOD_INT(v##opName##q,                                                                            \
                              return Wide{                                                                             \
                                  ,                                                                                    \
                              },                                                                                       \
                              left.m_Data, right.m_Data);                                                              \
        }
#    define CREATE_BITSHIFT_OP(op, argName)                                                                            \
        template <std::convertible_to<T> U>                                                                            \
        friend constexpr Wide operator op(const Wide &left, const U shift)                                             \
            requires(Integer<T>)                                                                                       \
        {                                                                                                              \
            CREATE_METHOD_INT(vshlq,                                                                                   \
                              return Wide{                                                                             \
                                  ,                                                                                    \
                              },                                                                                       \
                              left.m_Data, set(static_cast<T>(argName)));                                              \
        }

    CREATE_ARITHMETIC_OP(+, add)
    CREATE_ARITHMETIC_OP(-, sub)

    CREATE_ARITHMETIC_OP(*, mul)

    CREATE_ARITHMETIC_OP_INT(&, and)
    CREATE_ARITHMETIC_OP_INT(|, orr)

    CREATE_BITSHIFT_OP(>>, -shift)
    CREATE_BITSHIFT_OP(<<, shift)

    friend constexpr Wide operator/(const Wide &left, const Wide &right)
    {
        if constexpr (s_Equals<f32>)
            return Wide{vdivq_f32(left.m_Data, right.m_Data)};
#    ifdef TKIT_AARCH64
        else if constexpr (s_Equals<f64>)
            return Wide{vdivq_f64(left.m_Data, right.m_Data)};
#    endif
        else
        {
#    ifdef TKIT_ALLOW_SCALAR_SIMD_FALLBACKS
            alignas(Alignment) T left[Lanes], right[Lanes], result[Lanes];
            left.StoreAligned(left);
            right.StoreAligned(right);
            for (usize i = 0; i < Lanes; ++i)
                result[i] = left[i] / right[i];
            return Wide{loadAligned(result)};
#    else
            static_assert(!std::is_integral_v<T>, "[TOOLKIT][SIMD] NEON does not support integer division."
                                                  " Enable TKIT_ALLOW_SCALAR_SIMD_FALLBACKS if you really need it.");
#    endif
        }
    }

    friend constexpr Wide operator-(const Wide &other)
    {
        return other * static_cast<T>(-1);
    }

#    define CREATE_SELF_OP(op, requires)                                                                               \
        constexpr Wide &operator op## = (const Wide &other) requires {                                                 \
            *this = *this op other;                                                                                    \
            return *this;                                                                                              \
        }

#    define CREATE_SCALAR_OP(op, requires)                                                                             \
        template <std::convertible_to<T> U>                                                                            \
        friend constexpr Wide operator op(const Wide &left, const U right) requires {                                  \
            return left op Wide{right};                                                                                \
        } template <std::convertible_to<T> U>                                                                          \
        friend constexpr Wide operator op(const U left, const Wide &right) requires { return Wide{left} op right; }

    CREATE_SCALAR_OP(+, )
    CREATE_SCALAR_OP(-, )
    CREATE_SCALAR_OP(*, )
    CREATE_SCALAR_OP(/, )

    CREATE_SELF_OP(+, )
    CREATE_SELF_OP(-, )
    CREATE_SELF_OP(*, )
    CREATE_SELF_OP(/, )

    CREATE_SELF_OP(>>, requires(Integer<T>))
    CREATE_SELF_OP(<<, requires(Integer<T>))
    CREATE_SELF_OP(&, requires(Integer<T>))
    CREATE_SELF_OP(|, requires(Integer<T>))

#    define CREATE_CMP_OP(op, opName)                                                                                  \
        friend constexpr Mask operator op(const Wide &left, const Wide &right)                                         \
        {                                                                                                              \
            CREATE_METHOD(v##opName##q, return, , left.m_Data, right.m_Data);                                          \
        }

    CREATE_CMP_OP(==, ceq)
    CREATE_CMP_OP(>, cgt)
    CREATE_CMP_OP(<, clt)
    CREATE_CMP_OP(>=, cge)
    CREATE_CMP_OP(<=, cle)

    friend constexpr Mask operator!=(const Wide &left, const Wide &right)
    {
        return invert(left == right);
    }

    static constexpr BitMask PackMask(const Mask &mask)
    {
        const auto eval = [&mask]<usize... I>(std::integer_sequence<usize, I...>) constexpr {
            return (((getMaskLane<I>(mask) & 1ull) << I) | ...);
        };
        return eval(std::make_integer_sequence<usize, Lanes>{});
    }

    static constexpr Mask WidenMask(const BitMask mask)
    {
        using Integer = u<sizeof(T) * 8>;
        alignas(Alignment) Integer tmp[Lanes];

        for (usize i = 0; i < Lanes; ++i)
            tmp[i] = (mask & (BitMask{1} << i)) ? static_cast<Integer>(-1) : Integer{0};

        return load1(reinterpret_cast<const T *>(tmp));
    }

    static constexpr bool NoneOf(const Mask &mask)
    {
        if constexpr (Lanes == 2)
            return (vgetq_lane_u64(mask, 0) | vgetq_lane_u64(mask, 1)) == 0;
        else if constexpr (Lanes == 4)
            vmaxvq_u32(mask) == 0;
        else if constexpr (Lanes == 8)
            vmaxvq_u16(mask) == 0;
        else if constexpr (Lanes == 16)
            vmaxvq_u8(mask) == 0;
    }
    static constexpr bool AnyOf(const Mask &mask)
    {
        if constexpr (Lanes == 2)
            return (vgetq_lane_u64(mask, 0) | vgetq_lane_u64(mask, 1)) != 0;
        else if constexpr (Lanes == 4)
            vminvq_u32(mask) != 0;
        else if constexpr (Lanes == 8)
            vminvq_u16(mask) != 0;
        else if constexpr (Lanes == 16)
            vminvq_u8(mask) != 0;
    }
    static constexpr bool AllOf(const Mask &mask)
    {
        if constexpr (Lanes == 2)
            return (vgetq_lane_u64(mask, 0) & vgetq_lane_u64(mask, 1)) == 0xFFFFFFFFFFFFFFFF;
        else if constexpr (Lanes == 4)
            vmaxvq_u32(mask) == 0xFFFFFFFF;
        else if constexpr (Lanes == 8)
            vmaxvq_u16(mask) == 0xFFFF;
        else if constexpr (Lanes == 16)
            vmaxvq_u8(mask) == 0xFF;
    }

  private:
    static constexpr wide1_t load1(const T *data)
    {
        CREATE_METHOD(vld1q, return, , data);
    }
    static constexpr wide2_t load2(const T *data)
    {
        CREATE_METHOD(vld2q, return, , data);
    }
    static constexpr wide3_t load3(const T *data)
    {
        CREATE_METHOD(vld3q, return, , data);
    }
    static constexpr wide4_t load4(const T *data)
    {
        CREATE_METHOD(vld4q, return, , data);
    }
    static constexpr void store1(T *data, const wide1_t &wide)
    {
        CREATE_METHOD(vst1q, , , data, wide);
    }
    static constexpr void store2(T *data, const wide2_t &wide)
    {
        CREATE_METHOD(vst2q, , , data, wide);
    }
    static constexpr void store3(T *data, const wide3_t &wide)
    {
        CREATE_METHOD(vst3q, , , data, wide);
    }
    static constexpr void store4(T *data, const wide4_t &wide)
    {
        CREATE_METHOD(vst4q, , , data, wide);
    }
    static constexpr wide1_t set(const T data)
    {
        CREATE_METHOD(vdupq_n, return, , data);
    }
    template <usize Index> static constexpr T getLane(const wide1_t &wide)
    {
        static_assert(Index < Lanes, "[TOOLKIT][NEON] Index exceeds lane count");
        CREATE_METHOD(vgetq_lane, return, , wide, Index);
    }
    template <usize Index> static constexpr u<8 * sizeof(T)> getMaskLane(const Mask &mask)
    {
        static_assert(Index < Lanes, "[TOOLKIT][NEON] Index exceeds lane count");
        if constexpr (Lanes == 2)
            return vgetq_lane_u64(mask, Index);
        else if constexpr (Lanes == 4)
            return vgetq_lane_u32(mask, Index);
        else if constexpr (Lanes == 8)
            return vgetq_lane_u16(mask, Index);
        else if constexpr (Lanes == 16)
            return vgetq_lane_u8(mask, Index);
        CREATE_BAD_BRANCH()
    }
    static constexpr Mask invert(const Mask &mask)
    {
        if constexpr (Lanes == 2)
            return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(mask)));
        else if constexpr (Lanes == 4)
            return vmvnq_u32(mask);
        else if constexpr (Lanes == 8)
            return vmvnq_u16(mask);
        else if constexpr (Lanes == 16)
            return vmvnq_u8(mask);
        CREATE_BAD_BRANCH()
    }
    template <typename Callable, usize... I>
    static constexpr wide1_t makeIntrinsic(Callable &&callable, std::integer_sequence<usize, I...>)
    {
        alignas(Alignment) const T data[Lanes] = {static_cast<T>(std::forward<Callable>(callable)(I))...};
        return load1(data);
    }

    static constexpr uint64x2_t vmulq_u64(const uint64x2_t left, const uint64x2_t right)
    {
        const u64 a0 = vgetq_lane_u64(left, 0) * vgetq_lane_u64(right, 0);
        const u64 a1 = vgetq_lane_u64(left, 1) * vgetq_lane_u64(right, 1);
        return vsetq_lane_u64(a0, vsetq_lane_u64(a1, vdupq_n_u64(0), 1), 0);
    }
    static constexpr int64x2_t vmulq_s64(const int64x2_t left, const int64x2_t right)
    {
        const i64 a0 = vgetq_lane_s64(left, 0) * vgetq_lane_s64(right, 0);
        const i64 a1 = vgetq_lane_s64(left, 1) * vgetq_lane_s64(right, 1);
        return vsetq_lane_s64(a0, vsetq_lane_s64(a1, vdupq_n_s64(0), 1), 0);
    }

    static constexpr uint64x2_t vminq_u64(const uint64x2_t left, const uint64x2_t right)
    {
        return vbslq_u64(vcltq_u64(left, right), left, right);
    }
    static constexpr int64x2_t vminq_u64(const int64x2_t left, const int64x2_t right)
    {
        return vbslq_s64(vcltq_s64(left, right), left, right);
    }
    static constexpr uint64x2_t vmaxq_u64(const uint64x2_t left, const uint64x2_t right)
    {
        return vbslq_u64(vcgtq_u64(left, right), left, right);
    }
    static constexpr int64x2_t vmaxq_u64(const int64x2_t left, const int64x2_t right)
    {
        return vbslq_s64(vcgtq_s64(left, right), left, right);
    }

    wide1_t m_Data;
};
TKIT_COMPILER_WARNING_IGNORE_POP()

} // namespace TKit::Simd::NEON

#    undef CREATE_BAD_BRANCH
#    undef CREATE_METHOD
#    undef CREATE_METHOD_INT
#    undef CREATE_ARITHMETIC_OP
#    undef CREATE_ARITHMETIC_OP_INT
#    undef CREATE_BITSHIFT_OP
#    undef CREATE_SCALAR_OP
#    undef CREATE_CMP_OP

#endif
