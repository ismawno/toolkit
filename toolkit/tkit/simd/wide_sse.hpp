#pragma once

#include "tkit/preprocessor/system.hpp"
#ifdef TKIT_SIMD_SSE2
#    include "tkit/memory/memory.hpp"
#    include "tkit/simd/utils.hpp"
#    include "tkit/container/fixed_array.hpp"
#    include "tkit/utils/bit.hpp"
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
namespace TKit::Simd::SSE
{
template <Numeric T> struct TypeSelector;

template <> struct TypeSelector<f32>
{
    using m128 = __m128;
};
template <> struct TypeSelector<f64>
{
    using m128 = __m128d;
};

template <Integer T> struct TypeSelector<T>
{
    using m128 = __m128i;
};

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_GCC_WARNING_IGNORE("-Wignored-attributes")
TKIT_GCC_WARNING_IGNORE("-Wmaybe-uninitialized")
TKIT_CLANG_WARNING_IGNORE("-Wignored-attributes")
template <Numeric T> class Wide
{
    using m128 = typename TypeSelector<T>::m128;

    template <typename E> static constexpr bool s_Equals = std::is_same_v<m128, E>;
    template <usize Size> static constexpr bool s_IsSize = Size == sizeof(T);

  public:
    using ValueType = T;

    static constexpr usize Lanes = TKIT_SIMD_SSE_SIZE / sizeof(T);
    static constexpr usize Alignment = 16;

    using Mask = m128;
    using BitMask = u<MaskSize<Lanes>()>;

#    define CREATE_BAD_BRANCH()                                                                                        \
        else                                                                                                           \
        {                                                                                                              \
            TKIT_UNREACHABLE();                                                                                        \
        }

    constexpr Wide() = default;
    constexpr Wide(const m128 data) : m_Data(data)
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
        if constexpr (s_Equals<__m128>)
        {
            const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
            return Wide{_mm_i32gather_ps(data, indices, 1)};
        }
        else if constexpr (s_Equals<__m128d>)
        {
            const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
            return Wide{_mm_i32gather_pd(data, indices, 1)};
        }
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
                return Wide{_mm_i32gather_epi64(reinterpret_cast<const long long int *>(data), indices, 1)};
            }
            else if constexpr (s_IsSize<4>)
            {
                const __m128i indices = _mm_setr_epi32(0, idx, 2 * idx, 3 * idx);
                return Wide{_mm_i32gather_epi32(reinterpret_cast<const i32 *>(data), indices, 1)};
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
                    "[TOOLKIT][SSE] Data must be aligned to {} bytes to use the SSE SIMD set", Alignment);
        if constexpr (s_Equals<__m128>)
            _mm_store_ps(data, m_Data);
        else if constexpr (s_Equals<__m128d>)
            _mm_store_pd(data, m_Data);
        else if constexpr (s_Equals<__m128i>)
            _mm_store_si128(reinterpret_cast<__m128i *>(data), m_Data);
        CREATE_BAD_BRANCH()
    }
    constexpr void StoreUnaligned(T *data) const
    {
        if constexpr (s_Equals<__m128>)
            _mm_storeu_ps(data, m_Data);
        else if constexpr (s_Equals<__m128d>)
            _mm_storeu_pd(data, m_Data);
        else if constexpr (s_Equals<__m128i>)
            _mm_storeu_si128(reinterpret_cast<__m128i *>(data), m_Data);
        CREATE_BAD_BRANCH()
    }

    constexpr const T At(const usize index) const
    {
        TKIT_ASSERT(index < Lanes, "[TOOLKIT][SSE] Index exceeds lane count: {} >= {}", index, Lanes);
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        return tmp[index];
    }
    template <usize Index> T At() const
    {
        static_assert(Index < Lanes, "[TOOLKIT][SSE] Index exceeds lane count");
        return At(Index);
    }
    constexpr const T operator[](const usize index) const
    {
        return At(index);
    }

    static constexpr Wide Select(const Wide &left, const Wide &right, const Mask &mask)
    {
        if constexpr (s_Equals<__m128>)
            return Wide{_mm_blendv_ps(right.m_Data, left.m_Data, mask)};
        else if constexpr (s_Equals<__m128d>)
            return Wide{_mm_blendv_pd(right.m_Data, left.m_Data, mask)};
        else if constexpr (s_Equals<__m128i>)
            return Wide{_mm_blendv_epi8(right.m_Data, left.m_Data, mask)};
        CREATE_BAD_BRANCH()
    }

#    define CREATE_MIN_MAX(name, op)                                                                                   \
        static constexpr Wide name(const Wide &left, const Wide &right)                                                \
        {                                                                                                              \
            if constexpr (s_Equals<__m128>)                                                                            \
                return Wide{_mm_##op##_ps(left.m_Data, right.m_Data)};                                                 \
            else if constexpr (s_Equals<__m128d>)                                                                      \
                return Wide{_mm_##op##_pd(left.m_Data, right.m_Data)};                                                 \
            else if constexpr (s_Equals<__m128i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm_##op##_epi64(left.m_Data, right.m_Data)};                                          \
                else if constexpr (std::is_signed_v<T>)                                                                \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm_##op##_epi32(left.m_Data, right.m_Data)};                                      \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm_##op##_epi16(left.m_Data, right.m_Data)};                                      \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm_##op##_epi8(left.m_Data, right.m_Data)};                                       \
                }                                                                                                      \
                else                                                                                                   \
                {                                                                                                      \
                    if constexpr (s_IsSize<4>)                                                                         \
                        return Wide{_mm_##op##_epu32(left.m_Data, right.m_Data)};                                      \
                    else if constexpr (s_IsSize<2>)                                                                    \
                        return Wide{_mm_##op##_epu16(left.m_Data, right.m_Data)};                                      \
                    else if constexpr (s_IsSize<1>)                                                                    \
                        return Wide{_mm_##op##_epu8(left.m_Data, right.m_Data)};                                       \
                }                                                                                                      \
            }                                                                                                          \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_MIN_MAX(Min, min)
    CREATE_MIN_MAX(Max, max)

#    define CREATE_ARITHMETIC_OP(op, floatOp, intOp)                                                                   \
        friend constexpr Wide operator op(const Wide &left, const Wide &right)                                         \
        {                                                                                                              \
            if constexpr (s_Equals<__m128>)                                                                            \
                return Wide{_mm_##floatOp##_ps(left.m_Data, right.m_Data)};                                            \
            else if constexpr (s_Equals<__m128d>)                                                                      \
                return Wide{_mm_##floatOp##_pd(left.m_Data, right.m_Data)};                                            \
            else if constexpr (s_Equals<__m128i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return Wide{_mm_##intOp##_epi64(left.m_Data, right.m_Data)};                                       \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return Wide{_mm_##intOp##_epi32(left.m_Data, right.m_Data)};                                       \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return Wide{_mm_##intOp##_epi16(left.m_Data, right.m_Data)};                                       \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return Wide{_mm_##intOp##_epi8(left.m_Data, right.m_Data)};                                        \
                CREATE_BAD_BRANCH()                                                                                    \
            }                                                                                                          \
            CREATE_BAD_BRANCH()                                                                                        \
        }

    CREATE_ARITHMETIC_OP(+, add, add)
    CREATE_ARITHMETIC_OP(-, sub, sub)
    CREATE_ARITHMETIC_OP(*, mul, mullo)

    friend constexpr Wide operator/(const Wide &left, const Wide &right)
    {
        if constexpr (s_Equals<__m128>)
            return Wide{_mm_div_ps(left.m_Data, right.m_Data)};
        else if constexpr (s_Equals<__m128d>)
            return Wide{_mm_div_pd(left.m_Data, right.m_Data)};
        else if constexpr (s_Equals<__m128i>)
        {
#    ifdef TKIT_ALLOW_SCALAR_SIMD_FALLBACKS
            alignas(Alignment) T left[Lanes];
            alignas(Alignment) T right[Lanes];
            alignas(Alignment) T result[Lanes];

            left.StoreAligned(left);
            right.StoreAligned(right);
            for (usize i = 0; i < Lanes; ++i)
                result[i] = left[i] / right[i];

            return Wide{result};
#    else
            static_assert(!s_Equals<__m128i>, "[TOOLKIT][SIMD] SSE does not support integer division. Scalar "
                                              "fallback is disabled by default. "
                                              "If you really need it, enable TKIT_ALLOW_SCALAR_SIMD_FALLBACKS.");
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

    CREATE_SCALAR_OP(&, requires(Integer<T>))
    CREATE_SCALAR_OP(|, requires(Integer<T>))

    template <std::convertible_to<i32> U>
    friend constexpr Wide operator>>(const Wide &left, const U pshift)
        requires(Integer<T>)
    {
        const i32 shift = static_cast<i32>(pshift);
        if constexpr (s_IsSize<8>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi64(left.m_Data, shift)};
            else
                return Wide{_mm_srl_epi64(left.m_Data, _mm_cvtsi32_si128(shift))};
        }
        else if constexpr (s_IsSize<4>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi32(left.m_Data, _mm_cvtsi32_si128(shift))};
            else
                return Wide{_mm_srl_epi32(left.m_Data, _mm_cvtsi32_si128(shift))};
        }
        else if constexpr (s_IsSize<2>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi16(left.m_Data, _mm_cvtsi32_si128(shift))};
            else
                return Wide{_mm_srl_epi16(left.m_Data, _mm_cvtsi32_si128(shift))};
        }
        else if constexpr (s_IsSize<1>)
        {
            if constexpr (std::is_signed_v<T>)
                return Wide{_mm_sra_epi8(left.m_Data, shift)};
            else
                return Wide{_mm_srl_epi8(left.m_Data, shift)};
        }
    }
    template <std::convertible_to<i32> U>
    friend constexpr Wide operator<<(const Wide &left, const U pshift)
        requires(Integer<T>)
    {
        const i32 shift = static_cast<i32>(pshift);
        if constexpr (s_IsSize<8>)
            return Wide{_mm_sll_epi64(left.m_Data, _mm_cvtsi32_si128(shift))};
        else if constexpr (s_IsSize<4>)
            return Wide{_mm_sll_epi32(left.m_Data, _mm_cvtsi32_si128(shift))};
        else if constexpr (s_IsSize<2>)
            return Wide{_mm_sll_epi16(left.m_Data, _mm_cvtsi32_si128(shift))};
        else if constexpr (s_IsSize<1>)
            return Wide{_mm_sll_epi8(left.m_Data, shift)};
    }
    friend constexpr Wide operator&(const Wide &left, const Wide &right)
        requires(Integer<T>)
    {
        return Wide{_mm_and_si128(left.m_Data, right.m_Data)};
    }
    friend constexpr Wide operator|(const Wide &left, const Wide &right)
        requires(Integer<T>)
    {
        return Wide{_mm_or_si128(left.m_Data, right.m_Data)};
    }

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
            if constexpr (s_Equals<__m128>)                                                                            \
                return _mm_cmp##opName##_ps(left.m_Data, right.m_Data);                                                \
            else if constexpr (s_Equals<__m128d>)                                                                      \
                return _mm_cmp##opName##_pd(left.m_Data, right.m_Data);                                                \
            else if constexpr (s_Equals<__m128i>)                                                                      \
            {                                                                                                          \
                if constexpr (s_IsSize<8>)                                                                             \
                    return _mm_cmp##opName##_epi64(left.m_Data, right.m_Data);                                         \
                else if constexpr (s_IsSize<4>)                                                                        \
                    return _mm_cmp##opName##_epi32(left.m_Data, right.m_Data);                                         \
                else if constexpr (s_IsSize<2>)                                                                        \
                    return _mm_cmp##opName##_epi16(left.m_Data, right.m_Data);                                         \
                else if constexpr (s_IsSize<1>)                                                                        \
                    return _mm_cmp##opName##_epi8(left.m_Data, right.m_Data);                                          \
                CREATE_BAD_BRANCH()                                                                                    \
            }                                                                                                          \
        }

    CREATE_CMP_OP(==, eq)
    CREATE_CMP_OP(!=, neq)
    CREATE_CMP_OP(<, lt)
    CREATE_CMP_OP(>, gt)
    CREATE_CMP_OP(<=, le)
    CREATE_CMP_OP(>=, ge)

    static T Reduce(const Wide &wide)
    {
        if constexpr (s_Equals<__m128>)
        {
            __m128 shift = _mm_movehl_ps(wide.m_Data, wide.m_Data);
            __m128 sum = _mm_add_ps(wide.m_Data, shift);
            shift = _mm_shuffle_ps(sum, sum, 0x55);
            sum = _mm_add_ss(sum, shift);
            return _mm_cvtss_f32(sum);
        }
        else if constexpr (s_Equals<__m128d>)
        {
            __m128d shift = _mm_unpackhi_pd(wide.m_Data, wide.m_Data);
            const __m128d sum = _mm_add_sd(wide.m_Data, shift);
            return _mm_cvtsd_f64(sum);
        }
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
            {
                const __m128i tmp = _mm_srli_si128(wide.m_Data, 8);
                const __m128i sum = _mm_add_epi64(wide.m_Data, tmp);

                return _mm_cvtsi128_si64(sum);
            }
            else if constexpr (s_IsSize<4>)
            {
                __m128i tmp = _mm_srli_si128(wide.m_Data, 4);
                __m128i sum = _mm_add_epi32(wide.m_Data, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi32(sum, tmp);

                return _mm_cvtsi128_si32(sum);
            }
            else if constexpr (s_IsSize<2>)
            {
                __m128i tmp = _mm_srli_si128(wide.m_Data, 2);
                __m128i sum = _mm_add_epi16(wide.m_Data, tmp);
                tmp = _mm_srli_si128(sum, 4);
                sum = _mm_add_epi16(sum, tmp);
                tmp = _mm_srli_si128(sum, 8);
                sum = _mm_add_epi16(sum, tmp);

                return static_cast<T>(_mm_cvtsi128_si32(sum));
            }
            else if constexpr (s_IsSize<1>)
            {
                __m128i tmp = _mm_srli_si128(wide.m_Data, 1);
                __m128i sum = _mm_add_epi8(wide.m_Data, tmp);
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
        if constexpr (s_Equals<__m128>)
            return static_cast<BitMask>(_mm_movemask_ps(mask));
        else if constexpr (s_Equals<__m128d>)
            return static_cast<BitMask>(_mm_movemask_pd(mask));
        else if constexpr (s_Equals<__m128i>)
        {
            const u32 byteMask = static_cast<u32>(_mm_movemask_epi8(mask));
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
                for (usize i = 0; i < Lanes; ++i)
                    packed |= ((byteMask >> (i * sizeof(T))) & 1u) << i;

                return packed;
            }
        }
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

    static constexpr bool NoneOf(const Mask &mask)
    {
        return NoneOf(PackMask(mask));
    }
    static constexpr bool AnyOf(const Mask &mask)
    {
        return AnyOf(PackMask(mask));
    }
    static constexpr bool AllOf(const Mask &mask)
    {
        return AllOf(PackMask(mask));
    }

  private:
    static m128 loadAligned(const T *data)
    {
        TKIT_ASSERT(IsAligned(data, Alignment),
                    "[TOOLKIT][SSE] Data must be aligned to {} bytes to use the SSE SIMD set", Alignment);

        if constexpr (s_Equals<__m128>)
            return _mm_load_ps(data);
        else if constexpr (s_Equals<__m128d>)
            return _mm_load_pd(data);
        else if constexpr (s_Equals<__m128i>)
            return _mm_load_si128(reinterpret_cast<const __m128i *>(data));
        CREATE_BAD_BRANCH()
    }
    static m128 loadUnaligned(const T *data)
    {
        if constexpr (s_Equals<__m128>)
            return _mm_loadu_ps(data);
        else if constexpr (s_Equals<__m128d>)
            return _mm_loadu_pd(data);
        else if constexpr (s_Equals<__m128i>)
            return _mm_loadu_si128(reinterpret_cast<const __m128i *>(data));
        CREATE_BAD_BRANCH()
    }

    static m128 set(const T data)
    {
        if constexpr (s_Equals<__m128>)
            return _mm_set1_ps(data);
        else if constexpr (s_Equals<__m128d>)
            return _mm_set1_pd(data);
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm_set1_epi64x(data);
            else if constexpr (s_IsSize<4>)
                return _mm_set1_epi32(data);
            else if constexpr (s_IsSize<2>)
                return _mm_set1_epi16(data);
            else if constexpr (s_IsSize<1>)
                return _mm_set1_epi8(data);
            CREATE_BAD_BRANCH()
        }
        CREATE_BAD_BRANCH()
    }

    template <typename Callable, usize... I>
    static constexpr m128 makeIntrinsic(Callable &&callable, std::integer_sequence<usize, I...>)
    {
        if constexpr (s_Equals<__m128>)
            return _mm_setr_ps(static_cast<T>(std::forward<Callable>(callable)(I))...);
        else if constexpr (s_Equals<__m128d>)
            return _mm_setr_pd(static_cast<T>(std::forward<Callable>(callable)(I))...);
        else if constexpr (s_Equals<__m128i>)
        {
            if constexpr (s_IsSize<8>)
                return _mm_set_epi64x(static_cast<T>(std::forward<Callable>(callable)(Lanes - I - 1))...);
            else if constexpr (s_IsSize<4>)
                return _mm_setr_epi32(static_cast<T>(std::forward<Callable>(callable)(I))...);
            else if constexpr (s_IsSize<2>)
                return _mm_setr_epi16(static_cast<T>(std::forward<Callable>(callable)(I))...);
            else if constexpr (s_IsSize<1>)
                return _mm_setr_epi8(static_cast<T>(std::forward<Callable>(callable)(I))...);
        }
        CREATE_BAD_BRANCH()
    }

    static constexpr __m128i _mm_srai_epi64_32(const __m128i data)
    {
        const __m128i shifted = _mm_srli_epi64(data, 32);

        __m128i sign = _mm_srai_epi32(_mm_shuffle_epi32(data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm_slli_epi64(sign, 32);

        return _mm_or_si128(shifted, sign);
    };
    static constexpr __m128i _mm_sra_epi64(const __m128i data, const i32 shift)
    {
        const __m128i shifted = _mm_srl_epi64(data, _mm_cvtsi32_si128(shift));

        __m128i sign = _mm_srai_epi32(_mm_shuffle_epi32(data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
        sign = _mm_sll_epi64(sign, _mm_cvtsi32_si128(64 - shift));

        return _mm_or_si128(shifted, sign);
    };
    static constexpr __m128i _mm_srl_epi8(const __m128i data, const i32 shift)
    {
        const __m128i shifted = _mm_srl_epi16(data, _mm_cvtsi32_si128(shift));

        const __m128i mask1 = _mm_set1_epi16(static_cast<i16>(0x00FF >> shift));
        const __m128i mask2 = _mm_set1_epi16(static_cast<i16>(0xFF00));

        const __m128i lo = _mm_and_si128(shifted, mask1);
        const __m128i hi = _mm_and_si128(shifted, mask2);
        return _mm_or_si128(lo, hi);
    }
    static constexpr __m128i _mm_sll_epi8(const __m128i data, const i32 shift)
    {
        const __m128i shifted = _mm_sll_epi16(data, _mm_cvtsi32_si128(shift));

        const __m128i mask1 = _mm_set1_epi16(static_cast<i16>(0xFF00 << shift));
        const __m128i mask2 = _mm_set1_epi16(static_cast<i16>(0x00FF));

        const __m128i lo = _mm_and_si128(shifted, mask1);
        const __m128i hi = _mm_and_si128(shifted, mask2);
        return _mm_or_si128(lo, hi);
    }
    static constexpr __m128i _mm_sra_epi8(const __m128i data, const i32 shift)
    {
        const __m128i shifted = _mm_srl_epi8(data, shift);
        const __m128i signmask = _mm_cmpgt_epi8(_mm_setzero_si128(), data);
        const __m128i mask = _mm_sll_epi8(signmask, 8 - shift);

        return _mm_or_si128(shifted, mask);
    }
    static constexpr __m128i _mm_mullo_epi64(const __m128i left, const __m128i right)
    {
        const __m128i mask32 = _mm_set1_epi64x(0xFFFFFFFF);

        const __m128i llo = _mm_and_si128(left, mask32);
        const __m128i rlo = _mm_and_si128(right, mask32);

        __m128i lhi;
        __m128i rhi;
        if constexpr (!std::is_signed_v<T>)
        {
            lhi = _mm_srli_epi64(left, 32);
            rhi = _mm_srli_epi64(right, 32);
        }
        else
        {
            lhi = _mm_srai_epi64_32(left);
            rhi = _mm_srai_epi64_32(right);
        }

        const __m128i lo = _mm_mul_epu32(llo, rlo);
        const __m128i mid1 = _mm_mullo_epi32(lhi, rlo);
        const __m128i mid2 = _mm_mullo_epi32(llo, rhi);

        __m128i mid = _mm_add_epi64(mid1, mid2);
        mid = _mm_slli_epi64(mid, 32);

        return _mm_add_epi64(lo, mid);
    }
#    ifndef TKIT_SIMD_SSE4_1
    static constexpr __m128i _mm_mullo_epi32(const __m128i left, const __m128i right)
    {
        const __m128i tmp1 = _mm_mul_epu32(left, right);
        const __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(left, 4), _mm_srli_si128(right, 4));

        return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)),
                                  _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
    }
#    endif
    static constexpr __m128i _mm_mullo_epi8(const __m128i left, const __m128i right)
    {
        const __m128i zero = _mm_setzero_si128();
        const __m128i llo = _mm_unpacklo_epi8(left, zero);
        const __m128i lhi = _mm_unpackhi_epi8(left, zero);

        const __m128i rlo = _mm_unpacklo_epi8(right, zero);
        const __m128i rhi = _mm_unpackhi_epi8(right, zero);

        const __m128i plo = _mm_mullo_epi16(llo, rlo);
        const __m128i phi = _mm_mullo_epi16(lhi, rhi);

        const __m128i mask = _mm_set1_epi16(0x00FF);
        return _mm_packus_epi16(_mm_and_si128(plo, mask), _mm_and_si128(phi, mask));
    }
    static constexpr __m128i _mm_min_epi64(const __m128i left, const __m128i right)
    {
        const __m128i cmp = _mm_cmpgt_epi64(left, right);
        return _mm_blendv_epi8(left, right, cmp);
    }
    static constexpr __m128i _mm_max_epi64(const __m128i left, const __m128i right)
    {
        const __m128i cmp = _mm_cmpgt_epi64(left, right);
        return _mm_blendv_epi8(right, left, cmp);
    }

#    ifndef TKIT_SIMD_SSE4_1
    static constexpr __m128 _mm_blendv_ps(const __m128 left, const __m128 right, const __m128 mask)
    {
        return _mm_or_ps(_mm_andnot_ps(mask, left), _mm_and_ps(mask, right));
    }
    static constexpr __m128d _mm_blendv_pd(const __m128d left, const __m128d right, const __m128d mask)
    {
        return _mm_or_pd(_mm_andnot_pd(mask, left), _mm_and_pd(mask, right));
    }
    static constexpr __m128i _mm_blendv_epi8(const __m128i left, const __m128i right, const __m128i mask)
    {
        return _mm_or_si128(_mm_andnot_si128(mask, left), _mm_and_si128(mask, right));
    }
    static constexpr __m128i _mm_cmpeq_epi64(const __m128i left, const __m128i right)
    {
        const __m128i cmp1 = _mm_cmpeq_epi32(left, right);
        const __m128i cmp2 = _mm_shuffle_epi32(cmp1, _MM_SHUFFLE(2, 3, 0, 1));
        return _mm_and_si128(cmp1, cmp2);
    }
#    endif

    static constexpr __m128i _mm_cmpgt_epi64(__m128i left, __m128i right)
    {
#    ifndef TKIT_SIMD_SSE4_2
        __m128i sign;
        if constexpr (!std::is_signed_v<T>)
            sign = _mm_set1_epi64x(static_cast<i64>(1ull << 63 | 1ull << 31));
        else
            sign = _mm_set1_epi64x(static_cast<i64>(1ull << 31));

        left = _mm_xor_si128(left, sign);
        right = _mm_xor_si128(right, sign);

        const __m128i lhi = _mm_srli_epi64(left, 32);
        const __m128i rhi = _mm_srli_epi64(right, 32);

        const __m128i gthi = ::_mm_cmpgt_epi32(lhi, rhi);
        const __m128i eqhi = _mm_cmpeq_epi32(lhi, rhi);

        const __m128i gtlo = ::_mm_cmpgt_epi32(left, right);
        const __m128i result = _mm_or_si128(gthi, _mm_and_si128(eqhi, gtlo));
        return _mm_shuffle_epi32(result, _MM_SHUFFLE(2, 2, 0, 0));
#    else
        if constexpr (!std::is_signed_v<T>)
        {
            const __m128i sign = _mm_set1_epi64x(static_cast<i64>(1ull << 63));
            left = _mm_xor_si128(left, sign);
            right = _mm_xor_si128(right, sign);
        }
        return ::_mm_cmpgt_epi64(left, right);
#    endif
    }
    static constexpr __m128i _mm_cmpgt_epi32(const __m128i left, const __m128i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmpgt_epi32(left, right);
        else
        {
            const __m128i offset = _mm_set1_epi32(static_cast<i32>(1 << 31));
            return ::_mm_cmpgt_epi32(_mm_xor_si128(left, offset), _mm_xor_si128(right, offset));
        }
    }
    static constexpr __m128i _mm_cmpgt_epi16(const __m128i left, const __m128i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmpgt_epi16(left, right);
        else
        {
            const __m128i offset = _mm_set1_epi16(static_cast<i16>(1 << 15));
            return ::_mm_cmpgt_epi16(_mm_xor_si128(left, offset), _mm_xor_si128(right, offset));
        }
    }
    static constexpr __m128i _mm_cmpgt_epi8(const __m128i left, const __m128i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmpgt_epi8(left, right);
        else
        {
            const __m128i offset = _mm_set1_epi8(static_cast<i8>(1 << 7));
            return ::_mm_cmpgt_epi8(_mm_xor_si128(left, offset), _mm_xor_si128(right, offset));
        }
    }
    static constexpr __m128i _mm_cmplt_epi64(const __m128i left, const __m128i right)
    {
        return _mm_cmpgt_epi64(right, left);
    }
    static constexpr __m128i _mm_cmplt_epi32(const __m128i left, const __m128i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmplt_epi32(left, right);
        else
        {
            const __m128i offset = _mm_set1_epi32(static_cast<i32>(1 << 31));
            return ::_mm_cmplt_epi32(_mm_xor_si128(left, offset), _mm_xor_si128(right, offset));
        }
    }
    static constexpr __m128i _mm_cmplt_epi16(const __m128i left, const __m128i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmplt_epi16(left, right);
        else
        {
            const __m128i offset = _mm_set1_epi16(static_cast<i16>(1 << 15));
            return ::_mm_cmplt_epi16(_mm_xor_si128(left, offset), _mm_xor_si128(right, offset));
        }
    }
    static constexpr __m128i _mm_cmplt_epi8(const __m128i left, const __m128i right)
    {
        if constexpr (std::is_signed_v<T>)
            return ::_mm_cmplt_epi8(left, right);
        else
        {
            const __m128i offset = _mm_set1_epi8(static_cast<i8>(1 << 7));
            return ::_mm_cmplt_epi8(_mm_xor_si128(left, offset), _mm_xor_si128(right, offset));
        }
    }
#    define CREATE_INT_CMP(type, name)                                                                                 \
        static constexpr __m128i _mm_cmpneq_ep##type(const __m128i left, const __m128i right)                          \
        {                                                                                                              \
            return _mm_xor_si128(_mm_cmpeq_ep##type(left, right), _mm_set1_ep##name(static_cast<type>(-1)));           \
        }                                                                                                              \
        static constexpr __m128i _mm_cmpge_ep##type(const __m128i left, const __m128i right)                           \
        {                                                                                                              \
            return _mm_xor_si128(_mm_cmplt_ep##type(left, right), _mm_set1_ep##name(static_cast<type>(-1)));           \
        }                                                                                                              \
        static constexpr __m128i _mm_cmple_ep##type(const __m128i left, const __m128i right)                           \
        {                                                                                                              \
            return _mm_xor_si128(_mm_cmpgt_ep##type(left, right), _mm_set1_ep##name(static_cast<type>(-1)));           \
        }

    CREATE_INT_CMP(i8, i8)
    CREATE_INT_CMP(i16, i16)
    CREATE_INT_CMP(i32, i32)
    CREATE_INT_CMP(i64, i64x)

    m128 m_Data;
};
} // namespace TKit::Simd::SSE
TKIT_COMPILER_WARNING_IGNORE_POP()
#    undef CREATE_ARITHMETIC_OP
#    undef CREATE_CMP_OP
#    undef CREATE_MIN_MAX
#    undef CREATE_BAD_BRANCH
#    undef CREATE_EQ_CMP
#    undef CREATE_INT_CMP
#    undef CREATE_SELF_OP
#endif
