#pragma once

#include "tkit/preprocessor/system.hpp"
#ifdef TKIT_SIMD_AVX
#    include "tkit/memory/memory.hpp"
#    include "tkit/simd/utils.hpp"
#    include "tkit/container/container.hpp"
#    include <immintrin.h>

namespace TKit::Detail::AVX
{
#    ifdef TKIT_SIMD_AVX2
template <typename T>
concept Arithmetic = Float<T> || Integer<T>;

#    else
template <typename T>
concept Arithmetic = Float<T>;
#    endif

template <Arithmetic T> struct TypeSelector;

template <> struct TypeSelector<f32>
{
    using Type = __m256;
};
template <> struct TypeSelector<f64>
{
    using Type = __m256d;
};

#    ifdef TKIT_SIMD_AVX2
template <Integer T> struct TypeSelector<T>
{
    using Type = __m256i;
};
#    endif

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wignored-attributes")
TKIT_CLANG_WARNING_IGNORE("-Wignored-attributes")
template <Arithmetic T, typename Traits = Container::ArrayTraits<T>> class Wide
{
    using m256 = TypeSelector<T>::Type;
    template <typename E> static constexpr bool s_Equals = std::is_same_v<m256, E>;
    template <usize Size> static constexpr bool s_IsSize = Size == sizeof(T);

  public:
    using ValueType = T;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = TKIT_SIMD_AVX_SIZE / sizeof(T);
    static constexpr SizeType Alignment = 32;

    using Mask = m256;
    using BitMask = u<MaskSize<Lanes>()>;

  public:
#    define CREATE_BAD_BRANCH()                                                                                        \
        else                                                                                                           \
        {                                                                                                              \
            TKIT_UNREACHABLE();                                                                                        \
        }

    static constexpr BitMask PackMask(const Mask &p_Mask)
    {
        if constexpr (s_Equals<__m256>)
            return static_cast<BitMask>(_mm256_movemask_ps(p_Mask));
        else if constexpr (s_Equals<__m256d>)
            return static_cast<BitMask>(_mm256_movemask_pd(p_Mask));
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
            const u32 byteMask = static_cast<u32>(_mm256_movemask_epi8(p_Mask));
            if constexpr (s_IsSize<1>)
                return static_cast<BitMask>(byteMask);
#        ifdef TKIT_BMI2
            else if constexpr (s_IsSize<2>)
                return static_cast<BitMask>(_pext_u32(byteMask, 0x55555555u));
            else if constexpr (s_IsSize<4>)
                return static_cast<BitMask>(_pext_u32(byteMask, 0x11111111u));
            else if constexpr (s_IsSize<8>)
                return static_cast<BitMask>(_pext_u32(byteMask, 0x01010101u));
#        endif
            else
            {
                BitMask packed = 0;
                for (SizeType i = 0; i < Lanes; ++i)
                    packed |= ((byteMask >> (i * sizeof(T))) & 1u) << i;

                return packed;
            }
        }
#    endif
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
    constexpr Wide(const m256 p_Data) : m_Data(p_Data)
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
                    "[TOOLKIT][AVX] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);
        if constexpr (s_Equals<__m256>)
            _mm256_store_ps(p_Data, m_Data);
        else if constexpr (s_Equals<__m256d>)
            _mm256_store_pd(p_Data, m_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            _mm256_store_si256(reinterpret_cast<__m256i *>(p_Data), m_Data);
#    endif
        CREATE_BAD_BRANCH()
    }
    constexpr void StoreUnaligned(T *p_Data) const
    {
        if constexpr (s_Equals<__m256>)
            _mm256_storeu_ps(p_Data, m_Data);
        else if constexpr (s_Equals<__m256d>)
            _mm256_storeu_pd(p_Data, m_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(p_Data), m_Data);
#    endif
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
        if constexpr (s_Equals<__m256>)
            return Wide{_mm256_blendv_ps(p_Right.m_Data, p_Left.m_Data, p_Mask)};
        else if constexpr (s_Equals<__m256d>)
            return Wide{_mm256_blendv_pd(p_Right.m_Data, p_Left.m_Data, p_Mask)};
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            return Wide{_mm256_blendv_epi8(p_Right.m_Data, p_Left.m_Data, p_Mask)};
#    endif
        CREATE_BAD_BRANCH()
    }

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_MIN_MAX_INT(p_Op)                                                                               \
            else if constexpr (s_Equals<__m256i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm256_##p_Op##_epi64(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (std::is_signed_v<T>)                                                                \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm256_##p_Op##_epi32(p_Left.m_Data, p_Right.m_Data)};                             \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm256_##p_Op##_epi16(p_Left.m_Data, p_Right.m_Data)};                             \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm256_##p_Op##_epi8(p_Left.m_Data, p_Right.m_Data)};                              \
                }                                                                                                      \
                else                                                                                                   \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm256_##p_Op##_epu32(p_Left.m_Data, p_Right.m_Data)};                             \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm256_##p_Op##_epu16(p_Left.m_Data, p_Right.m_Data)};                             \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm256_##p_Op##_epu8(p_Left.m_Data, p_Right.m_Data)};                              \
                }                                                                                                      \
            }
#    else
#        define CREATE_MIN_MAX_INT(p_Op)
#    endif

#    define CREATE_MIN_MAX(p_Name, p_Op)                                                                               \
        static constexpr Wide p_Name(const Wide &p_Left, const Wide &p_Right)                                          \
        {                                                                                                              \
            if constexpr (s_Equals<__m256>)                                                                            \
                return Wide{_mm256_##p_Op##_ps(p_Left.m_Data, p_Right.m_Data)};                                        \
            else if constexpr (s_Equals<__m256d>)                                                                      \
                return Wide{_mm256_##p_Op##_pd(p_Left.m_Data, p_Right.m_Data)};                                        \
            CREATE_MIN_MAX_INT(p_Op)                                                                                   \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_MIN_MAX(Min, min)
    CREATE_MIN_MAX(Max, max)

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_ARITHMETIC_OP_INT(p_Op)                                                                         \
            else if constexpr (s_Equals<__m256i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm256_##p_Op##_epi64(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return Wide{_mm256_##p_Op##_epi32(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return Wide{_mm256_##p_Op##_epi16(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return Wide{_mm256_##p_Op##_epi8(p_Left.m_Data, p_Right.m_Data)};                                  \
                CREATE_BAD_BRANCH()                                                                                    \
            }
#    else
#        define CREATE_ARITHMETIC_OP_INT(p_Op)
#    endif

#    define CREATE_ARITHMETIC_OP(p_Op, p_FloatOp, p_IntOp)                                                             \
        friend constexpr Wide operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            if constexpr (s_Equals<__m256>)                                                                            \
                return Wide{_mm256_##p_FloatOp##_ps(p_Left.m_Data, p_Right.m_Data)};                                   \
            else if constexpr (s_Equals<__m256d>)                                                                      \
                return Wide{_mm256_##p_FloatOp##_pd(p_Left.m_Data, p_Right.m_Data)};                                   \
            CREATE_ARITHMETIC_OP_INT(p_IntOp)                                                                          \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_ARITHMETIC_OP(+, add, add)
    CREATE_ARITHMETIC_OP(-, sub, sub)
    CREATE_ARITHMETIC_OP(*, mul, mullo)

    friend constexpr Wide operator/(const Wide &p_Left, const Wide &p_Right)
    {
        if constexpr (s_Equals<__m256>)
            return Wide{_mm256_div_ps(p_Left.m_Data, p_Right.m_Data)};
        else if constexpr (s_Equals<__m256d>)
            return Wide{_mm256_div_pd(p_Left.m_Data, p_Right.m_Data)};
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
#        ifdef TKIT_ALLOW_SCALAR_SIMD_FALLBACKS
            alignas(Alignment) T left[Lanes];
            alignas(Alignment) T right[Lanes];
            alignas(Alignment) T result[Lanes];

            p_Left.StoreAligned(left);
            p_Right.StoreAligned(right);
            for (SizeType i = 0; i < Lanes; ++i)
                result[i] = left[i] / right[i];

            return Wide{result};
#        else
            static_assert(
                !s_Equals<__m256i>,
                "[TOOLKIT][SIMD] AVX does not support integer division. Scalar fallback is disabled by default. "
                "If you really need it, enable TKIT_ALLOW_SCALAR_SIMD_FALLBACKS.");
#        endif
        }
#    endif
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

#    ifdef TKIT_SIMD_AVX2
    friend constexpr Wide operator>>(const Wide &p_Left, const i32 p_Shift)
        requires(Integer<T>)
    {
        if constexpr (s_IsSize<8>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi64(p_Left.m_Data, p_Shift)};
            else
                return Wide{_mm256_srl_epi64(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        }
        else if constexpr (s_IsSize<4>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi32(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
            else
                return Wide{_mm256_srl_epi32(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        }
        else if constexpr (s_IsSize<2>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi16(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
            else
                return Wide{_mm256_srl_epi16(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        }
        else if constexpr (s_IsSize<1>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi8(p_Left.m_Data, p_Shift)};
            else
                return Wide{_mm256_srl_epi8(p_Left.m_Data, p_Shift)};
        }
    }
    friend constexpr Wide operator<<(const Wide &p_Left, const i32 p_Shift)
        requires(Integer<T>)
    {
        if constexpr (s_IsSize<8>)
            return Wide{_mm256_sll_epi64(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        else if constexpr (s_IsSize<4>)
            return Wide{_mm256_sll_epi32(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        else if constexpr (s_IsSize<2>)
            return Wide{_mm256_sll_epi16(p_Left.m_Data, _mm_cvtsi32_si128(p_Shift))};
        else if constexpr (s_IsSize<1>)
            return Wide{_mm256_sll_epi8(p_Left.m_Data, p_Shift)};
    }
    friend constexpr Wide operator&(const Wide &p_Left, const Wide &p_Right)
        requires(Integer<T>)
    {
        return Wide{_mm256_and_si256(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator|(const Wide &p_Left, const Wide &p_Right)
        requires(Integer<T>)
    {
        return Wide{_mm256_or_si256(p_Left.m_Data, p_Right.m_Data)};
    }

    CREATE_SELF_OP(>>)
    CREATE_SELF_OP(<<)
    CREATE_SELF_OP(&)
    CREATE_SELF_OP(|)
#    endif

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_CMP_OP_INT(p_Op)                                                                                \
            else if constexpr (s_Equals<__m256i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return _mm256_cmp##p_Op##_epi64(p_Left.m_Data, p_Right.m_Data);                                    \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return _mm256_cmp##p_Op##_epi32(p_Left.m_Data, p_Right.m_Data);                                    \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return _mm256_cmp##p_Op##_epi16(p_Left.m_Data, p_Right.m_Data);                                    \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return _mm256_cmp##p_Op##_epi8(p_Left.m_Data, p_Right.m_Data);                                     \
                CREATE_BAD_BRANCH()                                                                                    \
            }
#    else
#        define CREATE_CMP_OP_INT(p_Op)
#    endif

#    define CREATE_CMP_OP(p_Op, p_Flag, p_IntOpName)                                                                   \
        friend constexpr Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            if constexpr (s_Equals<__m256>)                                                                            \
                return _mm256_cmp_ps(p_Left.m_Data, p_Right.m_Data, p_Flag);                                           \
            else if constexpr (s_Equals<__m256d>)                                                                      \
                return _mm256_cmp_pd(p_Left.m_Data, p_Right.m_Data, p_Flag);                                           \
            CREATE_CMP_OP_INT(p_IntOpName)                                                                             \
        }

    CREATE_CMP_OP(==, _CMP_EQ_OQ, eq)
    CREATE_CMP_OP(!=, _CMP_NEQ_UQ, neq)
    CREATE_CMP_OP(<, _CMP_LT_OQ, lt)
    CREATE_CMP_OP(>, _CMP_GT_OQ, gt)
    CREATE_CMP_OP(<=, _CMP_LE_OQ, le)
    CREATE_CMP_OP(>=, _CMP_GE_OQ, ge)

    static T Reduce(const Wide &p_Wide)
    {
        if constexpr (s_Equals<__m256>)
        {
            const __m128 lo = _mm256_castps256_ps128(p_Wide.m_Data);
            const __m128 hi = _mm256_extractf128_ps(p_Wide.m_Data, 1);
            __m128 sum = _mm_add_ps(lo, hi);
            __m128 shift = _mm_movehl_ps(sum, sum);
            sum = _mm_add_ps(sum, shift);
            shift = _mm_shuffle_ps(sum, sum, 0x55);
            sum = _mm_add_ss(sum, shift);
            return _mm_cvtss_f32(sum);
        }
        else if constexpr (s_Equals<__m256d>)
        {
            const __m128d lo = _mm256_castpd256_pd128(p_Wide.m_Data);
            const __m128d hi = _mm256_extractf128_pd(p_Wide.m_Data, 1);
            __m128d sum = _mm_add_pd(lo, hi);
            __m128d shift = _mm_unpackhi_pd(sum, sum);
            sum = _mm_add_sd(sum, shift);
            return _mm_cvtsd_f64(sum);
        }
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);

                __m128i sum = _mm_add_epi64(lo, hi);
                const __m128i tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi64(sum, tmp);

                return _mm_cvtsi128_si64(sum);
            }
            else if constexpr (s_IsSize<4>)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);
                __m128i sum = _mm_add_epi32(lo, hi);
                __m128i tmp = _mm_srli_si128(sum, 4);
                sum = _mm_add_epi32(sum, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi32(sum, tmp);

                return _mm_cvtsi128_si32(sum);
            }
            else if constexpr (s_IsSize<2>)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);
                __m128i sum = _mm_add_epi16(lo, hi);
                __m128i tmp = _mm_srli_si128(sum, 2);
                sum = _mm_add_epi16(sum, tmp);
                tmp = _mm_srli_si128(sum, 4);
                sum = _mm_add_epi16(sum, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi16(sum, tmp);

                return static_cast<T>(_mm_cvtsi128_si32(sum));
            }
            else if constexpr (s_IsSize<1>)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);

                __m128i sum = _mm_add_epi8(lo, hi);
                __m128i tmp = _mm_srli_si128(sum, 1);
                sum = _mm_add_epi8(sum, tmp);
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
    static m256 loadAligned(const T *p_Data)
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][AVX] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);

        if constexpr (s_Equals<__m256>)
            return _mm256_load_ps(p_Data);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_load_pd(p_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            return _mm256_load_si256(reinterpret_cast<const __m256i *>(p_Data));
#    endif
        CREATE_BAD_BRANCH()
    }

    static m256 loadUnaligned(const T *p_Data)
    {
        if constexpr (s_Equals<__m256>)
            return _mm256_loadu_ps(p_Data);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_loadu_pd(p_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p_Data));
#    endif
        CREATE_BAD_BRANCH()
    }

    static m256 set(const T p_Data)
    {
        if constexpr (s_Equals<__m256>)
            return _mm256_set1_ps(p_Data);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_set1_pd(p_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm256_set1_epi64x(p_Data);
            else if constexpr (s_IsSize<4>)
                return _mm256_set1_epi32(p_Data);
            else if constexpr (s_IsSize<2>)
                return _mm256_set1_epi16(p_Data);
            else if constexpr (s_IsSize<1>)
                return _mm256_set1_epi8(p_Data);
            CREATE_BAD_BRANCH()
        }
#    endif
        CREATE_BAD_BRANCH()
    }

    static m256 gather(const T *p_Data, const SizeType p_Stride)
    {
#    ifndef TKIT_SIMD_AVX2
        alignas(Alignment) T dst[Lanes];
        const std::byte *src = reinterpret_cast<const std::byte *>(p_Data);

        for (SizeType i = 0; i < Lanes; ++i)
            Memory::ForwardCopy(dst + i, src + i * p_Stride, sizeof(T));
        return loadAligned(dst);
#    else
        const i32 idx = static_cast<i32>(p_Stride);
        if constexpr (s_Equals<__m256>)
        {
            const __m256i indices = _mm256_setr_epi32(0, idx, 2 * idx, 3 * idx, 4 * idx, 5 * idx, 6 * idx, 7 * idx);
            return _mm256_i32gather_ps(p_Data, indices, 1);
        }
        else if constexpr (s_Equals<__m256d>)
        {
            const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
            return _mm256_i32gather_pd(p_Data, indices, 1);
        }
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
                return _mm256_i32gather_epi64(p_Data, indices, 1);
            }
            else if constexpr (s_IsSize<4>)
            {
                const __m256i indices = _mm256_setr_epi32(0, idx, 2 * idx, 3 * idx, 4 * idx, 5 * idx, 6 * idx, 7 * idx);
                return _mm256_i32gather_epi32(p_Data, indices, 1);
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
    static constexpr m256 makeIntrinsic(Callable &&p_Callable, std::index_sequence<I...>) noexcept
    {
        if constexpr (s_Equals<__m256>)
            return _mm256_setr_ps(static_cast<T>(p_Callable(I))...);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_setr_pd(static_cast<T>(p_Callable(I))...);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm256_setr_epi64x(static_cast<T>(p_Callable(I))...);
            else if constexpr (s_IsSize<4>)
                return _mm256_setr_epi32(static_cast<T>(p_Callable(I))...);
            else if constexpr (s_IsSize<2>)
                return _mm256_setr_epi16(static_cast<T>(p_Callable(I))...);
            else if constexpr (s_IsSize<1>)
                return _mm256_setr_epi8(static_cast<T>(p_Callable(I))...);
        }
#    endif
        CREATE_BAD_BRANCH()
    }

#    ifdef TKIT_SIMD_AVX2
    static constexpr __m256i _mm256_srai_epi64_32(const __m256i p_Data)
    {
        const __m256i shifted = _mm256_srli_epi64(p_Data, 32);

        __m256i sign = _mm256_srai_epi32(_mm256_shuffle_epi32(p_Data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm256_slli_epi64(sign, 32);

        return _mm256_or_si256(shifted, sign);
    };
    static constexpr __m256i _mm256_sra_epi64(const __m256i p_Data, const i32 p_Shift)
    {
        const __m256i shifted = _mm256_srl_epi64(p_Data, _mm_cvtsi32_si128(p_Shift));

        __m256i sign = _mm256_srai_epi32(_mm256_shuffle_epi32(p_Data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm256_sll_epi64(sign, _mm_cvtsi32_si128(64 - p_Shift));

        return _mm256_or_si256(shifted, sign);
    };
    static constexpr __m256i _mm256_srl_epi8(const __m256i p_Data, const i32 p_Shift)
    {
        const __m256i shifted = _mm256_srl_epi16(p_Data, _mm_cvtsi32_si128(p_Shift));

        const __m256i mask1 = _mm256_set1_epi16(static_cast<i16>(0x00FF >> p_Shift));
        const __m256i mask2 = _mm256_set1_epi16(static_cast<i16>(0xFF00));

        const __m256i lo = _mm256_and_si256(shifted, mask1);
        const __m256i hi = _mm256_and_si256(shifted, mask2);
        return _mm256_or_si256(lo, hi);
    }
    static constexpr __m256i _mm256_sll_epi8(const __m256i p_Data, const i32 p_Shift)
    {
        const __m256i shifted = _mm256_sll_epi16(p_Data, _mm_cvtsi32_si128(p_Shift));

        const __m256i mask1 = _mm256_set1_epi16(static_cast<i16>(0xFF00 << p_Shift));
        const __m256i mask2 = _mm256_set1_epi16(static_cast<i16>(0x00FF));

        const __m256i lo = _mm256_and_si256(shifted, mask1);
        const __m256i hi = _mm256_and_si256(shifted, mask2);
        return _mm256_or_si256(lo, hi);
    }
    static constexpr __m256i _mm256_sra_epi8(const __m256i p_Data, const i32 p_Shift)
    {
        const __m256i shifted = _mm256_srl_epi8(p_Data, p_Shift);
        const __m256i signmask = _mm256_cmpgt_epi8(_mm256_setzero_si256(), p_Data);
        const __m256i mask = _mm256_sll_epi8(signmask, 8 - p_Shift);

        return _mm256_or_si256(shifted, mask);
    }
    static constexpr __m256i _mm256_mullo_epi64(const __m256i p_Left, const __m256i p_Right)
    {
        const __m256i mask32 = _mm256_set1_epi64x(0xFFFFFFFF);

        const __m256i llo = _mm256_and_si256(p_Left, mask32);
        const __m256i rlo = _mm256_and_si256(p_Right, mask32);

        __m256i lhi;
        __m256i rhi;
        if constexpr (!std::is_signed_v<T>)
        {
            lhi = _mm256_srli_epi64(p_Left, 32);
            rhi = _mm256_srli_epi64(p_Right, 32);
        }
        else
        {
            lhi = _mm256_srai_epi64_32(p_Left);
            rhi = _mm256_srai_epi64_32(p_Right);
        }

        const __m256i lo = _mm256_mul_epu32(llo, rlo);
        const __m256i mid1 = _mm256_mullo_epi32(lhi, rlo);
        const __m256i mid2 = _mm256_mullo_epi32(llo, rhi);

        __m256i mid = _mm256_add_epi64(mid1, mid2);
        mid = _mm256_slli_epi64(mid, 32);

        return _mm256_add_epi64(lo, mid);
    }
    static constexpr __m256i _mm256_mullo_epi8(const __m256i p_Left, const __m256i p_Right)
    {
        const __m256i zero = _mm256_setzero_si256();
        const __m256i llo = _mm256_unpacklo_epi8(p_Left, zero);
        const __m256i lhi = _mm256_unpackhi_epi8(p_Left, zero);

        const __m256i rlo = _mm256_unpacklo_epi8(p_Right, zero);
        const __m256i rhi = _mm256_unpackhi_epi8(p_Right, zero);

        const __m256i plo = _mm256_mullo_epi16(llo, rlo);
        const __m256i phi = _mm256_mullo_epi16(lhi, rhi);

        const __m256i mask = _mm256_set1_epi16(0x00FF);
        return _mm256_packus_epi16(_mm256_and_si256(plo, mask), _mm256_and_si256(phi, mask));
    }

    static constexpr __m256i _mm256_min_epi64(const __m256i p_Left, const __m256i p_Right)
    {
        const __m256i cmp = _mm256_cmpgt_epi64(p_Left, p_Right);
        return _mm256_blendv_epi8(p_Left, p_Right, cmp);
    }
    static constexpr __m256i _mm256_max_epi64(const __m256i p_Left, const __m256i p_Right)
    {
        const __m256i cmp = _mm256_cmpgt_epi64(p_Left, p_Right);
        return _mm256_blendv_epi8(p_Right, p_Left, cmp);
    }

    static constexpr __m256i _mm256_cmpgt_epi64(__m256i p_Left, __m256i p_Right)
    {
        if constexpr (!std::is_signed_v<T>)
        {
            const __m256i offset = _mm256_set1_epi64x(static_cast<i64>(1ull << 63));
            p_Left = _mm256_xor_si256(p_Left, offset);
            p_Right = _mm256_xor_si256(p_Right, offset);
        }
        return ::_mm256_cmpgt_epi64(p_Left, p_Right);
    }
    static constexpr __m256i _mm256_cmpgt_epi32(const __m256i p_Left, const __m256i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm256_cmpgt_epi32(p_Left, p_Right);
        else
        {
            const __m256i offset = _mm256_set1_epi32(static_cast<i32>(1 << 31));
            return ::_mm256_cmpgt_epi32(_mm256_xor_si256(p_Left, offset), _mm256_xor_si256(p_Right, offset));
        }
    }
    static constexpr __m256i _mm256_cmpgt_epi16(const __m256i p_Left, const __m256i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm256_cmpgt_epi16(p_Left, p_Right);
        else
        {
            const __m256i offset = _mm256_set1_epi16(static_cast<i16>(1 << 15));
            return ::_mm256_cmpgt_epi16(_mm256_xor_si256(p_Left, offset), _mm256_xor_si256(p_Right, offset));
        }
    }
    static constexpr __m256i _mm256_cmpgt_epi8(const __m256i p_Left, const __m256i p_Right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm256_cmpgt_epi8(p_Left, p_Right);
        else
        {
            const __m256i offset = _mm256_set1_epi8(static_cast<i8>(1 << 7));
            return ::_mm256_cmpgt_epi8(_mm256_xor_si256(p_Left, offset), _mm256_xor_si256(p_Right, offset));
        }
    }
#        define CREATE_INT_CMP(p_Type, p_Name)                                                                         \
            static constexpr __m256i _mm256_cmpneq_ep##p_Type(const __m256i p_Left, const __m256i p_Right)             \
            {                                                                                                          \
                return _mm256_xor_si256(_mm256_cmpeq_ep##p_Type(p_Left, p_Right),                                      \
                                        _mm256_set1_ep##p_Name(static_cast<p_Type>(-1)));                              \
            }                                                                                                          \
            static constexpr __m256i _mm256_cmplt_ep##p_Type(const __m256i p_Left, const __m256i p_Right)              \
            {                                                                                                          \
                return _mm256_cmpgt_ep##p_Type(p_Right, p_Left);                                                       \
            }                                                                                                          \
            static constexpr __m256i _mm256_cmpge_ep##p_Type(const __m256i p_Left, const __m256i p_Right)              \
            {                                                                                                          \
                return _mm256_xor_si256(_mm256_cmplt_ep##p_Type(p_Left, p_Right),                                      \
                                        _mm256_set1_ep##p_Name(static_cast<p_Type>(-1)));                              \
            }                                                                                                          \
            static constexpr __m256i _mm256_cmple_ep##p_Type(const __m256i p_Left, const __m256i p_Right)              \
            {                                                                                                          \
                return _mm256_xor_si256(_mm256_cmpgt_ep##p_Type(p_Left, p_Right),                                      \
                                        _mm256_set1_ep##p_Name(static_cast<p_Type>(-1)));                              \
            }

    CREATE_INT_CMP(i8, i8)
    CREATE_INT_CMP(i16, i16)
    CREATE_INT_CMP(i32, i32)
    CREATE_INT_CMP(i64, i64x)
#    endif

    m256 m_Data;
};
} // namespace TKit::Detail::AVX
TKIT_COMPILER_WARNING_IGNORE_POP()
#endif

#undef CREATE_ARITHMETIC_OP
#undef CREATE_ARITHMETIC_OP_INT
#undef CREATE_CMP_OP
#undef CREATE_CMP_OP_INT
#undef CREATE_MIN_MAX
#undef CREATE_MIN_MAX_INT
#undef CREATE_BAD_BRANCH
#undef CREATE_EQ_CMP
#undef CREATE_INT_CMP
#undef CREATE_SELF_OP
