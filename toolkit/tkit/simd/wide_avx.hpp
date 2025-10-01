#pragma once

#include "tkit/preprocessor/system.hpp"
#if defined(TKIT_SIMD_AVX) || defined(TKIT_SIMD_AVX2)
#    include "tkit/simd/wide.hpp" // remove this
#    include <immintrin.h>

namespace TKit
{
template <Detail::Float T, usize L, typename Traits> class Wide<T, L, Traits>
{
  public:
    using ValueType = T;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = L;
    static constexpr SizeType Alignment = 32;
    using Mask = u<L>;

    constexpr Wide() = default;
    constexpr Wide(const __m256 p_Data) : m_Data(p_Data)
    {
    }
    constexpr Wide(const T p_Data) : m_Data(_mm256_set1_ps(p_Data))
    {
    }
#    ifndef TKIT_ENABLE_ASSERTS
    constexpr Wide(const T *p_Data) : m_Data(_mm256_load_ps(p_Data))
    {
    }
#    else
    constexpr Wide(const T *p_Data)
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][SIMD] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);
        m_Data = _mm256_load_ps(p_Data);
    }
#    endif

    template <typename Callable>
        requires std::invocable<Callable, SizeType>
    constexpr Wide(Callable &&p_Callable)
    {
        alignas(Alignment) T tmp[Lanes];
        for (SizeType i = 0; i < Lanes; ++i)
            tmp[i] = static_cast<T>(p_Callable(i));
        m_Data = _mm256_load_ps(tmp);
    }

    constexpr void Store(T *p_Data) const
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][SIMD] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);
        _mm256_store_ps(p_Data, m_Data);
    }

    constexpr const ValueType At(const SizeType p_Index) const
    {
        alignas(32) T tmp[8];
        _mm256_store_ps(tmp, m_Data);
        return tmp[p_Index];
    }
    constexpr ValueType &At(const SizeType) = delete;
    constexpr const ValueType operator[](const SizeType p_Index) const
    {
        return At(p_Index);
    }
    constexpr ValueType &operator[](const SizeType) = delete;

    friend constexpr Wide Min(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_min_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide Max(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_max_ps(p_Left.m_Data, p_Right.m_Data)};
    }

    // Select(mask, left, right)
    friend constexpr Wide Select(const Mask p_Mask, const Wide &p_Left, const Wide &p_Right)
    {
        alignas(32) u32 bits[Lanes];
        for (SizeType i = 0; i < Lanes; ++i)
            bits[i] = (p_Mask & (Mask{1} << i)) ? 0xFFFFFFFFu : 0u;

        const __m256 mv = _mm256_castsi256_ps(_mm256_load_si256(reinterpret_cast<const __m256i *>(bits)));
        return Wide{_mm256_or_ps(_mm256_and_ps(mv, p_Left.m_Data), _mm256_andnot_ps(mv, p_Right.m_Data))};
    }

    // ReduceAdd
    friend constexpr T ReduceAdd(const Wide &p_Wide)
    {
        if constexpr (std::same_as<T, f32>)
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
        else
        {
        }
    }

    friend constexpr Wide operator+(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_add_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator-(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_sub_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator*(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_mul_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator/(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_div_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator-(const Wide &p_Other)
    {
        return Wide{_mm256_sub_ps(_mm256_setzero_ps(), p_Other.m_Data)};
    }

#    define CREATE_CMP_OP(p_Op, p_Flag)                                                                                \
        friend inline Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                      \
        {                                                                                                              \
            const __m256 m = _mm256_cmp_ps(p_Left.m_Data, p_Right.m_Data, p_Flag);                                     \
            const i32 lo = _mm_movemask_ps(_mm256_castps256_ps128(m));                                                 \
            const i32 hi = _mm_movemask_ps(_mm256_extractf128_ps(m, 1));                                               \
            return static_cast<Mask>(static_cast<u32>(lo) | (static_cast<u32>(hi) << 4));                              \
        }

    CREATE_CMP_OP(==, _CMP_EQ_OQ)
    CREATE_CMP_OP(!=, _CMP_NEQ_UQ)
    CREATE_CMP_OP(<, _CMP_LT_OQ)
    CREATE_CMP_OP(>, _CMP_GT_OQ)
    CREATE_CMP_OP(<=, _CMP_LE_OQ)
    CREATE_CMP_OP(>=, _CMP_GE_OQ)

#    undef CREATE_CMP_OP

  private:
    __m256 m_Data;
};

#    ifdef TKIT_SIMD_AVX2

template <Detail::Integer T, usize L, typename Traits> class Wide<T, L, Traits>
{
  public:
    using ValueType = T;
    using SizeType = typename Traits::SizeType;

    static constexpr SizeType Lanes = L;
    static constexpr SizeType Alignment = 32;
    using Mask = u<Lanes>;

    constexpr Wide() = default;
    constexpr Wide(const __m256i p_Data) : m_Data(p_Data)
    {
    }
    constexpr Wide(const T p_Data)
    {
        if constexpr (Lanes == 4)
            m_Data = _mm256_set1_epi64x(static_cast<i64>(p_Data));
        else if constexpr (Lanes == 8)
            m_Data = _mm256_set1_epi32(static_cast<i32>(p_Data));
        else if constexpr (Lanes == 16)
            m_Data = _mm256_set1_epi16(static_cast<i16>(p_Data));
        else
            m_Data = _mm256_set1_epi8(static_cast<i8>(p_Data));
    }

#        ifndef TKIT_ENABLE_ASSERTS
    constexpr Wide(const T *p_Data) : m_Data(_mm256_load_si256(reinterpret_cast<const __m256i *>(p_Data)))
    {
    }
#        else
    constexpr Wide(const T *p_Data)
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][SIMD] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);
        m_Data = _mm256_load_si256(reinterpret_cast<const __m256i *>(p_Data));
    }
#        endif

    template <typename Callable>
        requires std::invocable<Callable, SizeType>
    constexpr Wide(Callable &&p_Callable)
    {
        alignas(Alignment) T tmp[Lanes];
        for (SizeType i = 0; i < Lanes; ++i)
            tmp[i] = static_cast<T>(p_Callable(i));
        m_Data = _mm256_load_si256(reinterpret_cast<const __m256i *>(tmp));
    }

    constexpr void Store(T *p_Data) const
    {
        TKIT_ASSERT(Memory::IsAligned(p_Data, Alignment),
                    "[TOOLKIT][SIMD] Data must be aligned to {} bytes to use the AVX SIMD set", Alignment);
        _mm256_store_si256(reinterpret_cast<__m256i *>(p_Data), m_Data);
    }

    friend constexpr Wide Min(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_min_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide Max(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_max_ps(p_Left.m_Data, p_Right.m_Data)};
    }

    // Select(mask, left, right)
    friend constexpr Wide Select(const Mask p_Mask, const Wide &p_Left, const Wide &p_Right)
    {
        alignas(32) u32 bits[Lanes];
        for (SizeType i = 0; i < Lanes; ++i)
            bits[i] = (p_Mask & (Mask{1} << i)) ? 0xFFFFFFFFu : 0u;

        const __m256 mv = _mm256_castsi256_ps(_mm256_load_si256(reinterpret_cast<const __m256i *>(bits)));
        return Wide{_mm256_or_ps(_mm256_and_ps(mv, p_Left.m_Data), _mm256_andnot_ps(mv, p_Right.m_Data))};
    }

    // ReduceAdd
    friend constexpr T ReduceAdd(const Wide &p_Wide)
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

    friend constexpr Wide operator+(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_add_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator-(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_sub_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator*(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_mul_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator/(const Wide &p_Left, const Wide &p_Right)
    {
        return Wide{_mm256_div_ps(p_Left.m_Data, p_Right.m_Data)};
    }
    friend constexpr Wide operator-(const Wide &p_Other)
    {
        return Wide{_mm256_sub_ps(_mm256_setzero_ps(), p_Other.m_Data)};
    }

#        define CREATE_CMP_OP(p_Op, p_Flag)                                                                            \
            friend inline Mask operator p_Op(const Wide &p_Left, const Wide &p_Right)                                  \
            {                                                                                                          \
                const __m256 m = _mm256_cmp_ps(p_Left.m_Data, p_Right.m_Data, p_Flag);                                 \
                const i32 lo = _mm_movemask_ps(_mm256_castps256_ps128(m));                                             \
                const i32 hi = _mm_movemask_ps(_mm256_extractf128_ps(m, 1));                                           \
                return static_cast<Mask>(static_cast<u32>(lo) | (static_cast<u32>(hi) << 4));                          \
            }

    CREATE_CMP_OP(==, _CMP_EQ_OQ)
    CREATE_CMP_OP(!=, _CMP_NEQ_UQ)
    CREATE_CMP_OP(<, _CMP_LT_OQ)
    CREATE_CMP_OP(>, _CMP_GT_OQ)
    CREATE_CMP_OP(<=, _CMP_LE_OQ)
    CREATE_CMP_OP(>=, _CMP_GE_OQ)

  private:
    __m256i m_Data;
};

#    endif

} // namespace TKit
#endif
