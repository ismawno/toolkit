#pragma once

#include "tkit/preprocessor/system.hpp"
#ifdef TKIT_SIMD_NEON
#    include "tkit/container/array.hpp"
#    include "tkit/simd/utils.hpp"
#    include <arm_neon.h>

namespace TKit::Simd::NEON
{
template <typename T>
concept Arithmetic = Float<T> || Integer<T>;

template <typename T> struct TypeSelector;

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

template <Arithmetic T, typename Traits = Container::ArrayTraits<T>> class Wide
{
    using wide1_t = typename TypeSelector<T>::wide1_t;
    using wide2_t = typename TypeSelector<T>::wide2_t;
    using wide3_t = typename TypeSelector<T>::wide3_t;
    using wide4_t = typename TypeSelector<T>::wide4_t;

    static_assert(!std::is_same_v<wide1_t, void>,
                  "[TOOLKIT][NEON] This build target doesn't support the requested element type in NEON.");

    template <typename E> static constexpr bool s_Equals = std::is_same_v<T, E>;

  public:
    using ValueType = typename Traits::ValueType;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = TKIT_SIMD_NEON_SIZE / sizeof(T);
    static constexpr SizeType Alignment = 16;

    using Mask = typename TypeSelector<u<8 * sizeof(T)>>::wide1_t;
    using BitMask = u<MaskSize<Lanes>()>;

#    define CREATE_BAD_BRANCH()                                                                                        \
        else                                                                                                           \
        {                                                                                                              \
            TKIT_UNREACHABLE();                                                                                        \
        }

    constexpr Wide() = default;
    constexpr Wide(const wide1_t p_Data) : m_Data(p_Data)
    {
    }
    constexpr Wide(T p_Data) : m_Data(set(p_Data))
    {
    }
    constexpr Wide(const T *p_Data) : m_Data(load1(p_Data))
    {
    }

    template <typename Callable>
        requires std::invocable<Callable, SizeType>
    constexpr Wide(Callable &&p_Callable)
        : m_Data(makeIntrinsic(std::forward<Callable>(p_Callable), std::make_index_sequence<Lanes>{}))
    {
    }

    static constexpr Wide LoadAligned(const T *p_Data)
    {
        return Wide{load1(p_Data)};
    }
    static constexpr Wide LoadUnaligned(const T *p_Data)
    {
        return Wide{load1(p_Data)};
    }
    static constexpr Wide Gather(const T *p_Data, const SizeType p_Stride)
    {
        alignas(Alignment) T dst[Lanes];
        const std::byte *src = reinterpret_cast<const std::byte *>(p_Data);

        for (SizeType i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(dst + i, src + i * p_Stride, sizeof(T));
        return Wide{load1(dst)};
    }
    constexpr void Scatter(T *p_Data, const SizeType p_Stride) const
    {
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        std::byte *dst = reinterpret_cast<std::byte *>(p_Data);
        for (SizeType i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(dst + i * p_Stride, &tmp[i], sizeof(T));
    }

    template <SizeType NWides, SizeType Stride = NWides * sizeof(T)>
    static constexpr Array<Wide, NWides> Gather(const T *p_Data)
    {
        Array<Wide, NWides> result;
        if constexpr (NWides != Stride / sizeof(T) || NWides > 4)
        {
            for (SizeType i = 0; i < NWides; ++i)
                result[i] = Gather(p_Data + i, Stride);
            return result;
        }
        else if constexpr (NWides == 1)
            return {Wide{load1(p_Data)}};
        else if constexpr (NWides == 2)
        {
            const auto packed = load2(p_Data);
            return {Wide{packed[0]}, Wide{packed[1]}};
        }
        else if constexpr (NWides == 3)
        {
            const auto packed = load3(p_Data);
            return {Wide{packed[0]}, Wide{packed[1]}, Wide{packed[2]}};
        }
        else
        {
            const auto packed = load4(p_Data);
            return {Wide{packed[0]}, Wide{packed[1]}, Wide{packed[2]}, Wide{packed[3]}};
        }
    }
    template <SizeType NWides, SizeType Stride = NWides * sizeof(T)>
    static constexpr void Scatter(T *p_Data, const Array<Wide, NWides> &p_Wides)
    {
        if constexpr (NWides != Stride / sizeof(T) || NWides > 4)
            for (SizeType i = 0; i < NWides; ++i)
                p_Wides[i].Scatter(p_Data, Stride);
        else if constexpr (NWides == 1)
            store1(p_Data, p_Wides[0].m_Data);
        else if constexpr (NWides == 2)
        {
            wide2_t wide;
            wide[0] = p_Wides[0].m_Data;
            wide[1] = p_Wides[1].m_Data;
            store2(p_Data, wide);
        }
        else if constexpr (NWides == 3)
        {
            wide3_t wide;
            wide[0] = p_Wides[0].m_Data;
            wide[1] = p_Wides[1].m_Data;
            wide[2] = p_Wides[2].m_Data;
            store3(p_Data, wide);
        }
        else
        {
            wide3_t wide;
            wide[0] = p_Wides[0].m_Data;
            wide[1] = p_Wides[1].m_Data;
            wide[2] = p_Wides[2].m_Data;
            wide[3] = p_Wides[3].m_Data;
            store4(p_Data, wide);
        }
    }

    constexpr void StoreAligned(T *p_Data) const
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][NEON] Data must be aligned to {} bytes to use the NEON SIMD set", Alignment);
        store1(p_Data, m_Data);
    }
    constexpr void StoreUnaligned(T *p_Data) const
    {
        store1(p_Data, m_Data);
    }

    constexpr T At(const SizeType p_Index) const
    {
        TKIT_ASSERT(p_Index < Lanes, "[TOOLKIT][NEON] Index exceeds lane count");
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        return tmp[p_Index];
    }
    template <SizeType Index> constexpr T At() const
    {
        return getLane<Index>();
    }
    constexpr T operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }

#    define CREATE_METHOD_INT(p_Func, p_Prefix, p_Suffix, ...)                                                         \
        if constexpr (s_Equals<u64>)                                                                                   \
            p_Prefix p_Func##_u64(__VA_ARGS__) p_Suffix;                                                               \
        else if constexpr (s_Equals<i64>)                                                                              \
            p_Prefix p_Func##_s64(__VA_ARGS__) p_Suffix;                                                               \
        else if constexpr (s_Equals<u32>)                                                                              \
            p_Prefix p_Func##_u32(__VA_ARGS__) p_Suffix;                                                               \
        else if constexpr (s_Equals<i32>)                                                                              \
            p_Prefix p_Func##_s32(__VA_ARGS__) p_Suffix;                                                               \
        else if constexpr (s_Equals<u16>)                                                                              \
            p_Prefix p_Func##_u16(__VA_ARGS__) p_Suffix;                                                               \
        else if constexpr (s_Equals<i16>)                                                                              \
            p_Prefix p_Func##_s16(__VA_ARGS__) p_Suffix;                                                               \
        else if constexpr (s_Equals<u8>)                                                                               \
            p_Prefix p_Func##_u8(__VA_ARGS__) p_Suffix;                                                                \
        else if constexpr (s_Equals<i8>)                                                                               \
            p_Prefix p_Func##_s8(__VA_ARGS__) p_Suffix;                                                                \
        CREATE_BAD_BRANCH()

#    ifdef TKIT_AARCH64
#        define CREATE_METHOD(p_Func, p_Prefix, p_Suffix, ...)                                                         \
            if constexpr (s_Equals<f32>)                                                                               \
                p_Prefix p_Func##_f32(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<f64>)                                                                          \
                p_Prefix p_Func##_f64(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u64>)                                                                          \
                p_Prefix p_Func##_u64(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<i64>)                                                                          \
                p_Prefix p_Func##_s64(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u32>)                                                                          \
                p_Prefix p_Func##_u32(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<i32>)                                                                          \
                p_Prefix p_Func##_s32(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u16>)                                                                          \
                p_Prefix p_Func##_u16(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<i16>)                                                                          \
                p_Prefix p_Func##_s16(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u8>)                                                                           \
                p_Prefix p_Func##_u8(__VA_ARGS__) p_Suffix;                                                            \
            else if constexpr (s_Equals<i8>)                                                                           \
                p_Prefix p_Func##_s8(__VA_ARGS__) p_Suffix;                                                            \
            CREATE_BAD_BRANCH()
#    else
#        define CREATE_METHOD(p_Func, p_Prefix, p_Suffix, ...)                                                         \
            if constexpr (s_Equals<f32>)                                                                               \
                p_Prefix p_Func##_f32(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<f64>)                                                                          \
                p_Prefix p_Func##_f64(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u64>)                                                                          \
                p_Prefix p_Func##_u64(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<i64>)                                                                          \
                p_Prefix p_Func##_s64(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u32>)                                                                          \
                p_Prefix p_Func##_u32(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<i32>)                                                                          \
                p_Prefix p_Func##_s32(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u16>)                                                                          \
                p_Prefix p_Func##_u16(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<i16>)                                                                          \
                p_Prefix p_Func##_s16(__VA_ARGS__) p_Suffix;                                                           \
            else if constexpr (s_Equals<u8>)                                                                           \
                p_Prefix p_Func##_u8(__VA_ARGS__) p_Suffix;                                                            \
            else if constexpr (s_Equals<i8>)                                                                           \
                p_Prefix p_Func##_s8(__VA_ARGS__) p_Suffix;                                                            \
            CREATE_BAD_BRANCH()
#    endif

    static constexpr Wide Select(const Wide &p_Left, const Wide &p_Right, const Mask &p_Mask)
    {
        CREATE_METHOD(vbslq,
                      return Wide{
                          ,
                      },
                      p_Mask, p_Left.m_Data, p_Right.m_Data);
    }

    static constexpr Wide Min(const Wide &p_Left, const Wide &p_Right)
    {
        CREATE_METHOD(vminq,
                      return Wide{
                          ,
                      },
                      p_Left.m_Data, p_Right.m_Data);
    }
    static constexpr Wide Max(const Wide &p_Left, const Wide &p_Right)
    {
        CREATE_METHOD(vmaxq,
                      return Wide{
                          ,
                      },
                      p_Left.m_Data, p_Right.m_Data);
    }

    static constexpr T Reduce(const Wide &p_Wide)
    {
        CREATE_METHOD(vaddvq, return, , p_Wide.m_Data);
    }

#    define CREATE_ARITHMETIC_OP(p_Op, p_OpName)                                                                       \
        friend constexpr Wide operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            CREATE_METHOD(v##p_OpName##q,                                                                              \
                          return Wide{                                                                                 \
                              ,                                                                                        \
                          },                                                                                           \
                          p_Left.m_Data, p_Right.m_Data);                                                              \
        }
#    define CREATE_ARITHMETIC_OP_INT(p_Op, p_OpName)                                                                   \
        friend constexpr Wide operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
            requires(Integer<T>)                                                                                       \
        {                                                                                                              \
            CREATE_METHOD_INT(v##p_OpName##q,                                                                          \
                              return Wide{                                                                             \
                                  ,                                                                                    \
                              },                                                                                       \
                              p_Left.m_Data, p_Right.m_Data);                                                          \
        }
#    define CREATE_BITSHIFT_OP(p_Op, p_ArgName)                                                                        \
        friend constexpr Wide operator p_Op(const Wide &p_Left, const T p_Shift)                                       \
            requires(Integer<T>)                                                                                       \
        {                                                                                                              \
            CREATE_METHOD_INT(vshlq,                                                                                   \
                              return Wide{                                                                             \
                                  ,                                                                                    \
                              },                                                                                       \
                              p_Left.m_Data, set(p_ArgName));                                                          \
        }

    CREATE_ARITHMETIC_OP(+, add)
    CREATE_ARITHMETIC_OP(-, sub)

    CREATE_ARITHMETIC_OP(*, mul)

    CREATE_ARITHMETIC_OP_INT(&, and)
    CREATE_ARITHMETIC_OP_INT(|, orr)

    CREATE_BITSHIFT_OP(>>, -p_Shift)
    CREATE_BITSHIFT_OP(<<, p_Shift)

    friend constexpr Wide operator/(const Wide &p_Left, const Wide &p_Right)
    {
        if constexpr (s_Equals<f32>)
            return Wide{vdivq_f32(p_Left.m_Data, p_Right.m_Data)};
#    ifdef TKIT_AARCH64
        else if constexpr (s_Equals<f64>)
            return Wide{vdivq_f64(p_Left.m_Data, p_Right.m_Data)};
#    endif
        else
        {
#    ifdef TKIT_ALLOW_SCALAR_SIMD_FALLBACKS
            alignas(Alignment) T left[Lanes], right[Lanes], result[Lanes];
            p_Left.StoreAligned(left);
            p_Right.StoreAligned(right);
            for (SizeType i = 0; i < Lanes; ++i)
                result[i] = left[i] / right[i];
            return Wide{loadAligned(result)};
#    else
            static_assert(!std::is_integral_v<T>, "[TOOLKIT][SIMD] NEON does not support integer division."
                                                  " Enable TKIT_ALLOW_SCALAR_SIMD_FALLBACKS if you really need it.");
#    endif
        }
    }

    friend constexpr Wide operator-(const Wide &p_Other)
    {
        return p_Other * static_cast<T>(-1);
    }

#    define CREATE_SELF_OP(p_Op)                                                                                       \
        constexpr Wide &operator p_Op##=(const Wide & p_Other)                                                         \
        {                                                                                                              \
            *this = *this p_Op p_Other;                                                                                \
            return *this;                                                                                              \
        }

#    define CREATE_SCALAR_OP(p_Op)                                                                                     \
        friend constexpr Wide operator p_Op(const Wide &p_Left, const T p_Right)                                       \
        {                                                                                                              \
            return p_Left p_Op Wide{p_Right};                                                                          \
        }                                                                                                              \
        friend constexpr Wide operator p_Op(const T p_Left, const Wide &p_Right)                                       \
        {                                                                                                              \
            return Wide{p_Left} p_Op p_Right;                                                                          \
        }

    CREATE_SCALAR_OP(+)
    CREATE_SCALAR_OP(-)
    CREATE_SCALAR_OP(*)
    CREATE_SCALAR_OP(/)

    CREATE_SELF_OP(+)
    CREATE_SELF_OP(-)
    CREATE_SELF_OP(*)
    CREATE_SELF_OP(/)

    CREATE_SELF_OP(>>)
    CREATE_SELF_OP(<<)
    CREATE_SELF_OP(&)
    CREATE_SELF_OP(|)

#    define CREATE_CMP_OP(p_Op, p_OpName)                                                                              \
        friend constexpr Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            CREATE_METHOD(v##p_OpName##q, return, , p_Left.m_Data, p_Right.m_Data);                                    \
        }

    CREATE_CMP_OP(==, ceq)
    CREATE_CMP_OP(>, cgt)
    CREATE_CMP_OP(<, clt)
    CREATE_CMP_OP(>=, cge)
    CREATE_CMP_OP(<=, cle)

    friend constexpr Mask operator!=(const Wide &p_Left, const Wide &p_Right)
    {
        return invert(p_Left == p_Right);
    }

    static constexpr BitMask PackMask(const Mask &p_Mask)
    {
        const auto eval = [&p_Mask]<std::size_t... I>(std::index_sequence<I...>) constexpr {
            return (((getMaskLane<I>(p_Mask) & 1ull) << I) | ...);
        };
        return eval(std::make_index_sequence<Lanes>{});
    }

    static constexpr Mask WidenMask(const BitMask p_Mask)
    {
        using Integer = u<sizeof(T) * 8>;
        alignas(Alignment) Integer tmp[Lanes];

        for (SizeType i = 0; i < Lanes; ++i)
            tmp[i] = (p_Mask & (BitMask{1} << i)) ? static_cast<Integer>(-1) : Integer{0};

        return load1(reinterpret_cast<const T *>(tmp));
    }

    static constexpr bool NoneOf(const Mask &p_Mask)
    {
        if constexpr (Lanes == 2)
            return (vgetq_lane_u64(p_Mask, 0) | vgetq_lane_u64(p_Mask, 1)) == 0;
        else if constexpr (Lanes == 4)
            vmaxvq_u32(p_Mask) == 0;
        else if constexpr (Lanes == 8)
            vmaxvq_u16(p_Mask) == 0;
        else if constexpr (Lanes == 16)
            vmaxvq_u8(p_Mask) == 0;
    }
    static constexpr bool AnyOf(const Mask &p_Mask)
    {
        if constexpr (Lanes == 2)
            return (vgetq_lane_u64(p_Mask, 0) | vgetq_lane_u64(p_Mask, 1)) != 0;
        else if constexpr (Lanes == 4)
            vminvq_u32(p_Mask) != 0;
        else if constexpr (Lanes == 8)
            vminvq_u16(p_Mask) != 0;
        else if constexpr (Lanes == 16)
            vminvq_u8(p_Mask) != 0;
    }
    static constexpr bool AllOf(const Mask &p_Mask)
    {
        if constexpr (Lanes == 2)
            return (vgetq_lane_u64(p_Mask, 0) & vgetq_lane_u64(p_Mask, 1)) == 0xFFFFFFFFFFFFFFFF;
        else if constexpr (Lanes == 4)
            vmaxvq_u32(p_Mask) == 0xFFFFFFFF;
        else if constexpr (Lanes == 8)
            vmaxvq_u16(p_Mask) == 0xFFFF;
        else if constexpr (Lanes == 16)
            vmaxvq_u8(p_Mask) == 0xFF;
    }

  private:
    static constexpr wide1_t load1(const T *p_Data)
    {
        CREATE_METHOD(vld1q, return, , p_Data);
    }
    static constexpr wide2_t load2(const T *p_Data)
    {
        CREATE_METHOD(vld2q, return, , p_Data);
    }
    static constexpr wide3_t load3(const T *p_Data)
    {
        CREATE_METHOD(vld3q, return, , p_Data);
    }
    static constexpr wide4_t load4(const T *p_Data)
    {
        CREATE_METHOD(vld4q, return, , p_Data);
    }
    static constexpr void store1(T *p_Data, const wide1_t &p_Wide)
    {
        CREATE_METHOD(vst1q, , , p_Data, p_Wide);
    }
    static constexpr void store2(T *p_Data, const wide2_t &p_Wide)
    {
        CREATE_METHOD(vst2q, , , p_Data, p_Wide);
    }
    static constexpr void store3(T *p_Data, const wide3_t &p_Wide)
    {
        CREATE_METHOD(vst3q, , , p_Data, p_Wide);
    }
    static constexpr void store4(T *p_Data, const wide4_t &p_Wide)
    {
        CREATE_METHOD(vst4q, , , p_Data, p_Wide);
    }
    static constexpr wide1_t set(const T p_Data)
    {
        CREATE_METHOD(vdupq_n, return, , p_Data);
    }
    template <SizeType Index> static constexpr T getLane(const wide1_t &p_Wide)
    {
        static_assert(Index < Lanes, "[TOOLKIT][NEON] Index exceeds lane count");
        CREATE_METHOD(vgetq_lane, return, , p_Wide, Index);
    }
    template <SizeType Index> static constexpr u<8 * sizeof(T)> getMaskLane(const Mask &p_Mask)
    {
        static_assert(Index < Lanes, "[TOOLKIT][NEON] Index exceeds lane count");
        if constexpr (Lanes == 2)
            return vgetq_lane_u64(p_Mask, Index);
        else if constexpr (Lanes == 4)
            return vgetq_lane_u32(p_Mask, Index);
        else if constexpr (Lanes == 8)
            return vgetq_lane_u16(p_Mask, Index);
        else if constexpr (Lanes == 16)
            return vgetq_lane_u8(p_Mask, Index);
        CREATE_BAD_BRANCH()
    }
    static constexpr Mask invert(const Mask &p_Mask)
    {
        if constexpr (Lanes == 2)
            return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(p_Mask)));
        else if constexpr (Lanes == 4)
            return vmvnq_u32(p_Mask);
        else if constexpr (Lanes == 8)
            return vmvnq_u16(p_Mask);
        else if constexpr (Lanes == 16)
            return vmvnq_u8(p_Mask);
        CREATE_BAD_BRANCH()
    }
    template <typename Callable, std::size_t... I>
    static constexpr wide1_t makeIntrinsic(Callable &&p_Callable, std::index_sequence<I...>)
    {
        alignas(Alignment) const T data[Lanes] = {static_cast<T>(p_Callable(I))...};
        return load1(data);
    }

    static constexpr uint64x2_t vmulq_u64(const uint64x2_t p_Left, const uint64x2_t p_Right)
    {
        const u64 a0 = vgetq_lane_u64(p_Left, 0) * vgetq_lane_u64(p_Right, 0);
        const u64 a1 = vgetq_lane_u64(p_Left, 1) * vgetq_lane_u64(p_Right, 1);
        return vsetq_lane_u64(a0, vsetq_lane_u64(a1, vdupq_n_u64(0), 1), 0);
    }
    static constexpr int64x2_t vmulq_s64(const int64x2_t p_Left, const int64x2_t p_Right)
    {
        const i64 a0 = vgetq_lane_s64(p_Left, 0) * vgetq_lane_s64(p_Right, 0);
        const i64 a1 = vgetq_lane_s64(p_Left, 1) * vgetq_lane_s64(p_Right, 1);
        return vsetq_lane_s64(a0, vsetq_lane_s64(a1, vdupq_n_s64(0), 1), 0);
    }

    static constexpr uint64x2_t vminq_u64(const uint64x2_t p_Left, const uint64x2_t p_Right)
    {
        return vbslq_u64(vcltq_u64(p_Left, p_Right), p_Left, p_Right);
    }
    static constexpr int64x2_t vminq_u64(const int64x2_t p_Left, const int64x2_t p_Right)
    {
        return vbslq_s64(vcltq_s64(p_Left, p_Right), p_Left, p_Right);
    }
    static constexpr uint64x2_t vmaxq_u64(const uint64x2_t p_Left, const uint64x2_t p_Right)
    {
        return vbslq_u64(vcgtq_u64(p_Left, p_Right), p_Left, p_Right);
    }
    static constexpr int64x2_t vmaxq_u64(const int64x2_t p_Left, const int64x2_t p_Right)
    {
        return vbslq_s64(vcgtq_s64(p_Left, p_Right), p_Left, p_Right);
    }

    wide1_t m_Data;
};

} // namespace TKit::Simd::NEON

#    undef CREATE_BAD_BRANCH
#    undef CREATE_METHOD

#endif
