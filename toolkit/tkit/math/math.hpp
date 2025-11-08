#pragma once

#include "tkit/math/tensor.hpp"
#include <concepts>

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

template <typename T> constexpr T SquareRoot(const T p_Value)
{
    return std::sqrt(p_Value);
}

// TODO: Adapt so SIMD
template <typename T, usize N0, usize... N> constexpr ten<T, N0, N...> SquareRoot(const ten<T, N0, N...> &p_Tensor)
{
    ten<T, N0, N...> result;
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        result.Flat[i] = SquareRoot(p_Tensor.Flat[i]);

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

// TODO: Adapt so SIMD
template <typename T, usize N0, usize... N> constexpr T Min(const ten<T, N0, N...> &p_Tensor)
{
    T mn{std::numeric_limits<T>::max()};
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        if (mn > p_Tensor.Flat[i])
            mn = p_Tensor.Flat[i];
    return mn;
}
template <typename T, usize N0, usize... N> constexpr T Max(const ten<T, N0, N...> &p_Tensor)
{
    T mx{static_cast<T>(0)};
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        if (mx < p_Tensor.Flat[i])
            mx = p_Tensor.Flat[i];
    return mx;
}

template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Min(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    ten<T, N0, N...> mn;
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        mn.Flat[i] = Min(p_Left.Flat[i], p_Right.Flat[i]);
    return mn;
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Max(const ten<T, N0, N...> &p_Left, const ten<T, N0, N...> &p_Right)
{
    ten<T, N0, N...> mx;
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        mx.Flat[i] = Max(p_Left.Flat[i], p_Right.Flat[i]);
    return mx;
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Clamp(const ten<T, N0, N...> &p_Tensor, const ten<T, N0, N...> &p_Min,
                                 const ten<T, N0, N...> &p_Max)
{
    ten<T, N0, N...> result;
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        result.Flat[i] = Clamp(p_Tensor.Flat[i], p_Min.Flat[i], p_Max.Flat[i]);
    return result;
}
template <typename T, std::convertible_to<T> U, usize N0, usize... N>
constexpr ten<T, N0, N...> Clamp(const ten<T, N0, N...> &p_Tensor, U &&p_Min, U &&p_Max)
{
    ten<T, N0, N...> result;
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        result.Flat[i] =
            Clamp(p_Tensor.Flat[i], static_cast<T>(std::forward<U>(p_Min)), static_cast<T>(std::forward<U>(p_Max)));
    return result;
}

template <typename T> constexpr T Pi()
{
    return static_cast<T>(3.14159265358979323846);
}
template <typename T> constexpr T Radians(const T p_Degrees)
{
    return p_Degrees * (Pi<T>() / 180.f);
}
template <typename T> constexpr T Degrees(const T p_Radians)
{
    return p_Radians * (180.f / Pi<T>());
}
template <typename T> constexpr T Absolute(const T p_Value)
{
    return std::abs(p_Value);
}
template <typename T> constexpr T Cosine(const T p_Value)
{
    return std::cos(p_Value);
}
template <typename T> constexpr T Sine(const T p_Value)
{
    return std::sin(p_Value);
}
template <typename T> constexpr T Tangent(const T p_Value)
{
    return std::tan(p_Value);
}
template <typename T> constexpr T AntiCosine(const T p_Value)
{
    return std::acos(p_Value);
}
template <typename T> constexpr T AntiSine(const T p_Value)
{
    return std::asin(p_Value);
}
template <typename T> constexpr T AntiTangent(const T p_Value)
{
    return std::atan(p_Value);
}
template <typename T> constexpr T AntiTangent(const T p_Y, const T p_X)
{
    return std::atan2(p_Y, p_X);
}

#define CREATE_MATH_TENSOR_FUNC(p_Name)                                                                                \
    template <typename T, usize N0, usize... N> constexpr ten<T, N0, N...> p_Name(const ten<T, N0, N...> &p_Tensor)    \
    {                                                                                                                  \
        ten<T, N0, N...> result;                                                                                       \
        constexpr usize length = N0 * (N * ... * 1);                                                                   \
        for (usize i = 0; i < length; ++i)                                                                             \
            result.Flat[i] = p_Name(p_Tensor.Flat[i]);                                                                 \
        return result;                                                                                                 \
    }

CREATE_MATH_TENSOR_FUNC(Radians)
CREATE_MATH_TENSOR_FUNC(Degrees)
CREATE_MATH_TENSOR_FUNC(Absolute)
CREATE_MATH_TENSOR_FUNC(Cosine)
CREATE_MATH_TENSOR_FUNC(Sine)
CREATE_MATH_TENSOR_FUNC(Tangent)
CREATE_MATH_TENSOR_FUNC(AntiCosine)
CREATE_MATH_TENSOR_FUNC(AntiSine)
CREATE_MATH_TENSOR_FUNC(AntiTangent)

template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> AntiTangent(const ten<T, N0, N...> &p_Y, const ten<T, N0, N...> &p_X)
{
    ten<T, N0, N...> sn;
    constexpr usize length = N0 * (N * ... * 1);
    for (usize i = 0; i < length; ++i)
        sn.Flat[i] = AntiTangent(p_Y.Flat[i], p_X.Flat[i]);
    return sn;
}

// LU reduction
// determinant
// inverse

} // namespace TKit::Math
