#pragma once

#include "tkit/math/math.hpp"
#include "tkit/utils/limits.hpp"

namespace TKit::Math
{
template <Float T> struct Quaternion
{
    using ValueType = T;

    static constexpr usize Size = 4;

    constexpr Quaternion() = default;
    constexpr Quaternion(const Quaternion &) = default;

    template <std::convertible_to<T> U>
    constexpr Quaternion(const U w, const U x, const U y, const U z)
        : w(static_cast<T>(w)), x(static_cast<T>(x)), y(static_cast<T>(y)), z(static_cast<T>(z))
    {
    }
    constexpr Quaternion(const T w, const T x, const T y, const T z) : w(w), x(x), y(y), z(z)
    {
    }

    template <std::convertible_to<T> U>
    constexpr Quaternion(const Quaternion<U> &quaternion)
        : w(static_cast<T>(quaternion.w)), x(static_cast<T>(quaternion.x)), y(static_cast<T>(quaternion.y)),
          z(static_cast<T>(quaternion.z))

    {
    }

    constexpr explicit Quaternion(const vec3<T> &eulerAngles)
    {
        *this = FromEulerAngles(eulerAngles);
    }
    constexpr explicit Quaternion(const vec4<T> &vector)
        : w(vector[0]), x(vector[1]), y(vector[2]), z(vector[3])
    {
    }

    constexpr explicit Quaternion(const mat3<T> &matrix)
    {
        *this = FromMat3(matrix);
    }
    constexpr explicit Quaternion(const mat4<T> &matrix)
    {
        *this = FromMat4(matrix);
    }

    template <std::convertible_to<T> U>
    constexpr Quaternion(const vec3<T> &vector, const U value)
        : w(vector[0]), x(vector[1]), y(vector[2]), z(static_cast<T>(value))
    {
    }

    template <std::convertible_to<T> U>
    constexpr Quaternion(const U value, const vec3<T> &vector)
        : w(static_cast<T>(value)), x(vector[0]), y(vector[1]), z(vector[2])
    {
    }

    constexpr Quaternion &operator=(const Quaternion &) = default;

    template <std::convertible_to<T> U> constexpr Quaternion &operator=(const Quaternion<U> &quaternion)
    {
        w = static_cast<T>(quaternion.w);
        x = static_cast<T>(quaternion.x);
        y = static_cast<T>(quaternion.y);
        z = static_cast<T>(quaternion.z);
    }

    static constexpr Quaternion FromEulerAngles(const vec3<T> &eulerAngles)
    {
        Quaternion q;
        const vec3<T> half = static_cast<T>(0.5) * eulerAngles;
        const vec3<T> c = Cosine(half);
        const vec3<T> s = Sine(half);

        q.w = c[0] * c[1] * c[2] + s[0] * s[1] * s[2];
        q.x = s[0] * c[1] * c[2] - c[0] * s[1] * s[2];
        q.y = c[0] * s[1] * c[2] + s[0] * c[1] * s[2];
        q.z = c[0] * c[1] * s[2] - s[0] * s[1] * c[2];
        return q;
    }
    static constexpr Quaternion FromMat3(const mat3<T> &matrix)
    {
        const T fourXSquaredMinus1 = matrix[0][0] - matrix[1][1] - matrix[2][2];
        const T fourYSquaredMinus1 = matrix[1][1] - matrix[0][0] - matrix[2][2];
        const T fourZSquaredMinus1 = matrix[2][2] - matrix[0][0] - matrix[1][1];
        const T fourWSquaredMinus1 = matrix[0][0] + matrix[1][1] + matrix[2][2];

        usize biggestIndex = 0;
        T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
        if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourXSquaredMinus1;
            biggestIndex = 1;
        }
        if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourYSquaredMinus1;
            biggestIndex = 2;
        }
        if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourZSquaredMinus1;
            biggestIndex = 3;
        }

        const T biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<T>(1)) * static_cast<T>(0.5);
        const T mult = static_cast<T>(0.25) / biggestVal;

        switch (biggestIndex)
        {
        case 0:
            return Quaternion(biggestVal, (matrix[1][2] - matrix[2][1]) * mult,
                              (matrix[2][0] - matrix[0][2]) * mult, (matrix[0][1] - matrix[1][0]) * mult);
        case 1:
            return Quaternion((matrix[1][2] - matrix[2][1]) * mult, biggestVal,
                              (matrix[0][1] + matrix[1][0]) * mult, (matrix[2][0] + matrix[0][2]) * mult);
        case 2:
            return Quaternion((matrix[2][0] - matrix[0][2]) * mult, (matrix[0][1] + matrix[1][0]) * mult,
                              biggestVal, (matrix[1][2] + matrix[2][1]) * mult);
        case 3:
            return Quaternion((matrix[0][1] - matrix[1][0]) * mult, (matrix[2][0] + matrix[0][2]) * mult,
                              (matrix[1][2] + matrix[2][1]) * mult, biggestVal);
        default:
            TKIT_UNREACHABLE();
            break;
        }
    }

    static constexpr Quaternion FromMat4(const mat4<T> &matrix)
    {
        return FromMat3(mat3<T>{vec3<T>{matrix[0]}, vec3<T>{matrix[1]}, vec3<T>{matrix[2]}});
    }

    static constexpr Quaternion FromAngleAxis(const T angle, const vec3<T> &axis)
    {
        constexpr T half = static_cast<T>(0.5);
        const T s = Sine(angle * half);

        return Quaternion{Cosine(angle * half), axis * s};
    }

    constexpr const T *GetData() const
    {
        return &x;
    }
    constexpr T *GetData()
    {
        return &x;
    }

    constexpr const T &At(const usize index) const
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, Size, "[TOOLKIT][QUAT] ");
        return (&w)[index];
    }
    constexpr T &At(const usize index)
    {
        TKIT_CHECK_OUT_OF_BOUNDS(index, Size, "[TOOLKIT][QUAT] ");
        return (&w)[index];
    }

    constexpr const T &operator[](const usize index) const
    {
        return At(index);
    }
    constexpr T &operator[](const usize index)
    {
        return At(index);
    }

    constexpr explicit operator mat3<T>()
    {
        return ToMat3(*this);
    }
    constexpr explicit operator mat4<T>()
    {
        return ToMat4(*this);
    }
    constexpr explicit operator vec4<T>()
    {
        return vec4<T>{x, y, z, w};
    }
    constexpr explicit operator vec3<T>()
    {
        return vec3<T>{x, y, z};
    }

#define CREATE_ARITHMETIC_OP(op)                                                                                     \
    friend constexpr Quaternion operator op(const Quaternion &left, const Quaternion &right)                     \
    {                                                                                                                  \
        return Quaternion{left.w op right.w, left.x op right.x, left.y op right.y,                   \
                          left.z op right.z};                                                                    \
    }

    CREATE_ARITHMETIC_OP(+)
    CREATE_ARITHMETIC_OP(-)

    friend constexpr vec3<T> operator*(const Quaternion &left, const vec3<T> &right)
    {
        const vec3<T> q{left.x, left.y, left.z};
        const vec3<T> uv{Cross(q, right)};
        const vec3<T> uuv{Cross(q, uv)};

        return right + ((uv * left.w) + uuv) * static_cast<T>(2);
    }
    friend constexpr vec3<T> operator*(const vec3<T> &left, const Quaternion &right)
    {
        return Inverse(right) * left;
    }
    friend constexpr vec4<T> operator*(const Quaternion &left, const vec4<T> &right)
    {
        return vec4<T>{left * vec3<T>{right}, right[3]};
    }
    friend constexpr vec4<T> operator*(const vec4<T> &left, const Quaternion &right)
    {
        return Inverse(right) * left;
    }
    friend constexpr Quaternion operator*(const Quaternion &left, const Quaternion &right)
    {
        Quaternion result;
        result.w = left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z;
        result.x = left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y;
        result.y = left.w * right.y + left.y * right.w + left.z * right.x - left.x * right.z;
        result.z = left.w * right.z + left.z * right.w + left.x * right.y - left.y * right.x;
        return result;
    }

#define CREATE_SCALAR_OP(op)                                                                                         \
    friend constexpr Quaternion operator op(const Quaternion &left, const T right)                               \
    {                                                                                                                  \
        return Quaternion{left.w op right, left.x op right, left.y op right, left.z op right}; \
    }                                                                                                                  \
    friend constexpr Quaternion operator op(const T left, const Quaternion &right)                               \
    {                                                                                                                  \
        return Quaternion{left op right.w, left op right.x, left op right.y, left op right.z}; \
    }

    CREATE_SCALAR_OP(+)
    CREATE_SCALAR_OP(-)
    CREATE_SCALAR_OP(*)
    CREATE_SCALAR_OP(/)

#define CREATE_CMP_OP(op, cmp)                                                                                     \
    friend constexpr bool operator op(const Quaternion &left, const Quaternion &right)                           \
    {                                                                                                                  \
        return left.w op right.w cmp left.x op right.x cmp left.y op right.y cmp               \
            left.z op right.z;                                                                                   \
    }

    CREATE_CMP_OP(==, &&)
    CREATE_CMP_OP(!=, ||)

#define CREATE_SELF_OP(op)                                                                                           \
    constexpr Quaternion &operator op## = (const Quaternion &other)                                                \
    {                                                                                                                  \
        *this = *this op other;                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
    template <std::convertible_to<T> U> constexpr Quaternion &operator op## = (const U value)                      \
    {                                                                                                                  \
        *this = *this op value;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_SELF_OP(+)
    CREATE_SELF_OP(-)
    CREATE_SELF_OP(*)

    friend constexpr Quaternion operator-(const Quaternion &quaternion)
    {
        return {-quaternion.w, -quaternion.x, -quaternion.y, -quaternion.z};
    }

    T w, x, y, z;
};
} // namespace TKit::Math
namespace TKit
{
namespace Math
{
template <typename T> constexpr T Dot(const Quaternion<T> &left, const Quaternion<T> &right)
{
    return left.w * right.w + left.x * right.x + left.y * right.y + left.z * right.z;
}

template <typename T> constexpr T NormSquared(const Quaternion<T> &quaternion)
{
    return Dot(quaternion, quaternion);
}
template <typename T> constexpr T Norm(const Quaternion<T> &quaternion)
{
    return SquareRoot(Dot(quaternion, quaternion));
}
template <typename T> constexpr Quaternion<T> Normalize(const Quaternion<T> &quaternion)
{
    return quaternion / Norm(quaternion);
}

template <typename T> constexpr Quaternion<T> Conjugate(const Quaternion<T> &quaternion)
{
    return Quaternion<T>{quaternion.w, -quaternion.x, -quaternion.y, -quaternion.z};
}
template <typename T> constexpr Quaternion<T> Inverse(const Quaternion<T> &quaternion)
{
    return Conjugate(quaternion) / NormSquared(quaternion);
}

template <typename T> constexpr T Cross(const Quaternion<T> &left, const Quaternion<T> &right)
{
    return Quaternion<T>(left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z,
                         left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
                         left.w * right.y + left.y * right.w + left.z * right.x - left.x * right.z,
                         left.w * right.z + left.z * right.w + left.x * right.y - left.y * right.x);
}
template <typename T> constexpr T GetAngle(const Quaternion<T> &quaternion)
{
    constexpr T cosOneOverTwo = static_cast<T>(0.877582561890372716130286068203503191);
    constexpr T zero = static_cast<T>(0);
    constexpr T two = static_cast<T>(2);
    if (Absolute(quaternion.w) > cosOneOverTwo)
    {
        T const a = AntiSine(SquareRoot(quaternion.x * quaternion.x + quaternion.y * quaternion.y +
                                        quaternion.z * quaternion.z)) *
                    two;
        if (quaternion.w < zero)
            return Pi<T>() * two - a;
        return a;
    }

    return AntiCosine(quaternion.w) * two;
}

template <typename T> constexpr vec3<T> GetAxis(const Quaternion<T> &quaternion)
{
    constexpr T zero = static_cast<T>(0);
    constexpr T one = static_cast<T>(1);
    const T tmp1 = one - quaternion.w * quaternion.w;
    if (tmp1 <= zero)
        return vec3<T>{zero, zero, one};
    const T tmp2 = one / SquareRoot(tmp1);
    return vec3<T>{quaternion.x * tmp2, quaternion.y * tmp2, quaternion.z * tmp2};
}

template <typename T> constexpr T Pitch(const Quaternion<T> &quaternion)
{
    constexpr T two = static_cast<T>(2);
    const T y = two * (quaternion.y * quaternion.z + quaternion.w * quaternion.x);
    const T x = quaternion.w * quaternion.w - quaternion.x * quaternion.x - quaternion.y * quaternion.y +
                quaternion.z * quaternion.z;

    if (ApproachesZero(x) && ApproachesZero(y))
        return two * AntiTangent(quaternion.x, quaternion.w);
    return AntiTangent(y, x);
}
template <typename T> constexpr T Yaw(const Quaternion<T> &quaternion)
{
    return AntiSine(Clamp(static_cast<T>(-2) * (quaternion.x * quaternion.z - quaternion.w * quaternion.y),
                          static_cast<T>(-1), static_cast<T>(1)));
}
template <typename T> constexpr T Roll(const Quaternion<T> &quaternion)
{
    constexpr T two = static_cast<T>(2);
    const T y = two * (quaternion.x * quaternion.y + quaternion.w * quaternion.z);
    const T x = quaternion.w * quaternion.w + quaternion.x * quaternion.x - quaternion.y * quaternion.y -
                quaternion.z * quaternion.z;

    if (ApproachesZero(x) && ApproachesZero(y))
        return static_cast<T>(0);
    return AntiTangent(y, x);
}

template <typename T> constexpr vec3<T> ToEulerAngles(const Quaternion<T> &quaternion)
{
    return vec3<T>{Pitch(quaternion), Yaw(quaternion), Roll(quaternion)};
}

template <typename T> constexpr mat3<T> ToMat3(const Quaternion<T> &quaternion)
{
    mat3<T> result = mat3<T>::Identity();
    const T qxx(quaternion.x * quaternion.x);
    const T qyy(quaternion.y * quaternion.y);
    const T qzz(quaternion.z * quaternion.z);
    const T qxz(quaternion.x * quaternion.z);
    const T qxy(quaternion.x * quaternion.y);
    const T qyz(quaternion.y * quaternion.z);
    const T qwx(quaternion.w * quaternion.x);
    const T qwy(quaternion.w * quaternion.y);
    const T qwz(quaternion.w * quaternion.z);

    constexpr T one = static_cast<T>(1);
    constexpr T two = static_cast<T>(2);

    result[0][0] = one - two * (qyy + qzz);
    result[0][1] = two * (qxy + qwz);
    result[0][2] = two * (qxz - qwy);

    result[1][0] = two * (qxy - qwz);
    result[1][1] = one - two * (qxx + qzz);
    result[1][2] = two * (qyz + qwx);

    result[2][0] = two * (qxz + qwy);
    result[2][1] = two * (qyz - qwx);
    result[2][2] = one - two * (qxx + qyy);
    return result;
}
template <typename T> constexpr mat4<T> ToMat4(const Quaternion<T> &quaternion)
{
    const mat3<T> m3 = ToMat3(quaternion);
    return mat4<T>{vec4<T>{m3[0], 0.f}, vec4<T>{m3[1], 0.f}, vec4<T>{m3[2], 0.f}, vec4<T>{0.f, 0.f, 0.f, 1.f}};
}

} // namespace Math
namespace Alias
{
template <typename T> using qua = Math::Quaternion<T>;
using f32q = Math::Quaternion<f32>;
using f64q = Math::Quaternion<f64>;
} // namespace Alias
using namespace Alias;
} // namespace TKit
