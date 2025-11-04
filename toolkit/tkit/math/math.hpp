#pragma once

#include "tkit/math/tensor.hpp"
#include <math.h>

namespace TKit::Math
{
template <std::floating_point Float> constexpr bool ApproachesZero(const Float p_Value)
{
    return std::abs(p_Value) <= std::numeric_limits<Float>::epsilon();
}

template <std::floating_point Float> constexpr bool Approximately(const Float p_Left, const Float p_Right)
{
    return ApproachesZero(p_Left - p_Right);
}

template <typename T, usize N0, usize... N>
constexpr T Dot(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    T result{static_cast<T>(0)};
    for (usize i = 0; i < N0; ++i)
        if constexpr (sizeof...(N) == 0)
            result += p_Left.Ranked[i] * p_Right.Ranked[i];
        else
            result += Dot(p_Left.Ranked[i], p_Right.Ranked[i]);
    return result;
}

template <typename T, usize N0, usize... N> constexpr T NormSquared(const ten<T, N0, N...> &p_Tensor)
{
    return Dot(p_Tensor, p_Tensor);
}
template <typename T, usize N0, usize... N> constexpr T Norm(const ten<T, N0, N...> &p_Tensor)
{
    return std::sqrt(Dot(p_Tensor, p_Tensor));
}

template <typename T, usize N0, usize... N>
constexpr T DistanceSquared(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    const ten<T, N0, N...> diff = p_Right - p_Left;
    return Dot(diff, diff);
}
template <typename T, usize N0, usize... N>
constexpr T Distance(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    return std::sqrt(DistanceSquared(p_Left, p_Right));
}

template <typename T, usize N0, usize... N> constexpr ten<T, N0, N...> Normalize(const ten<T, N0, N...> &p_Tensor)
{
    return p_Tensor / Norm(p_Tensor);
}

template <typename T> constexpr T SquareRoot(const T p_Value)
{
    return std::sqrt(p_Value);
}

// TODO: Adapt so SIMD
template <typename T, usize N0, usize... N> constexpr ten<T, N0, N...> SquareRoot(const ten<T, N0, N...> &p_Tensor)
{
    ten<T, N0, N...> result;
    for (usize i = 0; i < N0; ++i)
        result.Ranked[i] = SquareRoot(p_Tensor.Ranked[i]);
    return result;
}

template <typename T> constexpr T Min(const std::initializer_list<T> p_Values)
{
    return std::min(p_Values);
}
template <typename T> constexpr T Max(const std::initializer_list<T> p_Values)
{
    return std::max(p_Values);
}

template <typename T> constexpr T Min(const T p_Left, const T p_Right)
{
    return std::min(p_Left, p_Right);
}
template <typename T> constexpr T Max(const T p_Left, const T p_Right)
{
    return std::max(p_Left, p_Right);
}
template <typename T> constexpr T Clamp(const T p_Value, const T p_Min, const T p_Max)
{
    return std::clamp(p_Value, p_Min, p_Max);
}

namespace Detail
{
template <typename T, usize N0, usize... N> constexpr void Min(const ten<T, N0, N...> &p_Tensor, T &p_Min)
{
    for (usize i = 0; i < N0; ++i)
        if constexpr (sizeof...(N) == 0)
        {
            if (p_Tensor.Ranked[i] < p_Min)
                p_Min = p_Tensor.Ranked[i];
        }
        else
            Min(p_Tensor.Ranked[i], p_Min);
}

template <typename T, usize N0, usize... N> constexpr void Max(const ten<T, N0, N...> &p_Tensor, T &p_Max)
{
    for (usize i = 0; i < N0; ++i)
        if constexpr (sizeof...(N) == 0)
        {
            if (p_Tensor.Ranked[i] > p_Max)
                p_Max = p_Tensor.Ranked[i];
        }
        else
            Max(p_Tensor.Ranked[i], p_Max);
}
} // namespace Detail

// TODO: Adapt so SIMD
template <typename T, usize N0, usize... N> constexpr T Min(const ten<T, N0, N...> &p_Tensor)
{
    T mn{std::numeric_limits<T>::max()};
    Detail::Min(p_Tensor, mn);
    return mn;
}
template <typename T, usize N0, usize... N> constexpr T Max(const ten<T, N0, N...> &p_Tensor)
{
    T mn{static_cast<T>(0)};
    Detail::Max(p_Tensor, mn);
    return mn;
}

template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Min(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    ten<T, N0, N...> mn;
    for (usize i = 0; i < N0; ++i)
        mn.Ranked[i] = Min(p_Left.Ranked[i], p_Right.Ranked[i]);
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Max(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    ten<T, N0, N...> mx;
    for (usize i = 0; i < N0; ++i)
        mx.Ranked[i] = Max(p_Left.Ranked[i], p_Right.Ranked[i]);
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Clamp(const ten<T, N0, N...> &p_Tensor, const ten<T, N0, N...> &p_Min,
                                 const ten<T, N0, N...> &p_Max)
{
    ten<T, N0, N...> result;
    for (usize i = 0; i < N0; ++i)
        result.Ranked[i] = Clamp(p_Tensor.Ranked[i], p_Min.Ranked[i], p_Max.Ranked[i]);
    return result;
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Clamp(const ten<T, N0, N...> &p_Tensor, const T p_Min, const T p_Max)
{
    ten<T, N0, N...> result;
    for (usize i = 0; i < N0; ++i)
        result.Ranked[i] = Clamp(p_Tensor.Ranked[i], p_Min, p_Max);
    return result;
}

template <typename T> constexpr T Cosine(const T p_Value)
{
    return std::cos(p_Value);
}
template <typename T> constexpr T Sine(const T p_Value)
{
    return std::sin(p_Value);
}

template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Cosine(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    ten<T, N0, N...> cs;
    for (usize i = 0; i < N0; ++i)
        cs.Ranked[i] = Cosine(p_Left.Ranked[i], p_Right.Ranked[i]);
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Sine(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    ten<T, N0, N...> sn;
    for (usize i = 0; i < N0; ++i)
        sn.Ranked[i] = Sine(p_Left.Ranked[i], p_Right.Ranked[i]);
}

template <typename T> constexpr T Pi()
{
    return static_cast<T>(3.14159265358979323846);
}

// TODO: For vecs this breaks because of the Pi<T>()
template <typename T> constexpr T Radians(const T p_Degrees)
{
    return p_Degrees * (Pi<T>() / 180.f);
}
template <typename T> constexpr T Degrees(const T p_Radians)
{
    return p_Radians * (180.f / Pi<T>());
}

namespace Detail
{
template <typename Src, usize I0, usize... I, typename T, usize N0, usize... N>
constexpr void Permute(const typename ten<T, N0, N...>::template Permute<I0, I...> &p_Dst, const Src &p_Src)
{
    constexpr usize n = GetAxis<I0, N0, N...>();
    for (usize i = 0; i < n; ++i)
        if constexpr (sizeof...(N) == 0)
            p_Src.template At<I0>() = p_Dst[i];
}
} // namespace Detail

template <usize I0, usize I1, usize... I, typename T, usize N0, usize N1, usize... N>
constexpr ten<T, N0, N1, N...>::template Permute<I0, I1, I...> Permute(const ten<T, N0, N1, N...> &p_Tensor)
{
    static_assert(sizeof...(I) == sizeof...(N), "[TOOLKIT][MATH] The rank of the index object defining the permutation "
                                                "must have the same rank as the tensor to permutate.");
    using pten = ten<T, N0, N1, N...>::template Permute<I0, I1, I...>;

    constexpr usize rank = sizeof...(N) + 2;
    pten result;

    return result;
}

template <typename T, usize N0, usize N1, usize... N> constexpr ten<T, N0, N1> Transpose(const ten<T, N0, N1> &p_Tensor)
{
}

// transpose
// inverse
// determinant
// tan
// acos
// asin
// atan
// abs
// ctors to N - 1

} // namespace TKit::Math
