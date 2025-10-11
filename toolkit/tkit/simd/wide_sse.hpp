#pragma once

#include "tkit/preprocessor/system.hpp"
#ifdef TKIT_SIMD_SSE2

#    include "tkit/memory/memory.hpp"
#    include "tkit/simd/utils.hpp"
#    include "tkit/container/container.hpp"
#    ifdef TKIT_SIMD_SSE4_2
#        include <nmmintrin.h>
#    elif defined(TKIT_SIMD_SSE4_1)
#        include <smmintrin.h>
#    elif defined(TKIT_SIMD_SSSE3)
#        include <tmmintrin.h>
#    elif defined(TKIT_SIMD_SSE3)
#        include <pmmintrin.h>
#    elif defined(TKIT_SIMD_SSE2)
#        include <emmintrin.h>
#    endif
#    if defined(TKIT_SIMD_AVX2) || defined(TKIT_BMI2)
#        include <immintrin.h>
#    endif
namespace TKit::Detail::SSE
{
template <typename T>
concept Arithmetic = Float<T> || Integer<T>;

template <Arithmetic T> struct TypeSelector;

template <> struct TypeSelector<f32>
{
    using Type = __m128;
};
template <> struct TypeSelector<f64>
{
    using Type = __m128d;
};

template <Integer T> struct TypeSelector<T>
{
    using Type = __m128i;
};

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wignored-attributes")
TKIT_CLANG_WARNING_IGNORE("-Wignored-attributes")
template <Arithmetic T, typename Traits = Container::ArrayTraits<T>> class Wide
{
    using m128 = TypeSelector<T>::Type;

    template <typename E> static constexpr bool s_Equals = std::is_same_v<m128, E>;
    template <usize Size> static constexpr bool s_IsSize = Size == sizeof(T);

  public:
    using ValueType = T;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = TKIT_SIMD_SSE_SIZE / sizeof(T);
    static constexpr SizeType Alignment = 16;

    using Mask = m128;
    using BitMask = u<MaskSize<Lanes>()>;

#    define CREATE_BAD_BRANCH()                                                                                        \
        else                                                                                                           \
        {                                                                                                              \
            TKIT_UNREACHABLE();                                                                                        \
        }
    static constexpr BitMask PackMask(const Mask &p_Mask)
    {
        if constexpr (s_Equals<__m128>)
            return static_cast<BitMask>(_mm_movemask_ps(p_Mask));
        else if constexpr (s_Equals<__m128d>)
            return static_cast<BitMask>(_mm_movemask_pd(p_Mask));
        else if constexpr (s_Equals<__m128i>)
        {
            const u32 byteMask = static_cast<u32>(_mm_movemask_epi8(p_Mask));
            if constexpr (s_IsSize<1>)
                return static_cast<BitMask>(byteMask);
#    ifdef TKIT_BMI2
            else if constexpr (s_IsSize<2>)
                return static_cast<BitMask>(_pext_u32(byteMask, 0x55555555u));
            else if constexpr (s_IsSize<4>)
                return static_cast<BitMask>(_pext_u32(byteMask, 0x11111111u));
            else if constexpr (s_IsSize<8>)
                return static_cast<BitMask>(_pext_u32(byteMask, 0x01010101u));
#    endif
            else
            {
                BitMask packed = 0;
                for (SizeType i = 0; i < Lanes; ++i)
                    packed |= ((byteMask >> (i * sizeof(T))) & 1u) << i;

                return packed;
            }
        }
        CREATE_BAD_BRANCH()
    }

    static constexpr Mask WidenMask(const BitMask p_Bits)
    {
        using Integer = u<sizeof(T) * 8>;
        alignas(Alignment) Integer tmp[Lanes];

        for (SizeType i = 0; i < Lanes; ++i)
            tmp[i] = (p_Bits & (BitMask{1} << i)) ? static_cast<Integer>(-1) : Integer{0};

        return loadAligned(reinterpret_cast<const T *>(tmp));
    }

    constexpr Wide() = default;
    constexpr Wide(const m128 p_Data) : m_Data(p_Data)
    {
    }
    constexpr Wide(const T p_Data) : m_Data(set(p_Data))
    {
    }
    constexpr Wide(const T *p_Data) : m_Data(loadAligned(p_Data))
    {
    }
    constexpr Wide(const T *p_Data, const SizeType p_Stride) : m_Data(gather(p_Data, p_Stride))
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
        return Wide{loadAligned(p_Data)};
    }
    static constexpr Wide LoadUnaligned(const T *p_Data)
    {
        return Wide{loadUnaligned(p_Data)};
    }
    static constexpr Wide Gather(const T *p_Data, const SizeType p_Stride)
    {
        return Wide{p_Data, p_Stride};
    }

    constexpr void StoreAligned(T *p_Data) const
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][SSE] Data must be aligned to {} bytes to use the SSE SIMD set", Alignment);
        if constexpr (s_Equals<__m128>)
            _mm_store_ps(p_Data, m_Data);
        else if constexpr (s_Equals<__m128d>)
            _mm_store_pd(p_Data, m_Data);
        else if constexpr (s_Equals<__m128i>)
            _mm_store_si128(reinterpret_cast<__m128i *>(p_Data), m_Data);
        CREATE_BAD_BRANCH()
    }
    constexpr void StoreUnaligned(T *p_Data) const
    {
        if constexpr (s_Equals<__m128>)
            _mm_storeu_ps(p_Data, m_Data);
        else if constexpr (s_Equals<__m128d>)
            _mm_storeu_pd(p_Data, m_Data);
        else if constexpr (s_Equals<__m128i>)
            _mm_storeu_si128(reinterpret_cast<__m128i *>(p_Data), m_Data);
        CREATE_BAD_BRANCH()
    }
    constexpr void Scatter(T *p_Data, const SizeType p_Stride) const
    {
        alignas(Alignment) T src[Lanes];
        StoreAligned(src);

        std::byte *dst = reinterpret_cast<std::byte *>(p_Data);
        for (SizeType i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(dst + i * p_Stride, &src[i], sizeof(T));
    }

    constexpr const ValueType At(const SizeType p_Index) const
    {
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        return tmp[p_Index];
    }
    constexpr const ValueType operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }

    static constexpr Wide Select(const Wide &p_Left, const Wide &p_Right, const Mask &p_Mask)
    {
        if constexpr (s_Equals<__m128>)
            return Wide{_mm_blendv_ps(p_Right.m_Data, p_Left.m_Data, p_Mask)};
        else if constexpr (s_Equals<__m128d>)
            return Wide{_mm_blendv_pd(p_Right.m_Data, p_Left.m_Data, p_Mask)};
        else if constexpr (s_Equals<__m128i>)
            return Wide{_mm_blendv_epi8(p_Right.m_Data, p_Left.m_Data, p_Mask)};
        CREATE_BAD_BRANCH()
    }

#    define CREATE_MIN_MAX(p_Name, p_Op)                                                                               \
        static constexpr Wide p_Name(const Wide &p_Left, const Wide &p_Right)                                          \
        {                                                                                                              \
            if constexpr (s_Equals<__m128>)                                                                            \
                return Wide{_mm_##p_Op##_ps(p_Left.m_Data, p_Right.m_Data)};                                           \
            else if constexpr (s_Equals<__m128d>)                                                                      \
                return Wide{_mm_##p_Op##_pd(p_Left.m_Data, p_Right.m_Data)};                                           \
            else if constexpr (s_Equals<__m128i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm_##p_Op##_epi64(p_Left.m_Data, p_Right.m_Data)};                                    \
                else if constexpr (std::is_signed_v<T>)                                                                \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm_##p_Op##_epi32(p_Left.m_Data, p_Right.m_Data)};                                \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm_##p_Op##_epi16(p_Left.m_Data, p_Right.m_Data)};                                \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm_##p_Op##_epi8(p_Left.m_Data, p_Right.m_Data)};                                 \
                }                                                                                                      \
                else                                                                                                   \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm_##p_Op##_epu32(p_Left.m_Data, p_Right.m_Data)};                                \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm_##p_Op##_epu16(p_Left.m_Data, p_Right.m_Data)};                                \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm_##p_Op##_epu8(p_Left.m_Data, p_Right.m_Data)};                                 \
                }                                                                                                      \
            }                                                                                                          \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_MIN_MAX(Min, min)
    CREATE_MIN_MAX(Max, max)

#    define CREATE_ARITHMETIC_OP(p_Op, p_FloatOp, p_IntOp)                                                             \
        friend constexpr Wide operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            if constexpr (s_Equals<__m128>)                                                                            \
                return Wide{_mm_##p_FloatOp##_ps(p_Left.m_Data, p_Right.m_Data)};                                      \
            else if constexpr (s_Equals<__m128d>)                                                                      \
                return Wide{_mm_##p_FloatOp##_pd(p_Left.m_Data, p_Right.m_Data)};                                      \
            else if constexpr (s_Equals<__m128i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm_##p_IntOp##_epi64(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return Wide{_mm_##p_IntOp##_epi32(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return Wide{_mm_##p_IntOp##_epi16(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return Wide{_mm_##p_IntOp##_epi8(p_Left.m_Data, p_Right.m_Data)};                                  \
                CREATE_BAD_BRANCH()                                                                                    \
            }                                                                                                          \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_ARITHMETIC_OP(+, add, add)
    CREATE_ARITHMETIC_OP(-, sub, sub)
    CREATE_ARITHMETIC_OP(*, mul, mullo)

    friend constexpr Wide operator/(const Wide &p_Left, const Wide &p_Right)
    {
        if constexpr (s_Equals<__m128>)
            return Wide{_mm_div_ps(p_Left.m_Data, p_Right.m_Data)};
        else if constexpr (s_Equals<__m128d>)
            return Wide{_mm_div_pd(p_Left.m_Data, p_Right.m_Data)};
        else if constexpr (s_Equals<__m128i>)
        {
#    ifdef TKIT_ALLOW_SCALAR_SIMD_FALLBACKS
            alignas(Alignment) T left[Lanes];
            alignas(Alignment) T right[Lanes];
            alignas(Alignment) T result[Lanes];

            p_Left.StoreAligned(left);
            p_Right.StoreAligned(right);
            for (SizeType i = 0; i < Lanes; ++i)
                result[i] = left[i] / right[i];

            return Wide{result};
#    else
            static_assert(!s_Equals<__m128i>, "[TOOLKIT][SIMD] SSE does not support integer division. Scalar "
                                              "fallback is disabled by default. "
                                              "If you really need it, enable TKIT_ALLOW_SCALAR_SIMD_FALLBACKS.");
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

    friend constexpr Wide operator>>(const Wide &p_Left, const i32 p_Shift)
        requires(Integer<T>)
    {
        if constexpr (s_IsSize<8>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi64(p_Left.m_Data, p_Shift)};
            else
                return Wide{_mm_srl_epi64(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        }
        else if constexpr (s_IsSize<4>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi32(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
            else
                return Wide{_mm_srl_epi32(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        }
        else if constexpr (s_IsSize<2>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi16(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
            else
                return Wide{_mm_srl_epi16(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        }
        else if constexpr (s_IsSize<1>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi8(p_Left.m_Data, p_Shift)};
            else
                return Wide{_mm_srl_epi8(p_Left.m_Data, p_Shift)};
        }
    }
    friend constexpr Wide operator<<(const Wide &p_Left, const i32 p_Shift)
        requires(Integer<T>)
    {
        if constexpr (s_IsSize<8>)
            return Wide{_mm_sll_epi64(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        else if constexpr (s_IsSize<4>)
            return Wide{_mm_sll_epi32(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        else if constexpr (s_IsSize<2>)
            return Wide{_mm_sll_epi16(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        else if constexpr (s_IsSize<1>)
            return Wide{_mm_sll_epi8(p_Left.m_Data, p_Shift)};
    }
    friend constexpr Wide operator&(const Wide &p_Left, const Wide &p_Right)
        requires(Integer<T>)
    {
        return Wide{_mm_and_si128(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator|(const Wide &p_Left, const Wide &p_Right)
        requires(Integer<T>)
    {
        return Wide{_mm_or_si128(p_Left.m_Data, p_Right.m_Data)};
    }

    CREATE_SELF_OP(+)
    CREATE_SELF_OP(-)
    CREATE_SELF_OP(*)
    CREATE_SELF_OP(/)

    CREATE_SELF_OP(>>)
    CREATE_SELF_OP(<<)
    CREATE_SELF_OP(&)
    CREATE_SELF_OP(|)

#    define CREATE_CMP_OP(p_Op, p_Flag, p_IntOpName)                                                                   \
        friend constexpr Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            if constexpr (s_Equals<__m128>)                                                                            \
                return _mm_cmp_ps(p_Left.m_Data, p_Right.m_Data, p_Flag);                                              \
            else if constexpr (s_Equals<__m128d>)                                                                      \
                return _mm_cmp_pd(p_Left.m_Data, p_Right.m_Data, p_Flag);                                              \
            else if constexpr (s_Equals<__m128i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return _mm_cmp##p_IntOpName##_epi64(p_Left.m_Data, p_Right.m_Data);                                \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return _mm_cmp##p_IntOpName##_epi32(p_Left.m_Data, p_Right.m_Data);                                \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return _mm_cmp##p_IntOpName##_epi16(p_Left.m_Data, p_Right.m_Data);                                \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return _mm_cmp##p_IntOpName##_epi8(p_Left.m_Data, p_Right.m_Data);                                 \
                CREATE_BAD_BRANCH()                                                                                    \
            }                                                                                                          \
        }

    CREATE_CMP_OP(==, _CMP_EQ_OQ, eq)
    CREATE_CMP_OP(!=, _CMP_NEQ_UQ, neq)
    CREATE_CMP_OP(<, _CMP_LT_OQ, lt)
    CREATE_CMP_OP(>, _CMP_GT_OQ, gt)
    CREATE_CMP_OP(<=, _CMP_LE_OQ, le)
    CREATE_CMP_OP(>=, _CMP_GE_OQ, ge)

    static T Reduce(const Wide &p_Wide)
    {
        if constexpr (s_Equals<__m128>)
        {
            __m128 shift = _mm_movehl_ps(p_Wide.m_Data, p_Wide.m_Data);
            __m128 sum = _mm_add_ps(p_Wide.m_Data, shift);
            shift = _mm_shuffle_ps(sum, sum, 0x55);
            sum = _mm_add_ss(sum, shift);
            return _mm_cvtss_f32(sum);
        }
        else if constexpr (s_Equals<__m128d>)
        {
            __m128d shift = _mm_unpackhi_pd(p_Wide.m_Data, p_Wide.m_Data);
            const __m128d sum = _mm_add_sd(p_Wide.m_Data, shift);
            return _mm_cvtsd_f64(sum);
        }
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i tmp = _mm_srli_si128(p_Wide.m_Data, 8);
                const __m128i sum = _mm_add_epi64(p_Wide.m_Data, tmp);

                return _mm_cvtsi128_si64(sum);
            }
            else if constexpr (s_IsSize<4>)
            {
                __m128i tmp = _mm_srli_si128(p_Wide.m_Data, 4);
                __m128i sum = _mm_add_epi32(p_Wide.m_Data, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi32(sum, tmp);

                return _mm_cvtsi128_si32(sum);
            }
            else if constexpr (s_IsSize<2>)
            {
                __m128i tmp = _mm_srli_si128(p_Wide.m_Data, 2);
                __m128i sum = _mm_add_epi16(p_Wide.m_Data, tmp);
                tmp = _mm_srli_si128(sum, 4);
                sum = _mm_add_epi16(sum, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi16(sum, tmp);

                return static_cast<T>(_mm_cvtsi128_si32(sum));
            }
            else if constexpr (s_IsSize<1>)
            {
                __m128i tmp = _mm_srli_si128(p_Wide.m_Data, 1);
                __m128i sum = _mm_add_epi8(p_Wide.m_Data, tmp);
                tmp = _mm_srli_si128(sum, 2);
                sum = _mm_add_epi8(sum, tmp);
                tmp = _mm_srli_si128(sum, 4);
                sum = _mm_add_epi8(sum, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi8(sum, tmp);

                return static_cast<T>(_mm_cvtsi128_si32(sum));
            }
        }
    }

  private:
    static m128 loadAligned(const T *p_Data)
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][SSE] Data must be aligned to {} bytes to use the SSE SIMD set", Alignment);

        if constexpr (s_Equals<__m128>)
            return _mm_load_ps(p_Data);
        else if constexpr (s_Equals<__m128d>)
            return _mm_load_pd(p_Data);
        else if constexpr (s_Equals<__m128i>)
            return _mm_load_si128(reinterpret_cast<const __m128i *>(p_Data));
        CREATE_BAD_BRANCH()
    }
    static m128 loadUnaligned(const T *p_Data)
    {
        if constexpr (s_Equals<__m128>)
            return _mm_loadu_ps(p_Data);
        else if constexpr (s_Equals<__m128d>)
            return _mm_loadu_pd(p_Data);
        else if constexpr (s_Equals<__m128i>)
            return _mm_loadu_si128(reinterpret_cast<const __m128i *>(p_Data));
        CREATE_BAD_BRANCH()
    }

    static m128 set(const T p_Data)
    {
        if constexpr (s_Equals<__m128>)
            return _mm_set1_ps(p_Data);
        else if constexpr (s_Equals<__m128d>)
            return _mm_set1_pd(p_Data);
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm_set1_epi64x(p_Data);
            else if constexpr (s_IsSize<4>)
                return _mm_set1_epi32(p_Data);
            else if constexpr (s_IsSize<2>)
                return _mm_set1_epi16(p_Data);
            else if constexpr (s_IsSize<1>)
                return _mm_set1_epi8(p_Data);
            CREATE_BAD_BRANCH()
        }
        CREATE_BAD_BRANCH()
    }
    static m128 gather(const T *p_Data, const SizeType p_Stride)
    {
#    ifndef TKIT_SIMD_AVX2
        alignas(Alignment) T dst[Lanes];
        const std::byte *src = reinterpret_cast<const std::byte *>(p_Data);

        for (SizeType i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(dst + i, src + i * p_Stride, sizeof(T));
        return loadAligned(dst);
#    else
        const i32 idx = static_cast<i32>(p_Stride);
        if constexpr (s_Equals<__m128>)
        {
            const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
            return _mm_i32gather_ps(p_Data, indices, 1);
        }
        else if constexpr (s_Equals<__m128d>)
        {
            const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
            return _mm_i32gather_pd(p_Data, indices, 1);
        }
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
                return _mm_i32gather_epi64(reinterpret_cast<const long long int *>(p_Data), indices, 1);
            }
            else if constexpr (s_IsSize<4>)
            {
                const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
                return _mm_i32gather_epi32(reinterpret_cast<const i32 *>(p_Data), indices, 1);
            }
            else
            {
                alignas(Alignment) T dst[Lanes];
                const std::byte *src = reinterpret_cast<const std::byte *>(p_Data);

                for (SizeType i = 0; i < Lanes; ++i)
                    Memory::ForwardCopy(dst + i, src + i * p_Stride, sizeof(T));
                return loadAligned(dst);
            }
        }
        CREATE_BAD_BRANCH()
#    endif
    }

    template <typename Callable, std::size_t... I>
    static constexpr m128 makeIntrinsic(Callable &&p_Callable, std::index_sequence<I...>) noexcept
    {
        if constexpr (s_Equals<__m128>)
            return _mm_setr_ps(static_cast<T>(p_Callable(I))...);
        else if constexpr (s_Equals<__m128d>)
            return _mm_setr_pd(static_cast<T>(p_Callable(I))...);
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm_set_epi64x(static_cast<T>(p_Callable(Lanes - I - 1))...);
            else if constexpr (s_IsSize<4>)
                return _mm_setr_epi32(static_cast<T>(p_Callable(I))...);
            else if constexpr (s_IsSize<2>)
                return _mm_setr_epi16(static_cast<T>(p_Callable(I))...);
            else if constexpr (s_IsSize<1>)
                return _mm_setr_epi8(static_cast<T>(p_Callable(I))...);
        }
        CREATE_BAD_BRANCH()
    }

    static constexpr __m128i _mm_srai_epi64_32(const __m128i p_Data)
    {
        const __m128i shifted = _mm_srli_epi64(p_Data, 32);

        __m128i sign = _mm_srai_epi32(_mm_shuffle_epi32(p_Data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm_slli_epi64(sign, 32);

        return _mm_or_si128(shifted, sign);
    };
    static constexpr __m128i _mm_sra_epi64(const __m128i p_Data, const i32 p_Shift)
    {
        const __m128i shifted = _mm_srl_epi64(p_Data, _mm_cvtsi32_si128(p_Shift));

        __m128i sign = _mm_srai_epi32(_mm_shuffle_epi32(p_Data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm_sll_epi64(sign, _mm_cvtsi32_si128(64 - p_Shift));

        return _mm_or_si128(shifted, sign);
    };
    static constexpr __m128i _mm_srl_epi8(const __m128i p_Data, const i32 p_Shift)
    {
        const __m128i shifted = _mm_srl_epi16(p_Data, _mm_cvtsi32_si128(p_Shift));

        const __m128i mask1 = _mm_set1_epi16(static_cast<i16>(0x00FF >> p_Shift));
        const __m128i mask2 = _mm_set1_epi16(static_cast<i16>(0xFF00));

        const __m128i lo = _mm_and_si128(shifted, mask1);
        const __m128i hi = _mm_and_si128(shifted, mask2);
        return _mm_or_si128(lo, hi);
    }
    static constexpr __m128i _mm_sll_epi8(const __m128i p_Data, const i32 p_Shift)
    {
        const __m128i shifted = _mm_sll_epi16(p_Data, _mm_cvtsi32_si128(p_Shift));

        const __m128i mask1 = _mm_set1_epi16(static_cast<i16>(0xFF00 << p_Shift));
        const __m128i mask2 = _mm_set1_epi16(static_cast<i16>(0x00FF));

        const __m128i lo = _mm_and_si128(shifted, mask1);
        const __m128i hi = _mm_and_si128(shifted, mask2);
        return _mm_or_si128(lo, hi);
    }
    static constexpr __m128i _mm_sra_epi8(const __m128i p_Data, const i32 p_Shift)
    {
        const __m128i shifted = _mm_srl_epi8(p_Data, p_Shift);
        const __m128i signmask = _mm_cmpgt_epi8(_mm_setzero_si128(), p_Data);
        const __m128i mask = _mm_sll_epi8(signmask, 8 - p_Shift);

        return _mm_or_si128(shifted, mask);
    }
    static constexpr __m128i _mm_mullo_epi64(const __m128i p_Left, const __m128i p_Right)
    {
        const __m128i mask32 = _mm_set1_epi64x(0xFFFFFFFF);

        const __m128i llo = _mm_and_si128(p_Left, mask32);
        const __m128i rlo = _mm_and_si128(p_Right, mask32);

        __m128i lhi;
        __m128i rhi;
        if constexpr (!std::is_signed_v<T>)
        {
            lhi = _mm_srli_epi64(p_Left, 32);
            rhi = _mm_srli_epi64(p_Right, 32);
        }
        else
        {
            lhi = _mm_srai_epi64_32(p_Left);
            rhi = _mm_srai_epi64_32(p_Right);
        }

        const __m128i lo = _mm_mul_epu32(llo, rlo);
        const __m128i mid1 = _mm_mullo_epi32(lhi, rlo);
        const __m128i mid2 = _mm_mullo_epi32(llo, rhi);

        __m128i mid = _mm_add_epi64(mid1, mid2);
        mid = _mm_slli_epi64(mid, 32);

        return _mm_add_epi64(lo, mid);
    }
#    ifndef TKIT_SIMD_SSE4_1
    static constexpr __m128i _mm_mullo_epi32(const __m128i p_Left, const __m128i p_Right)
    {
        const __m128i tmp1 = _mm_mul_epu32(p_Left, p_Right);
        const __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(p_Left, 4), _mm_srli_si128(p_Right, 4));

        return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)),
                                  _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
    }
#    endif
    static constexpr __m128i _mm_mullo_epi8(const __m128i p_Left, const __m128i p_Right)
    {
        const __m128i zero = _mm_setzero_si128();
        const __m128i llo = _mm_unpacklo_epi8(p_Left, zero);
        const __m128i lhi = _mm_unpackhi_epi8(p_Left, zero);

        const __m128i rlo = _mm_unpacklo_epi8(p_Right, zero);
        const __m128i rhi = _mm_unpackhi_epi8(p_Right, zero);

        const __m128i plo = _mm_mullo_epi16(llo, rlo);
        const __m128i phi = _mm_mullo_epi16(lhi, rhi);

        const __m128i mask = _mm_set1_epi16(0x00FF);
        return _mm_packus_epi16(_mm_and_si128(plo, mask), _mm_and_si128(phi, mask));
    }
    static constexpr __m128i _mm_min_epi64(const __m128i p_Left, const __m128i p_Right)
    {
        const __m128i cmp = _mm_cmpgt_epi64(p_Left, p_Right);
        return _mm_blendv_epi8(p_Left, p_Right, cmp);
    }
    static constexpr __m128i _mm_max_epi64(const __m128i p_Left, const __m128i p_Right)
    {
        const __m128i cmp = _mm_cmpgt_epi64(p_Left, p_Right);
        return _mm_blendv_epi8(p_Right, p_Left, cmp);
    }

    static constexpr __m128i _mm_cmpgt_epi64(__m128i p_Left, __m128i p_Right)
    {
#    ifdef TKIT_SIMD_SSE4_2
        __m128i sign;
        if constexpr (!std::is_signed_v<T>)
            sign = _mm_set1_epi64x(static_cast<i64>(1ull << 63 | 1ull << 31));
        else
            sign = _mm_set1_epi64x(static_cast<i64>(1ull << 31));

        p_Left = _mm_xor_si128(p_Left, sign);
        p_Right = _mm_xor_si128(p_Right, sign);

        const __m128i lhi = _mm_srli_epi64(p_Left, 32);
        const __m128i rhi = _mm_srli_epi64(p_Right, 32);

        const __m128i gthi = ::_mm_cmpgt_epi32(lhi, rhi);
        const __m128i eqhi = _mm_cmpeq_epi32(lhi, rhi);

        const __m128i gtlo = ::_mm_cmpgt_epi32(p_Left, p_Right);
        const __m128i result = _mm_or_si128(gthi, _mm_and_si128(eqhi, gtlo));
        return _mm_shuffle_epi32(result, _MM_SHUFFLE(2, 2, 0, 0));
#    else
        if constexpr (!std::is_signed_v<T>)
        {
            const __m128i sign = _mm_set1_epi64x(static_cast<i64>(1ull << 63));
            p_Left = _mm_xor_si128(p_Left, sign);
            p_Right = _mm_xor_si128(p_Right, sign);
        }
        return ::_mm_cmpgt_epi64(p_Left, p_Right);
#    endif
    }
    static constexpr __m128i _mm_cmpgt_epi32(const __m128i p_Left, const __m128i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmpgt_epi32(p_Left, p_Right);
        else
        {
            const __m128i offset = _mm_set1_epi32(static_cast<i32>(1 << 31));
            return ::_mm_cmpgt_epi32(_mm_xor_si128(p_Left, offset), _mm_xor_si128(p_Right, offset));
        }
    }
    static constexpr __m128i _mm_cmpgt_epi16(const __m128i p_Left, const __m128i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmpgt_epi16(p_Left, p_Right);
        else
        {
            const __m128i offset = _mm_set1_epi16(static_cast<i16>(1 << 15));
            return ::_mm_cmpgt_epi16(_mm_xor_si128(p_Left, offset), _mm_xor_si128(p_Right, offset));
        }
    }
    static constexpr __m128i _mm_cmpgt_epi8(const __m128i p_Left, const __m128i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmpgt_epi8(p_Left, p_Right);
        else
        {
            const __m128i offset = _mm_set1_epi8(static_cast<i8>(1 << 7));
            return ::_mm_cmpgt_epi8(_mm_xor_si128(p_Left, offset), _mm_xor_si128(p_Right, offset));
        }
    }
    static constexpr __m128i _mm_cmplt_epi64(const __m128i p_Left, const __m128i p_Right)
    {
        return _mm_cmpgt_epi64(p_Right, p_Left);
    }
    static constexpr __m128i _mm_cmplt_epi32(const __m128i p_Left, const __m128i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmplt_epi32(p_Left, p_Right);
        else
        {
            const __m128i offset = _mm_set1_epi32(static_cast<i32>(1 << 31));
            return ::_mm_cmplt_epi32(_mm_xor_si128(p_Left, offset), _mm_xor_si128(p_Right, offset));
        }
    }
    static constexpr __m128i _mm_cmplt_epi16(const __m128i p_Left, const __m128i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmplt_epi16(p_Left, p_Right);
        else
        {
            const __m128i offset = _mm_set1_epi16(static_cast<i16>(1 << 15));
            return ::_mm_cmplt_epi16(_mm_xor_si128(p_Left, offset), _mm_xor_si128(p_Right, offset));
        }
    }
    static constexpr __m128i _mm_cmplt_epi8(const __m128i p_Left, const __m128i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmplt_epi8(p_Left, p_Right);
        else
        {
            const __m128i offset = _mm_set1_epi8(static_cast<i8>(1 << 7));
            return ::_mm_cmplt_epi8(_mm_xor_si128(p_Left, offset), _mm_xor_si128(p_Right, offset));
        }
    }
#    define CREATE_INT_CMP(p_Type, p_Name)                                                                             \
        static constexpr __m128i _mm_cmpneq_ep##p_Type(const __m128i p_Left, const __m128i p_Right)                    \
        {                                                                                                              \
            return _mm_xor_si128(_mm_cmpeq_ep##p_Type(p_Left, p_Right), _mm_set1_ep##p_Name(static_cast<p_Type>(-1))); \
        }                                                                                                              \
        static constexpr __m128i _mm_cmpge_ep##p_Type(const __m128i p_Left, const __m128i p_Right)                     \
        {                                                                                                              \
            return _mm_xor_si128(_mm_cmplt_ep##p_Type(p_Left, p_Right), _mm_set1_ep##p_Name(static_cast<p_Type>(-1))); \
        }                                                                                                              \
        static constexpr __m128i _mm_cmple_ep##p_Type(const __m128i p_Left, const __m128i p_Right)                     \
        {                                                                                                              \
            return _mm_xor_si128(_mm_cmpgt_ep##p_Type(p_Left, p_Right), _mm_set1_ep##p_Name(static_cast<p_Type>(-1))); \
        }

    CREATE_INT_CMP(i8, i8)
    CREATE_INT_CMP(i16, i16)
    CREATE_INT_CMP(i32, i32)
    CREATE_INT_CMP(i64, i64x)

    m128 m_Data;
};
} // namespace TKit::Detail::SSE
TKIT_COMPILER_WARNING_IGNORE_POP()
#endif

#undef CREATE_ARITHMETIC_OP
#undef CREATE_CMP_OP
#undef CREATE_MIN_MAX
#undef CREATE_BAD_BRANCH
#undef CREATE_EQ_CMP
#undef CREATE_INT_CMP
#undef CREATE_SELF_OP
