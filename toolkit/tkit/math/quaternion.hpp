#pragma once

#include "tkit/math/math.hpp"
#include "tkit/utils/limits.hpp"

namespace TKit::Math
{
template <typename T> struct Quaternion
{
    using ValueType = T;

    static constexpr usize Length = 4;

    constexpr Quaternion() = default;
    constexpr Quaternion(const Quaternion &) = default;

    template <std::convertible_to<T> U>
    constexpr Quaternion(const U p_W, const U p_X, const U p_Y, const U p_Z)
        : w(static_cast<T>(p_W)), x(static_cast<T>(p_X)), y(static_cast<T>(p_Y)), z(static_cast<T>(p_Z))
    {
    }
    constexpr Quaternion(const T p_W, const T p_X, const T p_Y, const T p_Z) : w(p_W), x(p_X), y(p_Y), z(p_Z)
    {
    }

    template <std::convertible_to<T> U>
    constexpr Quaternion(const Quaternion<U> &p_Quaternion)
        : w(static_cast<T>(p_Quaternion.w)), x(static_cast<T>(p_Quaternion.x)), y(static_cast<T>(p_Quaternion.y)),
          z(static_cast<T>(p_Quaternion.z))

    {
    }

    constexpr explicit Quaternion(const vec3<T> &p_EulerAngles)
    {
        *this = FromEulerAngles(p_EulerAngles);
    }
    constexpr explicit Quaternion(const vec4<T> &p_Vector)
        : w(p_Vector[0]), x(p_Vector[1]), y(p_Vector[2]), z(p_Vector[3])
    {
    }

    constexpr explicit Quaternion(const mat3<T> &p_Matrix)
    {
        *this = FromMat3(p_Matrix);
    }
    constexpr explicit Quaternion(const mat4<T> &p_Matrix)
    {
        *this = FromMat4(p_Matrix);
    }

    template <std::convertible_to<T> U>
    constexpr Quaternion(const vec3<T> &p_Vector, const U p_Value)
        : w(p_Vector[0]), x(p_Vector[1]), y(p_Vector[2]), z(static_cast<T>(p_Value))
    {
    }

    template <std::convertible_to<T> U>
    constexpr Quaternion(const U p_Value, const vec3<T> &p_Vector)
        : w(static_cast<T>(p_Value)), x(p_Vector[0]), y(p_Vector[1]), z(p_Vector[2])
    {
    }

    constexpr Quaternion &operator=(const Quaternion &) = default;

    template <std::convertible_to<T> U> constexpr Quaternion &operator=(const Quaternion<U> &p_Quaternion)
    {
        w = static_cast<T>(p_Quaternion.w);
        x = static_cast<T>(p_Quaternion.x);
        y = static_cast<T>(p_Quaternion.y);
        z = static_cast<T>(p_Quaternion.z);
    }

    constexpr friend T Dot(const Quaternion &p_Left, const Quaternion &p_Right)
    {
        return p_Left.w * p_Right.w + p_Left.x * p_Right.x + p_Left.y * p_Right.y + p_Left.z * p_Right.z;
    }

    friend constexpr T NormSquared(const Quaternion &p_Quaternion)
    {
        return Dot(p_Quaternion, p_Quaternion);
    }
    friend constexpr T Norm(const Quaternion &p_Quaternion)
    {
        return SquareRoot(Dot(p_Quaternion, p_Quaternion));
    }
    friend constexpr Quaternion Normalize(const Quaternion &p_Quaternion)
    {
        return p_Quaternion / Norm(p_Quaternion);
    }

    constexpr friend Quaternion Conjugate(const Quaternion &p_Quaternion)
    {
        return Quaternion{p_Quaternion.w, -p_Quaternion.x, -p_Quaternion.y, -p_Quaternion.z};
    }
    constexpr friend Quaternion Inverse(const Quaternion &p_Quaternion)
    {
        return Conjugate(p_Quaternion) / NormSquared(p_Quaternion);
    }

    constexpr friend T Cross(const Quaternion &p_Left, const Quaternion &p_Right)
    {
        return Quaternion(p_Left.w * p_Right.w - p_Left.x * p_Right.x - p_Left.y * p_Right.y - p_Left.z * p_Right.z,
                          p_Left.w * p_Right.x + p_Left.x * p_Right.w + p_Left.y * p_Right.z - p_Left.z * p_Right.y,
                          p_Left.w * p_Right.y + p_Left.y * p_Right.w + p_Left.z * p_Right.x - p_Left.x * p_Right.z,
                          p_Left.w * p_Right.z + p_Left.z * p_Right.w + p_Left.x * p_Right.y - p_Left.y * p_Right.x);
    }

    static constexpr Quaternion FromEulerAngles(const vec3<T> &p_EulerAngles)
    {
        Quaternion q;
        const vec3<T> half = static_cast<T>(0.5) * p_EulerAngles;
        const vec3<T> c = Cosine(half);
        const vec3<T> s = Sine(half);

        q.w = c[0] * c[1] * c[2] + s[0] * s[1] * s[2];
        q.x = s[0] * c[1] * c[2] - c[0] * s[1] * s[2];
        q.y = c[0] * s[1] * c[2] + s[0] * c[1] * s[2];
        q.z = c[0] * c[1] * s[2] - s[0] * s[1] * c[2];
        return q;
    }
    static constexpr Quaternion FromMat3(const mat3<T> &p_Matrix)
    {
        const T fourXSquaredMinus1 = p_Matrix[0][0] - p_Matrix[1][1] - p_Matrix[2][2];
        const T fourYSquaredMinus1 = p_Matrix[1][1] - p_Matrix[0][0] - p_Matrix[2][2];
        const T fourZSquaredMinus1 = p_Matrix[2][2] - p_Matrix[0][0] - p_Matrix[1][1];
        const T fourWSquaredMinus1 = p_Matrix[0][0] + p_Matrix[1][1] + p_Matrix[2][2];

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
            return Quaternion(biggestVal, (p_Matrix[1][2] - p_Matrix[2][1]) * mult,
                              (p_Matrix[2][0] - p_Matrix[0][2]) * mult, (p_Matrix[0][1] - p_Matrix[1][0]) * mult);
        case 1:
            return Quaternion((p_Matrix[1][2] - p_Matrix[2][1]) * mult, biggestVal,
                              (p_Matrix[0][1] + p_Matrix[1][0]) * mult, (p_Matrix[2][0] + p_Matrix[0][2]) * mult);
        case 2:
            return Quaternion((p_Matrix[2][0] - p_Matrix[0][2]) * mult, (p_Matrix[0][1] + p_Matrix[1][0]) * mult,
                              biggestVal, (p_Matrix[1][2] + p_Matrix[2][1]) * mult);
        case 3:
            return Quaternion((p_Matrix[0][1] - p_Matrix[1][0]) * mult, (p_Matrix[2][0] + p_Matrix[0][2]) * mult,
                              (p_Matrix[1][2] + p_Matrix[2][1]) * mult, biggestVal);
        default:
            TKIT_UNREACHABLE();
            break;
        }
    }

    static constexpr Quaternion FromMat4(const mat4<T> &p_Matrix)
    {
        return FromMat3(mat3<T>{vec3<T>{p_Matrix[0]}, vec3<T>{p_Matrix[1]}, vec3<T>{p_Matrix[2]}});
    }

    static constexpr Quaternion FromAngleAxis(const T p_Angle, const vec3<T> &p_Axis)
    {
        constexpr T half = static_cast<T>(0.5);
        const T s = Sine(p_Angle * half);

        return Quaternion{Cosine(p_Angle * half), p_Axis * s};
    }

    friend constexpr T GetAngle(const Quaternion &p_Quaternion)
    {
        constexpr T cosOneOverTwo = static_cast<T>(0.877582561890372716130286068203503191);
        constexpr T zero = static_cast<T>(0);
        constexpr T two = static_cast<T>(2);
        if (Absolute(p_Quaternion.w) > cosOneOverTwo)
        {
            T const a = AntiSine(SquareRoot(p_Quaternion.x * p_Quaternion.x + p_Quaternion.y * p_Quaternion.y +
                                            p_Quaternion.z * p_Quaternion.z)) *
                        two;
            if (p_Quaternion.w < zero)
                return Pi<T>() * two - a;
            return a;
        }

        return AntiCosine(p_Quaternion.w) * two;
    }

    friend constexpr vec3<T> GetAxis(const Quaternion &p_Quaternion)
    {
        constexpr T zero = static_cast<T>(0);
        constexpr T one = static_cast<T>(1);
        const T tmp1 = one - p_Quaternion.w * p_Quaternion.w;
        if (tmp1 <= zero)
            return vec3<T>{zero, zero, one};
        const T tmp2 = one / SquareRoot(tmp1);
        return vec3<T>{p_Quaternion.x * tmp2, p_Quaternion.y * tmp2, p_Quaternion.z * tmp2};
    }

    friend constexpr T Pitch(const Quaternion &p_Quaternion)
    {
        constexpr T two = static_cast<T>(2);
        const T y = two * (p_Quaternion.y * p_Quaternion.z + p_Quaternion.w * p_Quaternion.x);
        const T x = p_Quaternion.w * p_Quaternion.w - p_Quaternion.x * p_Quaternion.x -
                    p_Quaternion.y * p_Quaternion.y + p_Quaternion.z * p_Quaternion.z;

        if (ApproachesZero(x) && ApproachesZero(y))
            return two * AntiTangent(p_Quaternion.x, p_Quaternion.w);
        return AntiTangent(y, x);
    }
    friend constexpr T Yaw(const Quaternion &p_Quaternion)
    {
        return AntiSine(Clamp(static_cast<T>(-2) * (p_Quaternion.x * p_Quaternion.z - p_Quaternion.w * p_Quaternion.y),
                              static_cast<T>(-1), static_cast<T>(1)));
    }
    friend constexpr T Roll(const Quaternion &p_Quaternion)
    {
        constexpr T two = static_cast<T>(2);
        const T y = two * (p_Quaternion.x * p_Quaternion.y + p_Quaternion.w * p_Quaternion.z);
        const T x = p_Quaternion.w * p_Quaternion.w + p_Quaternion.x * p_Quaternion.x -
                    p_Quaternion.y * p_Quaternion.y - p_Quaternion.z * p_Quaternion.z;

        if (ApproachesZero(x) && ApproachesZero(y))
            return static_cast<T>(0);
        return AntiTangent(y, x);
    }

    friend constexpr vec3<T> ToEulerAngles(const Quaternion &p_Quaternion)
    {
        return vec3<T>{Pitch(p_Quaternion), Yaw(p_Quaternion), Roll(p_Quaternion)};
    }

    friend constexpr mat3<T> ToMat3(const Quaternion &p_Quaternion)
    {
        mat3<T> result = mat3<T>::Identity();
        const T qxx(p_Quaternion.x * p_Quaternion.x);
        const T qyy(p_Quaternion.y * p_Quaternion.y);
        const T qzz(p_Quaternion.z * p_Quaternion.z);
        const T qxz(p_Quaternion.x * p_Quaternion.z);
        const T qxy(p_Quaternion.x * p_Quaternion.y);
        const T qyz(p_Quaternion.y * p_Quaternion.z);
        const T qwx(p_Quaternion.w * p_Quaternion.x);
        const T qwy(p_Quaternion.w * p_Quaternion.y);
        const T qwz(p_Quaternion.w * p_Quaternion.z);

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

    friend constexpr mat4<T> ToMat4(const Quaternion &p_Quaternion)
    {
        const mat3<T> m3 = ToMat3(p_Quaternion);
        return mat4<T>{vec4<T>{m3[0], 0.f}, vec4<T>{m3[1], 0.f}, vec4<T>{m3[2], 0.f}, vec4<T>{0.f, 0.f, 0.f, 1.f}};
    }

    constexpr const T *GetData() const
    {
        return &x;
    }
    constexpr T *GetData()
    {
        return &x;
    }

    constexpr const T &At(const usize p_Index) const
    {
        TKIT_ASSERT(p_Index < 4, "[TOOLKIT][QUAT] Index is out of bounds");
        return (&w)[p_Index];
    }
    constexpr T &At(const usize p_Index)
    {
        TKIT_ASSERT(p_Index < 4, "[TOOLKIT][QUAT] Index is out of bounds");
        return (&w)[p_Index];
    }

    constexpr const T &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr T &operator[](const usize p_Index)
    {
        return At(p_Index);
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

#define CREATE_ARITHMETIC_OP(p_Op)                                                                                     \
    friend constexpr Quaternion operator p_Op(const Quaternion &p_Left, const Quaternion &p_Right)                     \
    {                                                                                                                  \
        return Quaternion{p_Left.w p_Op p_Right.w, p_Left.x p_Op p_Right.x, p_Left.y p_Op p_Right.y,                   \
                          p_Left.z p_Op p_Right.z};                                                                    \
    }

    CREATE_ARITHMETIC_OP(+)
    CREATE_ARITHMETIC_OP(-)

    friend constexpr vec3<T> operator*(const Quaternion &p_Left, const vec3<T> &p_Right)
    {
        const vec3<T> q{p_Left.x, p_Left.y, p_Left.z};
        const vec3<T> uv{Cross(q, p_Right)};
        const vec3<T> uuv{Cross(q, uv)};

        return p_Right + ((uv * p_Left.w) + uuv) * static_cast<T>(2);
    }
    friend constexpr vec3<T> operator*(const vec3<T> &p_Left, const Quaternion &p_Right)
    {
        return Inverse(p_Right) * p_Left;
    }
    friend constexpr vec4<T> operator*(const Quaternion &p_Left, const vec4<T> &p_Right)
    {
        return vec4<T>{p_Left * vec3<T>{p_Right}, p_Right[3]};
    }
    friend constexpr vec4<T> operator*(const vec4<T> &p_Left, const Quaternion &p_Right)
    {
        return Inverse(p_Right) * p_Left;
    }
    friend constexpr Quaternion operator*(const Quaternion &p_Left, const Quaternion &p_Right)
    {
        Quaternion result;
        result.w = p_Left.w * p_Right.w - p_Left.x * p_Right.x - p_Left.y * p_Right.y - p_Left.z * p_Right.z;
        result.x = p_Left.w * p_Right.x + p_Left.x * p_Right.w + p_Left.y * p_Right.z - p_Left.z * p_Right.y;
        result.y = p_Left.w * p_Right.y + p_Left.y * p_Right.w + p_Left.z * p_Right.x - p_Left.x * p_Right.z;
        result.z = p_Left.w * p_Right.z + p_Left.z * p_Right.w + p_Left.x * p_Right.y - p_Left.y * p_Right.x;
        return result;
    }

#define CREATE_SCALAR_OP(p_Op)                                                                                         \
    friend constexpr Quaternion operator p_Op(const Quaternion &p_Left, const T p_Right)                               \
    {                                                                                                                  \
        return Quaternion{p_Left.w p_Op p_Right, p_Left.x p_Op p_Right, p_Left.y p_Op p_Right, p_Left.z p_Op p_Right}; \
    }                                                                                                                  \
    friend constexpr Quaternion operator p_Op(const T p_Left, const Quaternion &p_Right)                               \
    {                                                                                                                  \
        return Quaternion{p_Left p_Op p_Right.w, p_Left p_Op p_Right.x, p_Left p_Op p_Right.y, p_Left p_Op p_Right.z}; \
    }

    CREATE_SCALAR_OP(+)
    CREATE_SCALAR_OP(-)
    CREATE_SCALAR_OP(*)
    CREATE_SCALAR_OP(/)

#define CREATE_SELF_OP(p_Op)                                                                                           \
    constexpr Quaternion &operator p_Op##=(const Quaternion & p_Other)                                                 \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_SELF_OP(+)
    CREATE_SELF_OP(-)
    CREATE_SELF_OP(*)

    friend constexpr Quaternion operator-(const Quaternion &p_Quaternion)
    {
        return {-p_Quaternion.w, -p_Quaternion.x, -p_Quaternion.y, -p_Quaternion.z};
    }

    T w, x, y, z;
};
} // namespace TKit::Math
namespace TKit
{
namespace Alias
{
template <typename T> using qua = Math::Quaternion<T>;
using f32q = Math::Quaternion<f32>;
using f64q = Math::Quaternion<f64>;
} // namespace Alias
using namespace Alias;
} // namespace TKit
