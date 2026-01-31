#pragma once

#include "tkit/preprocessor/system.hpp"
#ifdef TKIT_SIMD_AVX
#    include "tkit/memory/memory.hpp"
#    include "tkit/simd/utils.hpp"
#    include "tkit/container/fixed_array.hpp"
#    include "tkit/utils/bit.hpp"
#    include <immintrin.h>

namespace TKit::Simd::AVX
{
#    ifdef TKIT_SIMD_AVX2
template <typename T>
concept Valid = Float<T> || Integer<T>;

#    else
template <typename T>
concept Valid = Float<T>;
#    endif

template <Valid T> struct TypeSelector;

template <> struct TypeSelector<f32>
{
    using m256 = __m256;
};
template <> struct TypeSelector<f64>
{
    using m256 = __m256d;
};

#    ifdef TKIT_SIMD_AVX2
template <Integer T> struct TypeSelector<T>
{
    using m256 = __m256i;
};
#    endif

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wignored-attributes")
TKIT_GCC_WARNING_IGNORE("-Wmaybe-uninitialized")
TKIT_CLANG_WARNING_IGNORE("-Wignored-attributes")
template <Valid T> class Wide
{
    using m256 = typename TypeSelector<T>::m256;
    template <typename E> static constexpr bool s_Equals = std::is_same_v<m256, E>;
    template <usize Size> static constexpr bool s_IsSize = Size == sizeof(T);

  public:
    using ValueType = T;

    static constexpr usize Lanes = TKIT_SIMD_AVX_SIZE / sizeof(T);
    static constexpr usize Alignment = 32;

    using Mask = m256;
    using BitMask = u<MaskSize<Lanes>()>;

  public:
#    define CREATE_BAD_BRANCH()                                                                                        \
        else                                                                                                           \
        {                                                                                                              \
            TKIT_UNREACHABLE();                                                                                        \
        }

    constexpr Wide() = default;
    constexpr Wide(const m256 data) : m_Data(data)
    {
    }
    template <std::convertible_to<T> U> constexpr Wide(const U data) : m_Data(set(static_cast<T>(data)))
    {
    }
    constexpr explicit Wide(const T *data) : m_Data(loadAligned(data))
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
        return Wide{loadAligned(data)};
    }
    static constexpr Wide LoadUnaligned(const T *data)
    {
        return Wide{loadUnaligned(data)};
    }
    static constexpr Wide Gather(const T *data, const usize stride)
    {
        TKIT_ASSERT(stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}", stride,
                    sizeof(T));
        TKIT_LOG_WARNING_IF(
            stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous load", stride);

#    ifndef TKIT_SIMD_AVX2
        alignas(Alignment) T dst[Lanes];
        const std::byte *src = reinterpret_cast<const std::byte *>(data);

        for (usize i = 0; i < Lanes; ++i)
            ForwardCopy(dst + i, src + i * stride, sizeof(T));
        return Wide{loadAligned(dst)};
#    else
        const i32 idx = static_cast<i32>(stride);
        if constexpr (s_Equals<__m256>)
        {
            const __m256i indices = _mm256_setr_epi32(0, idx, 2 * idx, 3 * idx, 4 * idx, 5 * idx, 6 * idx, 7 * idx);
            return Wide{_mm256_i32gather_ps(data, indices, 1)};
        }
        else if constexpr (s_Equals<__m256d>)
        {
            const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
            return Wide{_mm256_i32gather_pd(data, indices, 1)};
        }
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
                return Wide{_mm256_i32gather_epi64(reinterpret_cast<const long long int *>(data), indices, 1)};
            }
            else if constexpr (s_IsSize<4>)
            {
                const __m256i indices = _mm256_setr_epi32(0, idx, 2 * idx, 3 * idx, 4 * idx, 5 * idx, 6 * idx, 7 * idx);
                return Wide{_mm256_i32gather_epi32(reinterpret_cast<const i32 *>(data), indices, 1)};
            }
            else
            {
                alignas(Alignment) T dst[Lanes];
                const std::byte *src = reinterpret_cast<const std::byte *>(data);

                for (usize i = 0; i < Lanes; ++i)
                    ForwardCopy(dst + i, src + i * stride, sizeof(T));
                return Wide{loadAligned(dst)};
            }
        }
        CREATE_BAD_BRANCH()
#    endif
    }
    constexpr void Scatter(T *data, const usize stride) const
    {
        TKIT_ASSERT(stride >= sizeof(T), "[TOOLKIT][SIMD] The stride ({}) must be greater than sizeof(T) = {}", stride,
                    sizeof(T));
        TKIT_LOG_WARNING_IF(
            stride == sizeof(T),
            "[TOOLKIT][SIMD] Stride of {} is equal to sizeof(T), which might as well be a contiguous store", stride);
        alignas(Alignment) T src[Lanes];
        StoreAligned(src);

        std::byte *dst = reinterpret_cast<std::byte *>(data);
        for (usize i = 0; i < Lanes; ++i)
            ForwardCopy(dst + i * stride, &src[i], sizeof(T));
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
        TKIT_ASSERT(IsAligned(data, Alignment),
                    "[TOOLKIT][AVX] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);
        if constexpr (s_Equals<__m256>)
            _mm256_store_ps(data, m_Data);
        else if constexpr (s_Equals<__m256d>)
            _mm256_store_pd(data, m_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            _mm256_store_si256(reinterpret_cast<__m256i *>(data), m_Data);
#    endif
        CREATE_BAD_BRANCH()
    }
    constexpr void StoreUnaligned(T *data) const
    {
        if constexpr (s_Equals<__m256>)
            _mm256_storeu_ps(data, m_Data);
        else if constexpr (s_Equals<__m256d>)
            _mm256_storeu_pd(data, m_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(data), m_Data);
#    endif
        CREATE_BAD_BRANCH()
    }

    constexpr const T At(const usize index) const
    {
        TKIT_ASSERT(index < Lanes, "[TOOLKIT][AVX] Index exceeds lane count: {} >= {}", index, Lanes);
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        return tmp[index];
    }
    constexpr const T operator[](const usize index) const
    {
        return At(index);
    }

    static constexpr Wide Select(const Wide &left, const Wide &right, const Mask &mask)
    {
        if constexpr (s_Equals<__m256>)
            return Wide{_mm256_blendv_ps(right.m_Data, left.m_Data, mask)};
        else if constexpr (s_Equals<__m256d>)
            return Wide{_mm256_blendv_pd(right.m_Data, left.m_Data, mask)};
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            return Wide{_mm256_blendv_epi8(right.m_Data, left.m_Data, mask)};
#    endif
        CREATE_BAD_BRANCH()
    }

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_MIN_MAX_INT(op)                                                                                 \
            else if constexpr (s_Equals<__m256i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm256_##op##_epi64(left.m_Data, right.m_Data)};                                       \
                else if constexpr (std::is_signed_v<T>)                                                                \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm256_##op##_epi32(left.m_Data, right.m_Data)};                                   \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm256_##op##_epi16(left.m_Data, right.m_Data)};                                   \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm256_##op##_epi8(left.m_Data, right.m_Data)};                                    \
                }                                                                                                      \
                else                                                                                                   \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm256_##op##_epu32(left.m_Data, right.m_Data)};                                   \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm256_##op##_epu16(left.m_Data, right.m_Data)};                                   \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm256_##op##_epu8(left.m_Data, right.m_Data)};                                    \
                }                                                                                                      \
            }
#    else
#        define CREATE_MIN_MAX_INT(op)
#    endif

#    define CREATE_MIN_MAX(name, op)                                                                                   \
        static constexpr Wide name(const Wide &left, const Wide &right)                                                \
        {                                                                                                              \
            if constexpr (s_Equals<__m256>)                                                                            \
                return Wide{_mm256_##op##_ps(left.m_Data, right.m_Data)};                                              \
            else if constexpr (s_Equals<__m256d>)                                                                      \
                return Wide{_mm256_##op##_pd(left.m_Data, right.m_Data)};                                              \
            CREATE_MIN_MAX_INT(op)                                                                                     \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_MIN_MAX(Min, min)
    CREATE_MIN_MAX(Max, max)

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_ARITHMETIC_OP_INT(op)                                                                           \
            else if constexpr (s_Equals<__m256i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm256_##op##_epi64(left.m_Data, right.m_Data)};                                       \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return Wide{_mm256_##op##_epi32(left.m_Data, right.m_Data)};                                       \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return Wide{_mm256_##op##_epi16(left.m_Data, right.m_Data)};                                       \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return Wide{_mm256_##op##_epi8(left.m_Data, right.m_Data)};                                        \
                CREATE_BAD_BRANCH()                                                                                    \
            }
#    else
#        define CREATE_ARITHMETIC_OP_INT(op)
#    endif

#    define CREATE_ARITHMETIC_OP(op, floatOp, intOp)                                                                   \
        friend constexpr Wide operator op(const Wide &left, const Wide &right)                                         \
        {                                                                                                              \
            if constexpr (s_Equals<__m256>)                                                                            \
                return Wide{_mm256_##floatOp##_ps(left.m_Data, right.m_Data)};                                         \
            else if constexpr (s_Equals<__m256d>)                                                                      \
                return Wide{_mm256_##floatOp##_pd(left.m_Data, right.m_Data)};                                         \
            CREATE_ARITHMETIC_OP_INT(intOp)                                                                            \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_ARITHMETIC_OP(+, add, add)
    CREATE_ARITHMETIC_OP(-, sub, sub)
    CREATE_ARITHMETIC_OP(*, mul, mullo)

    friend constexpr Wide operator/(const Wide &left, const Wide &right)
    {
        if constexpr (s_Equals<__m256>)
            return Wide{_mm256_div_ps(left.m_Data, right.m_Data)};
        else if constexpr (s_Equals<__m256d>)
            return Wide{_mm256_div_pd(left.m_Data, right.m_Data)};
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
#        ifdef TKIT_ALLOW_SCALAR_SIMD_FALLBACKS
            alignas(Alignment) T left[Lanes];
            alignas(Alignment) T right[Lanes];
            alignas(Alignment) T result[Lanes];

            left.StoreAligned(left);
            right.StoreAligned(right);
            for (usize i = 0; i < Lanes; ++i)
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

#    ifdef TKIT_SIMD_AVX2
    template <std::convertible_to<i32> U>
    friend constexpr Wide operator>>(const Wide &left, const U pshift)
        requires(Integer<T>)
    {
        const i32 shift = static_cast<i32>(pshift);
        if constexpr (s_IsSize<8>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi64(left.m_Data, shift)};
            else
                return Wide{_mm256_srl_epi64(left.m_Data, _mm_cvtsi32_si128(shift))};
        }
        else if constexpr (s_IsSize<4>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi32(left.m_Data, _mm_cvtsi32_si128(shift))};
            else
                return Wide{_mm256_srl_epi32(left.m_Data, _mm_cvtsi32_si128(shift))};
        }
        else if constexpr (s_IsSize<2>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi16(left.m_Data, _mm_cvtsi32_si128(shift))};
            else
                return Wide{_mm256_srl_epi16(left.m_Data, _mm_cvtsi32_si128(shift))};
        }
        else if constexpr (s_IsSize<1>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm256_sra_epi8(left.m_Data, shift)};
            else
                return Wide{_mm256_srl_epi8(left.m_Data, shift)};
        }
    }

    template <std::convertible_to<i32> U>
    friend constexpr Wide operator<<(const Wide &left, const U pshift)
        requires(Integer<T>)
    {
        const i32 shift = static_cast<i32>(pshift);
        if constexpr (s_IsSize<8>)
            return Wide{_mm256_sll_epi64(left.m_Data, _mm_cvtsi32_si128(shift))};
        else if constexpr (s_IsSize<4>)
            return Wide{_mm256_sll_epi32(left.m_Data, _mm_cvtsi32_si128(shift))};
        else if constexpr (s_IsSize<2>)
            return Wide{_mm256_sll_epi16(left.m_Data, _mm_cvtsi32_si128(shift))};
        else if constexpr (s_IsSize<1>)
            return Wide{_mm256_sll_epi8(left.m_Data, shift)};
    }
    friend constexpr Wide operator&(const Wide &left, const Wide &right)
        requires(Integer<T>)
    {
        return Wide{_mm256_and_si256(left.m_Data, right.m_Data)};
    }
    friend constexpr Wide operator|(const Wide &left, const Wide &right)
        requires(Integer<T>)
    {
        return Wide{_mm256_or_si256(left.m_Data, right.m_Data)};
    }

    CREATE_SCALAR_OP(&, requires(Integer<T>))
    CREATE_SCALAR_OP(|, requires(Integer<T>))

    CREATE_SELF_OP(>>, requires(Integer<T>))
    CREATE_SELF_OP(<<, requires(Integer<T>))
    CREATE_SELF_OP(&, requires(Integer<T>))
    CREATE_SELF_OP(|, requires(Integer<T>))
#    endif

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_CMP_OP_INT(op)                                                                                  \
            else if constexpr (s_Equals<__m256i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return _mm256_cmp##op##_epi64(left.m_Data, right.m_Data);                                          \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return _mm256_cmp##op##_epi32(left.m_Data, right.m_Data);                                          \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return _mm256_cmp##op##_epi16(left.m_Data, right.m_Data);                                          \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return _mm256_cmp##op##_epi8(left.m_Data, right.m_Data);                                           \
                CREATE_BAD_BRANCH()                                                                                    \
            }
#    else
#        define CREATE_CMP_OP_INT(op)
#    endif

#    define CREATE_CMP_OP(op, flag, intOpName)                                                                         \
        friend constexpr Mask operator op(const Wide &left, const Wide &right)                                         \
        {                                                                                                              \
            if constexpr (s_Equals<__m256>)                                                                            \
                return _mm256_cmp_ps(left.m_Data, right.m_Data, flag);                                                 \
            else if constexpr (s_Equals<__m256d>)                                                                      \
                return _mm256_cmp_pd(left.m_Data, right.m_Data, flag);                                                 \
            CREATE_CMP_OP_INT(intOpName)                                                                               \
        }

    CREATE_CMP_OP(==, _CMP_EQ_OQ, eq)
    CREATE_CMP_OP(!=, _CMP_NEQ_UQ, neq)
    CREATE_CMP_OP(<, _CMP_LT_OQ, lt)
    CREATE_CMP_OP(>, _CMP_GT_OQ, gt)
    CREATE_CMP_OP(<=, _CMP_LE_OQ, le)
    CREATE_CMP_OP(>=, _CMP_GE_OQ, ge)

    static T Reduce(const Wide &wide)
    {
        if constexpr (s_Equals<__m256>)
        {
            const __m128 lo = _mm256_castps256_ps128(wide.m_Data);
            const __m128 hi = _mm256_extractf128_ps(wide.m_Data, 1);
            __m128 sum = _mm_add_ps(lo, hi);
            __m128 shift = _mm_movehl_ps(sum, sum);
            sum = _mm_add_ps(sum, shift);
            shift = _mm_shuffle_ps(sum, sum, 0x55);
            sum = _mm_add_ss(sum, shift);
            return _mm_cvtss_f32(sum);
        }
        else if constexpr (s_Equals<__m256d>)
        {
            const __m128d lo = _mm256_castpd256_pd128(wide.m_Data);
            const __m128d hi = _mm256_extractf128_pd(wide.m_Data, 1);
            __m128d sum = _mm_add_pd(lo, hi);
            __m128d shift = _mm_unpackhi_pd(sum, sum);
            sum = _mm_add_sd(sum, shift);
            return _mm_cvtsd_f64(sum);
        }
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i lo = _mm256_castsi256_si128(wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(wide.m_Data, 1);

                __m128i sum = _mm_add_epi64(lo, hi);
                const __m128i tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi64(sum, tmp);

                return _mm_cvtsi128_si64(sum);
            }
            else if constexpr (s_IsSize<4>)
            {
                const __m128i lo = _mm256_castsi256_si128(wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(wide.m_Data, 1);
                __m128i sum = _mm_add_epi32(lo, hi);
                __m128i tmp = _mm_srli_si128(sum, 4);
                sum = _mm_add_epi32(sum, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi32(sum, tmp);

                return _mm_cvtsi128_si32(sum);
            }
            else if constexpr (s_IsSize<2>)
            {
                const __m128i lo = _mm256_castsi256_si128(wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(wide.m_Data, 1);
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
                const __m128i lo = _mm256_castsi256_si128(wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(wide.m_Data, 1);

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

    static constexpr BitMask PackMask(const Mask &mask)
    {
        if constexpr (s_Equals<__m256>)
            return static_cast<BitMask>(_mm256_movemask_ps(mask));
        else if constexpr (s_Equals<__m256d>)
            return static_cast<BitMask>(_mm256_movemask_pd(mask));
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
            const u32 byteMask = static_cast<u32>(_mm256_movemask_epi8(mask));
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
                for (usize i = 0; i < Lanes; ++i)
                    packed |= ((byteMask >> (i * sizeof(T))) & 1u) << i;

                return packed;
            }
        }
#    endif
        CREATE_BAD_BRANCH()
    }

    static constexpr Mask WidenMask(const BitMask mask)
    {
        using Integer = u<sizeof(T) * 8>;
        alignas(Alignment) Integer tmp[Lanes];

        for (usize i = 0; i < Lanes; ++i)
            tmp[i] = (mask & (BitMask{1} << i)) ? static_cast<Integer>(-1) : Integer{0};

        return loadAligned(reinterpret_cast<const T *>(tmp));
    }

    static constexpr bool AllOf(const Mask &mask)
    {
        return AllOf(PackMask(mask));
    }
    static constexpr bool NoneOf(const Mask &mask)
    {
        return NoneOf(PackMask(mask));
    }
    static constexpr bool AnyOf(const Mask &mask)
    {
        return AnyOf(PackMask(mask));
    }

  private:
    static m256 loadAligned(const T *data)
    {
        TKIT_ASSERT(IsAligned(data, Alignment),
                    "[TOOLKIT][AVX] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);

        if constexpr (s_Equals<__m256>)
            return _mm256_load_ps(data);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_load_pd(data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            return _mm256_load_si256(reinterpret_cast<const __m256i *>(data));
#    endif
        CREATE_BAD_BRANCH()
    }

    static m256 loadUnaligned(const T *data)
    {
        if constexpr (s_Equals<__m256>)
            return _mm256_loadu_ps(data);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_loadu_pd(data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
            return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data));
#    endif
        CREATE_BAD_BRANCH()
    }

    static m256 set(const T data)
    {
        if constexpr (s_Equals<__m256>)
            return _mm256_set1_ps(data);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_set1_pd(data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm256_set1_epi64x(data);
            else if constexpr (s_IsSize<4>)
                return _mm256_set1_epi32(data);
            else if constexpr (s_IsSize<2>)
                return _mm256_set1_epi16(data);
            else if constexpr (s_IsSize<1>)
                return _mm256_set1_epi8(data);
            CREATE_BAD_BRANCH()
        }
#    endif
        CREATE_BAD_BRANCH()
    }

    template <typename Callable, usize... I>
    static constexpr m256 makeIntrinsic(Callable &&callable, std::integer_sequence<usize, I...>)
    {
        if constexpr (s_Equals<__m256>)
            return _mm256_setr_ps(static_cast<T>(std::forward<Callable>(callable)(I))...);
        else if constexpr (s_Equals<__m256d>)
            return _mm256_setr_pd(static_cast<T>(std::forward<Callable>(callable)(I))...);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (s_Equals<__m256i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm256_setr_epi64x(static_cast<T>(std::forward<Callable>(callable)(I))...);
            else if constexpr (s_IsSize<4>)
                return _mm256_setr_epi32(static_cast<T>(std::forward<Callable>(callable)(I))...);
            else if constexpr (s_IsSize<2>)
                return _mm256_setr_epi16(static_cast<T>(std::forward<Callable>(callable)(I))...);
            else if constexpr (s_IsSize<1>)
                return _mm256_setr_epi8(static_cast<T>(std::forward<Callable>(callable)(I))...);
        }
#    endif
        CREATE_BAD_BRANCH()
    }

#    ifdef TKIT_SIMD_AVX2
    static constexpr __m256i _mm256_srai_epi64_32(const __m256i data)
    {
        const __m256i shifted = _mm256_srli_epi64(data, 32);

        __m256i sign = _mm256_srai_epi32(_mm256_shuffle_epi32(data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm256_slli_epi64(sign, 32);

        return _mm256_or_si256(shifted, sign);
    };
    static constexpr __m256i _mm256_sra_epi64(const __m256i data, const i32 shift)
    {
        const __m256i shifted = _mm256_srl_epi64(data, _mm_cvtsi32_si128(shift));

        __m256i sign = _mm256_srai_epi32(_mm256_shuffle_epi32(data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm256_sll_epi64(sign, _mm_cvtsi32_si128(64 - shift));

        return _mm256_or_si256(shifted, sign);
    };
    static constexpr __m256i _mm256_srl_epi8(const __m256i data, const i32 shift)
    {
        const __m256i shifted = _mm256_srl_epi16(data, _mm_cvtsi32_si128(shift));

        const __m256i mask1 = _mm256_set1_epi16(static_cast<i16>(0x00FF >> shift));
        const __m256i mask2 = _mm256_set1_epi16(static_cast<i16>(0xFF00));

        const __m256i lo = _mm256_and_si256(shifted, mask1);
        const __m256i hi = _mm256_and_si256(shifted, mask2);
        return _mm256_or_si256(lo, hi);
    }
    static constexpr __m256i _mm256_sll_epi8(const __m256i data, const i32 shift)
    {
        const __m256i shifted = _mm256_sll_epi16(data, _mm_cvtsi32_si128(shift));

        const __m256i mask1 = _mm256_set1_epi16(static_cast<i16>(0xFF00 << shift));
        const __m256i mask2 = _mm256_set1_epi16(static_cast<i16>(0x00FF));

        const __m256i lo = _mm256_and_si256(shifted, mask1);
        const __m256i hi = _mm256_and_si256(shifted, mask2);
        return _mm256_or_si256(lo, hi);
    }
    static constexpr __m256i _mm256_sra_epi8(const __m256i data, const i32 shift)
    {
        const __m256i shifted = _mm256_srl_epi8(data, shift);
        const __m256i signmask = _mm256_cmpgt_epi8(_mm256_setzero_si256(), data);
        const __m256i mask = _mm256_sll_epi8(signmask, 8 - shift);

        return _mm256_or_si256(shifted, mask);
    }
    static constexpr __m256i _mm256_mullo_epi64(const __m256i left, const __m256i right)
    {
        const __m256i mask32 = _mm256_set1_epi64x(0xFFFFFFFF);

        const __m256i llo = _mm256_and_si256(left, mask32);
        const __m256i rlo = _mm256_and_si256(right, mask32);

        __m256i lhi;
        __m256i rhi;
        if constexpr (!std::is_signed_v<T>)
        {
            lhi = _mm256_srli_epi64(left, 32);
            rhi = _mm256_srli_epi64(right, 32);
        }
        else
        {
            lhi = _mm256_srai_epi64_32(left);
            rhi = _mm256_srai_epi64_32(right);
        }

        const __m256i lo = _mm256_mul_epu32(llo, rlo);
        const __m256i mid1 = _mm256_mullo_epi32(lhi, rlo);
        const __m256i mid2 = _mm256_mullo_epi32(llo, rhi);

        __m256i mid = _mm256_add_epi64(mid1, mid2);
        mid = _mm256_slli_epi64(mid, 32);

        return _mm256_add_epi64(lo, mid);
    }
    static constexpr __m256i _mm256_mullo_epi8(const __m256i left, const __m256i right)
    {
        const __m256i zero = _mm256_setzero_si256();
        const __m256i llo = _mm256_unpacklo_epi8(left, zero);
        const __m256i lhi = _mm256_unpackhi_epi8(left, zero);

        const __m256i rlo = _mm256_unpacklo_epi8(right, zero);
        const __m256i rhi = _mm256_unpackhi_epi8(right, zero);

        const __m256i plo = _mm256_mullo_epi16(llo, rlo);
        const __m256i phi = _mm256_mullo_epi16(lhi, rhi);

        const __m256i mask = _mm256_set1_epi16(0x00FF);
        return _mm256_packus_epi16(_mm256_and_si256(plo, mask), _mm256_and_si256(phi, mask));
    }

    static constexpr __m256i _mm256_min_epi64(const __m256i left, const __m256i right)
    {
        const __m256i cmp = _mm256_cmpgt_epi64(left, right);
        return _mm256_blendv_epi8(left, right, cmp);
    }
    static constexpr __m256i _mm256_max_epi64(const __m256i left, const __m256i right)
    {
        const __m256i cmp = _mm256_cmpgt_epi64(left, right);
        return _mm256_blendv_epi8(right, left, cmp);
    }

    static constexpr __m256i _mm256_cmpgt_epi64(__m256i left, __m256i right)
    {
        if constexpr (!std::is_signed_v<T>)
        {
            const __m256i offset = _mm256_set1_epi64x(static_cast<i64>(1ull << 63));
            left = _mm256_xor_si256(left, offset);
            right = _mm256_xor_si256(right, offset);
        }
        return ::_mm256_cmpgt_epi64(left, right);
    }
    static constexpr __m256i _mm256_cmpgt_epi32(const __m256i left, const __m256i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm256_cmpgt_epi32(left, right);
        else
        {
            const __m256i offset = _mm256_set1_epi32(static_cast<i32>(1 << 31));
            return ::_mm256_cmpgt_epi32(_mm256_xor_si256(left, offset), _mm256_xor_si256(right, offset));
        }
    }
    static constexpr __m256i _mm256_cmpgt_epi16(const __m256i left, const __m256i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm256_cmpgt_epi16(left, right);
        else
        {
            const __m256i offset = _mm256_set1_epi16(static_cast<i16>(1 << 15));
            return ::_mm256_cmpgt_epi16(_mm256_xor_si256(left, offset), _mm256_xor_si256(right, offset));
        }
    }
    static constexpr __m256i _mm256_cmpgt_epi8(const __m256i left, const __m256i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm256_cmpgt_epi8(left, right);
        else
        {
            const __m256i offset = _mm256_set1_epi8(static_cast<i8>(1 << 7));
            return ::_mm256_cmpgt_epi8(_mm256_xor_si256(left, offset), _mm256_xor_si256(right, offset));
        }
    }
#        define CREATE_INT_CMP(type, name)                                                                             \
            static constexpr __m256i _mm256_cmpneq_ep##type(const __m256i left, const __m256i right)                   \
            {                                                                                                          \
                return _mm256_xor_si256(_mm256_cmpeq_ep##type(left, right),                                            \
                                        _mm256_set1_ep##name(static_cast<type>(-1)));                                  \
            }                                                                                                          \
            static constexpr __m256i _mm256_cmplt_ep##type(const __m256i left, const __m256i right)                    \
            {                                                                                                          \
                return _mm256_cmpgt_ep##type(right, left);                                                             \
            }                                                                                                          \
            static constexpr __m256i _mm256_cmpge_ep##type(const __m256i left, const __m256i right)                    \
            {                                                                                                          \
                return _mm256_xor_si256(_mm256_cmplt_ep##type(left, right),                                            \
                                        _mm256_set1_ep##name(static_cast<type>(-1)));                                  \
            }                                                                                                          \
            static constexpr __m256i _mm256_cmple_ep##type(const __m256i left, const __m256i right)                    \
            {                                                                                                          \
                return _mm256_xor_si256(_mm256_cmpgt_ep##type(left, right),                                            \
                                        _mm256_set1_ep##name(static_cast<type>(-1)));                                  \
            }

    CREATE_INT_CMP(i8, i8)
    CREATE_INT_CMP(i16, i16)
    CREATE_INT_CMP(i32, i32)
    CREATE_INT_CMP(i64, i64x)
#    endif

    m256 m_Data;
};
} // namespace TKit::Simd::AVX
TKIT_COMPILER_WARNING_IGNORE_POP()
#    undef CREATE_ARITHMETIC_OP
#    undef CREATE_ARITHMETIC_OP_INT
#    undef CREATE_CMP_OP
#    undef CREATE_CMP_OP_INT
#    undef CREATE_MIN_MAX
#    undef CREATE_MIN_MAX_INT
#    undef CREATE_BAD_BRANCH
#    undef CREATE_EQ_CMP
#    undef CREATE_INT_CMP
#    undef CREATE_SELF_OP
#endif
