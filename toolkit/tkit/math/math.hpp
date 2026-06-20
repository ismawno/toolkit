#pragma once

#include "tkit/math/tensor.hpp"
#include "tkit/utils/limits.hpp"
#include <concepts>
#include <cmath>

namespace TKit::Math
{
using Detail::SquareRoot;

// TODO: Adapt so SIMD
template <Float T, usize N0, usize... N> constexpr ten<T, N0, N...> SquareRoot(const ten<T, N0, N...> &tensor)
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

template <Float T> constexpr T Power(const T value, const T power)
{
    return std::pow(value, power);
}
template <Float T> constexpr T Log(const T value)
{
    return std::log(value);
}
template <Float T> constexpr T Log(const T value, const T base)
{
    return Log(value) / Log(base);
}
template <Float T> constexpr T Round(const T value)
{
    return std::round(value);
}
template <Float T> constexpr T Round(const T value, const T step)
{
    return Round(value / step) * step;
}
template <Float T> constexpr T Round(const T value, const u32 decimals)
{
    const T scale = Power(T(10), T(decimals));
    return Round(value * scale) / scale;
}
template <Float T> constexpr T LinearLerp(const T v0, const T v1, const T t)
{
    return v0 + (v1 - v0) * t;
}
template <Float T> constexpr T InverseLinearLerp(const T v0, const T v1, const T v)
{
    return (v - v0) / (v1 - v0);
}
template <Float T> constexpr T LogLerp(const T v0, const T v1, const T t)
{
    const T offset = T(1) - Min(v0, v1);
    const T a = v0 + offset;
    const T b = v1 + offset;
    return a * Power(b / a, t) - offset;
}
template <Float T> constexpr T InverseLogLerp(const T v0, const T v1, const T value)
{
    const T offset = T(1) - Min(v0, v1);
    const T a = v0 + offset;
    const T b = v1 + offset;
    return Log((value + offset) / a, b / a);
}

template <typename T> constexpr T LinearMap(const T value, const T min0, const T max0, const T min1, const T max1)
{
    const T t = InverseLinearLerp(min0, max0, value);
    return LinearLerp(min1, max1, t);
}
template <typename T> constexpr T LogMap(const T value, const T min0, const T max0, const T min1, const T max1)
{
    const T t = InverseLinearLerp(min0, max0, value);
    return LogLerp(min1, max1, t);
}
template <typename T> constexpr T InverseLogMap(const T value, const T min0, const T max0, const T min1, const T max1)
{
    const T t = InverseLogLerp(min0, max0, value);
    return LinearLerp(min1, max1, t);
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
    T mx{T(0)};
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
constexpr ten<T, N0, N...> Clamp(const ten<T, N0, N...> &tensor, const U min, const U max)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = Clamp(tensor.Flat[i], T(min), T(max));
    return result;
}

template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> Power(const ten<T, N0, N...> &tensor, const ten<T, N0, N...> &power)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = Power(tensor.Flat[i], power.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N> constexpr ten<T, N0, N...> Power(const ten<T, N0, N...> &tensor, const T power)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = Power(tensor.Flat[i], power);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LinearLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1,
                                      const ten<T, N0, N...> &t)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = LinearLerp(tensor0.Flat[i], tensor1.Flat[i], t.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LinearLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1, const T t)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = LinearLerp(tensor0.Flat[i], tensor1.Flat[i], t);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LinearLerp(const T v0, const T v1, const ten<T, N0, N...> &t)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = LinearLerp(v0, v1, t.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLinearLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1,
                                             const ten<T, N0, N...> &v)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = InverseLinearLerp(tensor0.Flat[i], tensor1.Flat[i], v.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLinearLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1,
                                             const T v)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = InverseLinearLerp(tensor0.Flat[i], tensor1.Flat[i], v);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLinearLerp(const T v0, const T v1, const ten<T, N0, N...> &v)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = InverseLinearLerp(v0, v1, v.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LogLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1,
                                   const ten<T, N0, N...> &t)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = LogLerp(tensor0.Flat[i], tensor1.Flat[i], t.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LogLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1, const T t)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = LogLerp(tensor0.Flat[i], tensor1.Flat[i], t);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LogLerp(const T v0, const T v1, const ten<T, N0, N...> &t)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = LogLerp(v0, v1, t.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLogLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1,
                                          const ten<T, N0, N...> &v)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = InverseLogLerp(tensor0.Flat[i], tensor1.Flat[i], v.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLogLerp(const ten<T, N0, N...> &tensor0, const ten<T, N0, N...> &tensor1, const T v)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = InverseLogLerp(tensor0.Flat[i], tensor1.Flat[i], v);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLogLerp(const T v0, const T v1, const ten<T, N0, N...> &v)
{
    ten<T, N0, N...> result;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        result.Flat[i] = InverseLogLerp(v0, v1, v.Flat[i]);
    return result;
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LinearMap(const ten<T, N0, N...> &tensor, const ten<T, N0, N...> &min0,
                                     const ten<T, N0, N...> &max0, const ten<T, N0, N...> &min1,
                                     const ten<T, N0, N...> &max1)
{
    return LinearLerp(min1, max1, (tensor - min0) / (max0 - min0));
}
template <Float T, std::convertible_to<T> U, usize N0, usize... N>
constexpr ten<T, N0, N...> LinearMap(const ten<T, N0, N...> &tensor, const U min0, const U max0, const U min1,
                                     const U max1)
{
    return LinearLerp(min1, max1, (tensor - min0) / (max0 - min0));
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> LogMap(const ten<T, N0, N...> &tensor, const ten<T, N0, N...> &min0,
                                  const ten<T, N0, N...> &max0, const ten<T, N0, N...> &min1,
                                  const ten<T, N0, N...> &max1)
{
    return LogMap(min1, max1, (tensor - min0) / (max0 - min0));
}
template <Float T, std::convertible_to<T> U, usize N0, usize... N>
constexpr ten<T, N0, N...> LogMap(const ten<T, N0, N...> &tensor, const U min0, const U max0, const U min1,
                                  const U max1)
{
    return LogMap(min1, max1, (tensor - min0) / (max0 - min0));
}
template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLogMap(const ten<T, N0, N...> &tensor, const ten<T, N0, N...> &min0,
                                         const ten<T, N0, N...> &max0, const ten<T, N0, N...> &min1,
                                         const ten<T, N0, N...> &max1)
{
    return InverseLogMap(min1, max1, (tensor - min0) / (max0 - min0));
}
template <Float T, std::convertible_to<T> U, usize N0, usize... N>
constexpr ten<T, N0, N...> InverseLogMap(const ten<T, N0, N...> &tensor, const U min0, const U max0, const U min1,
                                         const U max1)
{
    return InverseLogMap(min1, max1, (tensor - min0) / (max0 - min0));
}

template <Float T = f32> constexpr T Pi()
{
    return T(3.14159265358979323846);
}
template <Float T> constexpr T Radians(const T degrees)
{
    return degrees * (Pi<T>() / 180.f);
}
template <Float T> constexpr T Degrees(const T radians)
{
    return radians * (180.f / Pi<T>());
}
template <typename T> constexpr T Absolute(const T value)
{
    return std::abs(value);
}
template <Float T> constexpr T Cosine(const T value)
{
    return std::cos(value);
}
template <Float T> constexpr T Sine(const T value)
{
    return std::sin(value);
}
template <Float T> constexpr T Tangent(const T value)
{
    return std::tan(value);
}
template <Float T> constexpr T AntiCosine(const T value)
{
    return std::acos(value);
}
template <Float T> constexpr T AntiSine(const T value)
{
    return std::asin(value);
}
template <Float T> constexpr T AntiTangent(const T value)
{
    return std::atan(value);
}
template <Float T> constexpr T AntiTangent(const T y, const T x)
{
    return std::atan2(y, x);
}

#define CREATE_MATH_TENSOR_FUNC(name)                                                                                  \
    template <Float T, usize N0, usize... N> constexpr ten<T, N0, N...> name(const ten<T, N0, N...> &tensor)           \
    {                                                                                                                  \
        ten<T, N0, N...> result;                                                                                       \
        constexpr usize size = N0 * (N * ... * 1);                                                                     \
        for (usize i = 0; i < size; ++i)                                                                               \
            result.Flat[i] = name(tensor.Flat[i]);                                                                     \
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

template <Float T, usize N0, usize... N>
constexpr ten<T, N0, N...> AntiTangent(const ten<T, N0, N...> &y, const ten<T, N0, N...> &x)
{
    ten<T, N0, N...> sn;
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        sn.Flat[i] = AntiTangent(y.Flat[i], x.Flat[i]);
    return sn;
}

template <Float T> constexpr bool ApproachesZero(const T value, const T epsilon = Limits<T>::Epsilon())
{
    if constexpr (Float<T>)
        return Absolute(value) <= epsilon;
    else
        return value == T(0);
}

template <Float T> constexpr bool Approximately(const T left, const T right, const T epsilon = Limits<T>::Epsilon())
{
    return ApproachesZero(left - right, epsilon);
}

template <Float T, usize N0, usize... N>
constexpr bool ApproachesZero(const ten<T, N0, N...> &tensor, const T epsilon = Limits<T>::Epsilon())
{
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        if (!ApproachesZero(tensor[i], epsilon))
            return false;
    return true;
}
template <Float T, usize N0, usize... N>
constexpr bool Approximately(const ten<T, N0, N...> &lhs, const ten<T, N0, N...> &rhs,
                             const T epsilon = Limits<T>::Epsilon())
{
    constexpr usize size = (N0 * ... * N);
    for (usize i = 0; i < size; ++i)
        if (!Approximately(lhs[i], rhs[i], epsilon))
            return false;
    return true;
}

// LU reduction
// determinant
// inverse

} // namespace TKit::Math
