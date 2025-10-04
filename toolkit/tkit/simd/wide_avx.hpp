#pragma once

#include "tkit/preprocessor/system.hpp"
#if defined(TKIT_SIMD_AVX) || defined(TKIT_SIMD_AVX2)
#    include "tkit/memory/memory.hpp"
#    include "tkit/simd/utils.hpp"
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

template <Arithmetic T, typename Traits> class Wide
{
    using m256 = TypeSelector<T>::Type;
    template <typename E> static constexpr bool Equals = std::is_same_v<m256, E>;

  public:
    using ValueType = T;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = TKIT_SIMD_AVX_SIZE / sizeof(T);
    static constexpr SizeType Alignment = 32;

    using Mask = m256;
    using BitMask = u<MaskSize<Lanes>()>;

#    define CREATE_BAD_BRANCH(...)                                                                                     \
        else                                                                                                           \
        {                                                                                                              \
            TKIT_ERROR("[TOOLKIT][AVX] This should not happen...");                                                    \
            return __VA_ARGS__;                                                                                        \
        }

    static constexpr BitMask PackMask(const Mask &p_Mask)
    {
        if constexpr (Equals<__m256>)
        {
            const u32 bits = static_cast<u32>(_mm256_movemask_ps(p_Mask));
            return static_cast<BitMask>(bits & ((1u << Lanes) - 1u));
        }
        else if constexpr (Equals<__m256d>)
        {
            const u32 bits = static_cast<u32>(_mm256_movemask_pd(p_Mask));
            return static_cast<BitMask>(bits & ((1u << Lanes) - 1u));
        }
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
        {
            const u32 byteMask = static_cast<u32>(_mm256_movemask_epi8(p_Mask));

            BitMask packed = 0;
            for (SizeType i = 0; i < Lanes; ++i)
                packed |= ((byteMask >> (i * sizeof(T))) & 1u) << i;

            return packed;
        }
#    endif
        CREATE_BAD_BRANCH(0)
    }

    static constexpr Mask WidenMask(const BitMask p_Bits)
    {
        using Integer = u<sizeof(T) * 8>;
        alignas(Alignment) Integer tmp[Lanes];

        for (SizeType i = 0; i < Lanes; ++i)
            tmp[i] = (p_Bits & (BitMask{1} << i)) ? static_cast<Integer>(-1) : Integer{0};

        if constexpr (Equals<__m256>)
            return _mm256_castps_si256(_mm256_load_ps(reinterpret_cast<const f32 *>(tmp)));
        else if constexpr (Equals<__m256d>)
            return _mm256_castpd_si256(_mm256_load_pd(reinterpret_cast<const f64 *>(tmp)));
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
            return _mm256_load_si256(reinterpret_cast<const __m256i *>(tmp));
#    endif
        else
            return Mask{};
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

    template <typename Callable>
        requires std::invocable<Callable, SizeType>
    constexpr Wide(Callable &&p_Callable)
    {
        alignas(Alignment) T tmp[Lanes];
        for (SizeType i = 0; i < Lanes; ++i)
            tmp[i] = static_cast<T>(p_Callable(i));
        m_Data = load(tmp);
    }

    constexpr static Wide LoadAligned(const T *p_Data)
    {
        return Wide{loadAligned(p_Data)};
    }
    constexpr static Wide LoadUnaligned(const T *p_Data)
    {
        return Wide{loadUnaligned(p_Data)};
    }

    constexpr void StoreAligned(T *p_Data) const
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][AVX] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);
        if constexpr (Equals<__m256>)
            _mm256_store_ps(p_Data, m_Data);
        else if constexpr (Equals<__m256d>)
            _mm256_store_pd(p_Data, m_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
            _mm256_store_si256(reinterpret_cast<__m256i *>(p_Data), m_Data);
#    endif
        CREATE_BAD_BRANCH()
    }

    constexpr void StoreUnaligned(T *p_Data) const
    {
        if constexpr (Equals<__m256>)
            _mm256_storeu_ps(p_Data, m_Data);
        else if constexpr (Equals<__m256d>)
            _mm256_storeu_pd(p_Data, m_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(p_Data), m_Data);
#    endif
        CREATE_BAD_BRANCH()
    }

    constexpr const ValueType At(const SizeType p_Index) const
    {
        alignas(Alignment) T tmp[Lanes];
        StoreAligned(tmp);
        return tmp[p_Index];
    }
    constexpr ValueType &At(const SizeType) = delete;
    constexpr const ValueType operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }
    constexpr ValueType &operator[](const SizeType) = delete;

    static constexpr Wide Select(const Mask &p_Mask, const Wide &p_Left, const Wide &p_Right)
    {
        if constexpr (Equals<__m256>)
            return Wide{_mm256_blendv_ps(p_Left, p_Right, p_Mask)};
        else if constexpr (Equals<__m256d>)
            return Wide{_mm256_blendv_pd(p_Left, p_Right, p_Mask)};
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
            return Wide{_mm256_blendv_epi8(p_Left, p_Right, p_Mask)};
#    endif
        CREATE_BAD_BRANCH(Wide{})
    }

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_MIN_MAX_INT(p_Op)                                                                               \
            else if constexpr (Equals<__m256i>)                                                                        \
            {                                                                                                          \
                if constexpr (Lanes == 4)                                                                              \
                    return Wide{_mm256_##p_Op##_epi64(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (Lanes == 8)                                                                         \
                    return Wide{_mm256_##p_Op##_epi32(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (Lanes == 16)                                                                        \
                    return Wide{_mm256_##p_Op##_epi16(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (Lanes == 32)                                                                        \
                    return Wide{_mm256_##p_Op##_epi8(p_Left.m_Data, p_Right.m_Data)};                                  \
                CREATE_BAD_BRANCH(Wide{})                                                                              \
            }
#    else
#        define CREATE_MIN_MAX_INT(p_Op)
#    endif

#    define CREATE_MIN_MAX(p_Name, p_Op)                                                                               \
        static constexpr Wide p_Name(const Wide &p_Left, const Wide &p_Right)                                          \
        {                                                                                                              \
            if constexpr (Equals<__m256>)                                                                              \
                return Wide{_mm256_##p_Op##_ps(p_Left.m_Data, p_Right.m_Data)};                                        \
            else if constexpr (Equals<__m256d>)                                                                        \
                return Wide{_mm256_##p_Op##_pd(p_Left.m_Data, p_Right.m_Data)};                                        \
            CREATE_MIN_MAX_INT(p_Op)                                                                                   \
            CREATE_BAD_BRANCH(Wide{})                                                                                  \
        }

    CREATE_MIN_MAX(Min, min)
    CREATE_MIN_MAX(Max, max)

#    undef CREATE_MIN_MAX
#    undef CREATE_MIN_MAX_INT

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_ARITHMETIC_OP_INT(p_Op)                                                                         \
            else if constexpr (Equals<__m256i>)                                                                        \
            {                                                                                                          \
                if constexpr (Lanes == 4)                                                                              \
                    return Wide{_mm256_##p_Op##_epi64(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (Lanes == 8)                                                                         \
                    return Wide{_mm256_##p_Op##_epi32(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (Lanes == 16)                                                                        \
                    return Wide{_mm256_##p_Op##_epi16(p_Left.m_Data, p_Right.m_Data)};                                 \
                else if constexpr (Lanes == 32)                                                                        \
                    return Wide{_mm256_##p_Op##_epi8(p_Left.m_Data, p_Right.m_Data)};                                  \
                CREATE_BAD_BRANCH(Wide{})                                                                              \
            }
#    else
#        define CREATE_ARITHMETIC_OP_INT(p_Op)
#    endif

#    define CREATE_ARITHMETIC_OP(p_Op, p_OpName)                                                                       \
        friend constexpr Wide operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            if constexpr (Equals<__m256>)                                                                              \
                return Wide{_mm256_##p_OpName##_ps(p_Left.m_Data, p_Right.m_Data)};                                    \
            else if constexpr (Equals<__m256d>)                                                                        \
                return Wide{_mm256_##p_OpName##_pd(p_Left.m_Data, p_Right.m_Data)};                                    \
            CREATE_ARITHMETIC_OP_INT(p_OpName)                                                                         \
            CREATE_BAD_BRANCH(Wide{})                                                                                  \
        }

    CREATE_ARITHMETIC_OP(+, add)
    CREATE_ARITHMETIC_OP(-, sub)

    friend constexpr Wide operator*(const Wide &p_Left, const Wide &p_Right)
    {
        if constexpr (Equals<__m256>)
            return Wide{_mm256_mul_ps(p_Left.m_Data, p_Right.m_Data)};
        else if constexpr (Equals<__m256d>)
            return Wide{_mm256_mul_pd(p_Left.m_Data, p_Right.m_Data)};
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
        {
            if constexpr (Lanes == 4)
            {
                const __m256i mask32 = _mm256_set1_epi64x(0xFFFFFFFF);

                const __m256i llo = _mm256_and_si256(p_Left.m_Data, mask32);
                const __m256i rlo = _mm256_and_si256(p_Right.m_Data, mask32);

                __m256i lhi;
                __m256i rhi;
                if constexpr (!std::is_signed_v<T>)
                {
                    lhi = _mm256_srli_epi64(p_Left.m_Data, 32);
                    rhi = _mm256_srli_epi64(p_Right.m_Data, 32);
                }
                else
                {
                    const auto srai_epi64_32 = [](const __m256i p_Data) {
                        const __m256i shifted = _mm256_srli_epi64(p_Data, 32);

                        __m256i sign = _mm256_srai_epi32(_mm256_shuffle_epi32(p_Data, _MM_SHUFFLE(3, 3, 1, 1)), 31);
                        sign = _mm256_slli_epi64(sign, 32);

                        return _mm256_or_si256(shifted, sign);
                    };

                    lhi = srai_epi64_32(p_Left.m_Data, 32);
                    rhi = srai_epi64_32(p_Right.m_Data, 32);
                }

                const __m256i lo = _mm256_mul_epu32(llo, rlo);
                const __m256i mid1 = _mm256_mullo_epi32(lhi, rlo);
                const __m256i mid2 = _mm256_mullo_epi32(llo, rhi);

                __m256i mid = _mm256_add_epi64(mid1, mid2);
                mid = _mm256_slli_epi64(mid, 32);

                return Wide{_mm256_add_epi64(lo, mid)};
            }
            else if constexpr (Lanes == 8)
                return Wide{_mm256_mullo_epi32(p_Left.m_Data, p_Right.m_Data)};
            else if constexpr (Lanes == 16)
                return Wide{_mm256_mullo_epi16(p_Left.m_Data, p_Right.m_Data)};
            else if constexpr (Lanes == 32)
            {
                const __m256i llo = _mm256_unpacklo_epi8(p_Left.m_Data, _mm256_setzero_si256());
                const __m256i lhi = _mm256_unpackhi_epi8(p_Left.m_Data, _mm256_setzero_si256());

                const __m256i rlo = _mm256_unpacklo_epi8(p_Right.m_Data, _mm256_setzero_si256());
                const __m256i rhi = _mm256_unpackhi_epi8(p_Right.m_Data, _mm256_setzero_si256());

                // Multiply 16-bit values
                const __m256i plo = _mm256_mullo_epi16(llo, rlo);
                const __m256i phi = _mm256_mullo_epi16(lhi, rhi);

                if constexpr (!std::is_signed_v<T>)
                    return Wide{_mm256_packus_epi16(plo, phi)};
                else
                    return Wide{_mm256_packs_epi16(plo, phi)};
            }
        }
#    endif
        CREATE_BAD_BRANCH(Wide{})
    }

#    undef CREATE_ARITHMETIC_OP
#    undef CREATE_ARITHMETIC_OP_INT

#    ifdef TKIT_SIMD_AVX2
#        define CREATE_CMP_OP_INT(p_Op)                                                                                \
            else if constexpr (Equals<__m256i>)                                                                        \
            {                                                                                                          \
                if constexpr (Lanes == 4)                                                                              \
                    return _mm256_##p_Op##_epi64(p_Left.m_Data, p_Right.m_Data);                                       \
                else if constexpr (Lanes == 8)                                                                         \
                    return _mm256_##p_Op##_epi32(p_Left.m_Data, p_Right.m_Data);                                       \
                else if constexpr (Lanes == 16)                                                                        \
                    return _mm256_##p_Op##_epi16(p_Left.m_Data, p_Right.m_Data);                                       \
                else if constexpr (Lanes == 32)                                                                        \
                    return _mm256_##p_Op##_epi8(p_Left.m_Data, p_Right.m_Data);                                        \
                CREATE_BAD_BRANCH(Mask{})                                                                              \
            }
#    else
#        define CREATE_CMP_OP_INT(p_Op)
#    endif

#    define CREATE_CMP_OP(p_Op, p_Flag, p_IntOpName)                                                                   \
        friend constexpr Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                   \
        {                                                                                                              \
            if constexpr (Equals<__m256>)                                                                              \
                return _mm256_cmp_ps(p_Left.m_Data, p_Right.m_Data, p_Flag);                                           \
            else if constexpr (Equals<__m256d>)                                                                        \
                return _mm256_cmp_pd(p_Left.m_Data, p_Right.m_Data, p_Flag);                                           \
            CREATE_CMP_OP_INT(p_IntOpName)                                                                             \
        }

    CREATE_CMP_OP(==, _CMP_EQ_OQ, eq)
    CREATE_CMP_OP(!=, _CMP_NEQ_UQ, neq)
    CREATE_CMP_OP(<, _CMP_LT_OQ, lt)
    CREATE_CMP_OP(>, _CMP_GT_OQ, gt)
    CREATE_CMP_OP(<=, _CMP_LE_OQ, le)
    CREATE_CMP_OP(>=, _CMP_GE_OQ, ge)

#    undef CREATE_CMP_OP
#    undef CREATE_CMP_OP_INT

#    define CREATE_REDUCE_INT(p_Op)                                                                                    \
        else if constexpr (Equals<__m256i>)                                                                            \
        {                                                                                                              \
            if constexpr (Lanes == 4)                                                                                  \
            {                                                                                                          \
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);                                              \
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);                                         \
                __m128i sum = _mm_##p_Op##_epi64(lo, hi);                                                              \
                __m128i shift = _mm_unpackhi_epi64(sum, sum);                                                          \
                sum = _mm_##p_Op##_epi64(sum, shift);                                                                  \
                return static_cast<T>(_mm_cvtsi128_si64(sum));                                                         \
            }                                                                                                          \
            else if constexpr (Lanes == 8)                                                                             \
            {                                                                                                          \
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);                                              \
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);                                         \
                __m128i sum = _mm_##p_Op##_epi32(lo, hi);                                                              \
                __m128i shift = _mm_unpackhi_epi32(sum, sum);                                                          \
                                                                                                                       \
                sum = _mm_##p_Op##_epi32(sum, shift);                                                                  \
                shift = _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1));                                               \
                sum = _mm_##p_Op##_epi32(sum, shift);                                                                  \
                                                                                                                       \
                return static_cast<T>(_mm_cvtsi128_si32(sum));                                                         \
            }                                                                                                          \
            else if constexpr (Lanes == 16)                                                                            \
            {                                                                                                          \
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);                                              \
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);                                         \
                __m128i sum = _mm_##p_Op##_epi16(lo, hi);                                                              \
                                                                                                                       \
                __m128i shift = _mm_unpackhi_epi16(sum, sum);                                                          \
                sum = _mm_##p_Op##_epi16(sum, shift);                                                                  \
                                                                                                                       \
                shift = _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1));                                               \
                sum = _mm_##p_Op##_epi16(sum, shift);                                                                  \
                                                                                                                       \
                shift = _mm_shufflelo_epi16(sum, _MM_SHUFFLE(2, 3, 0, 1));                                             \
                sum = _mm_##p_Op##_epi16(sum, shift);                                                                  \
                                                                                                                       \
                return static_cast<T>(_mm_cvtsi128_si16(sum));                                                         \
            }                                                                                                          \
            else if constexpr (Lanes == 32)                                                                            \
            {                                                                                                          \
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);                                              \
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);                                         \
                __m128i sum = _mm_##p_Op##_epi8(lo, hi);                                                               \
                                                                                                                       \
                __m128i shift = _mm_unpackhi_epi8(sum, sum);                                                           \
                sum = _mm_##p_Op##_epi8(sum, shift);                                                                   \
                                                                                                                       \
                shift = _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1));                                               \
                sum = _mm_##p_Op##_epi8(sum, shift);                                                                   \
                                                                                                                       \
                shift = _mm_shufflelo_epi16(sum, _MM_SHUFFLE(2, 3, 0, 1));                                             \
                sum = _mm_##p_Op##_epi8(sum, shift);                                                                   \
                                                                                                                       \
                shift = _mm_srli_si128(sum, 1);                                                                        \
                sum = _mm_##p_Op##_epi8(sum, shift);                                                                   \
                                                                                                                       \
                return static_cast<T>(_mm_cvtsi128_si16(sum));                                                         \
            }                                                                                                          \
        }

    static constexpr T Reduce(const Wide &p_Wide)
    {
        if constexpr (Equals<__m256>)
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
        else if constexpr (Equals<__m256d>)
        {
            const __m128d lo = _mm256_castpd256_pd128(p_Wide.m_Data);
            const __m128d hi = _mm256_extractf128_pd(p_Wide.m_Data, 1);
            __m128d sum = _mm_add_pd(lo, hi);
            __m128d shift = _mm_unpackhi_pd(sum, sum);
            sum = _mm_add_sd(sum, shift);
            return _mm_cvtsd_f64(sum);
        }
        else if constexpr (Equals<__m256i>)
        {
            if constexpr (Lanes == 4)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);
                __m128i sum = _mm_add_epi64(lo, hi);
                __m128i shift = _mm_unpackhi_epi64(sum, sum);
                sum = _mm_add_epi64(sum, shift);
                return static_cast<T>(_mm_cvtsi128_si64(sum));
            }
            else if constexpr (Lanes == 8)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);
                __m128i sum = _mm_add_epi32(lo, hi);
                __m128i shift = _mm_unpackhi_epi32(sum, sum);

                sum = _mm_add_epi32(sum, shift);
                shift = _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1));
                sum = _mm_add_epi32(sum, shift);

                return static_cast<T>(_mm_cvtsi128_si32(sum));
            }
            else if constexpr (Lanes == 16)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);
                __m128i sum = _mm_add_epi16(lo, hi);

                __m128i shift = _mm_unpackhi_epi16(sum, sum);
                sum = _mm_add_epi16(sum, shift);

                shift = _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1));
                sum = _mm_add_epi16(sum, shift);

                shift = _mm_shufflelo_epi16(sum, _MM_SHUFFLE(2, 3, 0, 1));
                sum = _mm_add_epi16(sum, shift);

                return static_cast<T>(_mm_cvtsi128_si16(sum));
            }
            else if constexpr (Lanes == 32)
            {
                const __m128i lo = _mm256_castsi256_si128(p_Wide.m_Data);
                const __m128i hi = _mm256_extracti128_si256(p_Wide.m_Data, 1);
                __m128i sum = _mm_add_epi8(lo, hi);

                __m128i shift = _mm_unpackhi_epi8(sum, sum);
                sum = _mm_add_epi8(sum, shift);

                shift = _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1));
                sum = _mm_add_epi8(sum, shift);

                shift = _mm_shufflelo_epi16(sum, _MM_SHUFFLE(2, 3, 0, 1));
                sum = _mm_add_epi8(sum, shift);

                shift = _mm_srli_si128(sum, 1);
                sum = _mm_add_epi8(sum, shift);

                return static_cast<T>(_mm_cvtsi128_si16(sum));
            }
        }
    }

  private:
    static m256 loadAligned(const T *p_Data)
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][AVX] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);

        if constexpr (Equals<__m256>)
            return _mm256_load_ps(p_Data);
        else if constexpr (Equals<__m256d>)
            return _mm256_load_pd(p_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
            return _mm256_load_si256(reinterpret_cast<const __m256i *>(p_Data));
#    endif
        CREATE_BAD_BRANCH(m256{})
    }
    static m256 loadUnaligned(const T *p_Data)
    {
        if constexpr (Equals<__m256>)
            return _mm256_loadu_ps(p_Data);
        else if constexpr (Equals<__m256d>)
            return _mm256_loadu_pd(p_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
            return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p_Data));
#    endif
        CREATE_BAD_BRANCH(m256{})
    }

    static m256 set(const T p_Data)
    {
        if constexpr (Equals<__m256>)
            return _mm256_set1_ps(p_Data);
        else if constexpr (Equals<__m256d>)
            return _mm256_set1_pd(p_Data);
#    ifdef TKIT_SIMD_AVX2
        else if constexpr (Equals<__m256i>)
        {
            if constexpr (Lanes == 4)
                return _mm256_set1_epi64x(p_Data);
            else if constexpr (Lanes == 8)
                return _mm256_set1_epi32(p_Data);
            else if constexpr (Lanes == 16)
                return _mm256_set1_epi16(p_Data);
            else if constexpr (Lanes == 32)
                return _mm256_set1_epi8(p_Data);
            CREATE_BAD_BRANCH(m256{})
        }
#    endif
        CREATE_BAD_BRANCH(m256{})
    }

#    undef CREATE_BAD_BRANCH

    m256 m_Data;
}; // namespace TKit::Detail::AVX
} // namespace TKit::Detail::AVX
#endif
