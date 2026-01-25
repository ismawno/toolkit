#pragma once

#include "tkit/math/tensor.hpp"
#include <concepts>

namespace TKit::Math
{
using Detail::SquareRoot;

// TODO: Adapt so SIMD
template <typename T, usize N0, usize... N> constexpr ten<T, N0, N...> SquareRoot(const ten<T, N0, N...> &tensor)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = SquareRoot(tensor.Flat[i]);

    return result;
}

template <typename T> constexpr T Min(const std::initializer_list<T> values)
{
    return std::min(values);
}
template <typename T> constexpr T Max(const std::initializer_list<T> values)
{
    return std::max(values);
}

template <typename T> constexpr T Min(const T left, const T right)
{
    return std::min(left, right);
}
template <typename T> constexpr T Max(const T left, const T right)
{
    return std::max(left, right);
}
template <typename T> constexpr T Clamp(const T value, const T min, const T max)
{
    return std::clamp(value, min, max);
}

// TODO: Adapt so SIMD
template <typename T, usize N0, usize... N> constexpr T Min(const ten<T, N0, N...> &tensor)
{
    T mn{std::numeric_limits<T>::max()};
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        if (mn > tensor.Flat[i])
            mn = tensor.Flat[i];
    return mn;
}
template <typename T, usize N0, usize... N> constexpr T Max(const ten<T, N0, N...> &tensor)
{
    T mx{static_cast<T>(0)};
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        if (mx < tensor.Flat[i])
            mx = tensor.Flat[i];
    return mx;
}

template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Min(const ten<T, N0, N...> &left, const ten<T, N0, N...> &right)
{
    ten<T, N0, N...> mn;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        mn.Flat[i] = Min(left.Flat[i], right.Flat[i]);
    return mn;
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Max(const ten<T, N0, N...> &left, const ten<T, N0, N...> &right)
{
    ten<T, N0, N...> mx;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        mx.Flat[i] = Max(left.Flat[i], right.Flat[i]);
    return mx;
}
template <typename T, usize N0, usize... N>
constexpr ten<T, N0, N...> Clamp(const ten<T, N0, N...> &tensor, const ten<T, N0, N...> &min,
                                 const ten<T, N0, N...> &max)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = Clamp(tensor.Flat[i], min.Flat[i], max.Flat[i]);
    return result;
}
template <typename T, std::convertible_to<T> U, usize N0, usize... N>
constexpr ten<T, N0, N...> Clamp(const ten<T, N0, N...> &tensor, U &&min, U &&max)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] =
            Clamp(tensor.Flat[i], static_cast<T>(std::forward<U>(min)), static_cast<T>(std::forward<U>(max)));
    return result;
}

template <typename T> constexpr T Pi()
{
    return static_cast<T>(3.14159265358979323846);
}
template <typename T> constexpr T Radians(const T degrees)
{
    return degrees * (Pi<T>() / 180.f);
}
template <typename T> constexpr T Degrees(const T radians)
{
    return radians * (180.f / Pi<T>());
}
template <typename T> constexpr T Absolute(const T value)
{
    return std::abs(value);
}
template <typename T> constexpr T Cosine(const T value)
{
    return std::cos(value);
}
template <typename T> constexpr T Sine(const T value)
{
    return std::sin(value);
}
template <typename T> constexpr T Tangent(const T value)
{
    return std::tan(value);
}
template <typename T> constexpr T AntiCosine(const T value)
{
    return std::acos(value);
}
template <typename T> constexpr T AntiSine(const T value)
{
    return std::asin(value);
}
template <typename T> constexpr T AntiTangent(const T value)
{
    return std::atan(value);
}
template <typename T> constexpr T AntiTangent(const T y, const T x)
{
    return std::atan2(y, x);
}

#define CREATE_MATH_TENSOR_FUNC(name)                                                                                \
    template <typename T, usize N0, usize... N> constexpr ten<T, N0, N...> name(const ten<T, N0, N...> &tensor)    \
    {                                                                                                                  \
        ten<T, N0, N...> result;                                                                                       \
        constexpr usize size = N0 * (N * ... * 1);                                                                     \
        for (usize i = 0; i < size; ++i)                                                                               \
            result.Flat[i] = name(tensor.Flat[i]);                                                                 \
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
constexpr ten<T, N0, N...> AntiTangent(const ten<T, N0, N...> &y, const ten<T, N0, N...> &x)
{
    ten<T, N0, N...> sn;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        sn.Flat[i] = AntiTangent(y.Flat[i], x.Flat[i]);
    return sn;
}

// LU reduction
// determinant
// inverse

} // namespace TKit::Math
