#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/utils/logging.hpp"
#include "tkit/container/array.hpp"
#include <math.h>

namespace TKit
{
namespace Math
{
template <typename T, usize N0, usize... N>
// requires((N0 > 0) && ((N > 0) && ... && true))
struct Tensor;

namespace Detail
{
template <usize I, usize N0, usize... N> constexpr usize GetAxis()
{
    static_assert(I < sizeof...(N) + 1, "[TOOLKIT][TENSOR] Axis index exceeds rank of tensor");
    if constexpr (I == 0)
        return N0;
    else
        return GetAxis<I - 1, N...>();
}
template <typename T, usize... N> struct Child
{
    using Type = Tensor<T, N...>;
};
template <typename T> struct Child<T>
{
    using Type = T;
};

} // namespace Detail

template <typename T, usize N0, usize... N>
// requires((N0 > 0) && ((N > 0) && ... && true))
struct Tensor
{
    using ValueType = T;
    using ChildType = Detail::Child<T, N...>::Type;

    static constexpr usize Length = N0 * (N * ... * 1);
    static constexpr usize Rank = sizeof...(N) + 1;
    template <usize I> static constexpr usize Axis = Detail::GetAxis<I, N0, N...>();

    template <usize I0, usize... I>
        requires(sizeof...(I) == sizeof...(N) && I0 < N0 && ((I < N) && ... && true))
    using Permuted = Tensor<T, Axis<I0>, Axis<I>...>;

    constexpr Tensor() = default;

    template <std::convertible_to<T> U> explicit constexpr Tensor(U &&p_Value)
    {
        for (usize i = 0; i < Length; ++i)
            Flat[i] = static_cast<T>(std::forward<U>(p_Value));
    }

    template <typename... Args>
        requires(sizeof...(Args) == Length && (std::convertible_to<Args, T> && ...))
    explicit constexpr Tensor(Args &&...p_Args) : Flat{static_cast<T>(std::forward<Args>(p_Args))...}
    {
    }
    template <typename... Args>
    explicit constexpr Tensor(Args &&...p_Args)
        requires(!std::same_as<ChildType, T> && sizeof...(Args) == N0 && (std::convertible_to<Args, ChildType> && ...))
        : Ranked{static_cast<ChildType>(std::forward<Args>(p_Args))...}
    {
    }

    template <typename U>
        requires(std::convertible_to<U, ChildType>)
    constexpr Tensor(const Tensor<T, N0 - 1, N...> &p_Tensor, U &&p_Value)
        requires(N0 > 1)
    {
        for (usize i = 0; i < N0 - 1; ++i)
            Ranked[i] = p_Tensor[i];
        Ranked[N0 - 1] = p_Value;
    }

    constexpr T Dot(const Tensor &p_Left, const Tensor &p_Right)
    {
        T result{static_cast<T>(0)};

        constexpr usize length = N0 * (N * ...);
        for (usize i = 0; i < length; ++i)
            result += p_Left.Flat[i] * p_Right.Flat[i];
        return result;
    }

    constexpr T NormSquared(const Tensor &p_Tensor)
    {
        return Dot(p_Tensor, p_Tensor);
    }
    constexpr T Norm(const Tensor &p_Tensor)
    {
        return std::sqrt(Dot(p_Tensor, p_Tensor));
    }

    constexpr T DistanceSquared(const Tensor &p_Left, const Tensor &p_Right)
    {
        const Tensor diff = p_Right - p_Left;
        return Dot(diff, diff);
    }

    constexpr T Distance(const Tensor &p_Left, const Tensor &p_Right)
    {
        return std::sqrt(DistanceSquared(p_Left, p_Right));
    }

    constexpr Tensor Normalize(const Tensor &p_Tensor)
    {
        return p_Tensor / Norm(p_Tensor);
    }

    template <usize R0, usize... R>
        requires(R0 *(R *... * 1) == Length)
    constexpr Tensor<T, R0, R...> Reshape() const
    {
        Tensor<T, R0, R...> reshaped;
        for (usize i = 0; i < Length; ++i)
            reshaped.Flat[i] = Flat[i];
        return reshaped;
    }

    template <typename I0, typename... I>
        requires(sizeof...(I) == sizeof...(N) && std::convertible_to<I0, usize> &&
                 (std::convertible_to<I, usize> && ... && true))
    constexpr Tensor<T, N0 - 1, (N - 1)...> SubTensor(I0 &&p_First, I &&...p_Rest)
        requires(N0 > 1 && ((N0 == N) && ... && true))
    {
        TKIT_ASSERT(p_First < N0, "[TOOLKIT][TENSOR] Index is out of bounds");
        Tensor<T, N0 - 1, (N - 1)...> minor;
        for (usize i = 0, j = 0; i < N0; ++i)
            if (i != p_First)
            {
                if constexpr (sizeof...(I) == 0)
                    minor.Flat[j] = Flat[i];
                else
                    minor.Ranked[j] = Ranked[i].SubTensor(std::forward<I>(p_Rest)...);
                ++j;
            }
        return minor;
    }

    constexpr Tensor<T, N0, N...> Cofactors()
        requires(Rank == 2 && ((N0 == N) && ...))
    {
        Tensor<T, N0, N...> cofactors;
        i32 sign = 1;
        for (usize i = 0; i < N0; ++i)
            for (usize j = 0; j < Tensor<T, N...>::Length; ++j)
            {
                cofactors.Ranked[i][j] = sign * SubTensor(i, j).Determinant();
                sign *= -1;
            }
        return cofactors;
    }

    constexpr T Determinant()
        requires(Rank == 2 && ((N0 == N) && ...))
    {
        if constexpr (N0 == 1)
            return Flat[0];
        else if constexpr (N0 == 2)
            return Flat[0] * Flat[3] - Flat[1] * Flat[2];
        else if constexpr (N0 == 3)
            return Flat[0] * Flat[4] * Flat[8] + Flat[3] * Flat[7] * Flat[2] + Flat[1] * Flat[5] * Flat[6] -
                   Flat[6] * Flat[4] * Flat[2] - Flat[1] * Flat[3] * Flat[8] - Flat[0] * Flat[7] * Flat[5];
        else if constexpr (N0 == 4)
        {
            const T factor0 = Ranked[2][2] * Ranked[3][3] - Ranked[3][2] * Ranked[2][3];
            const T factor1 = Ranked[2][1] * Ranked[3][3] - Ranked[3][1] * Ranked[2][3];
            const T factor2 = Ranked[2][1] * Ranked[3][2] - Ranked[3][1] * Ranked[2][2];
            const T factor3 = Ranked[2][0] * Ranked[3][3] - Ranked[3][0] * Ranked[2][3];
            const T factor4 = Ranked[2][0] * Ranked[3][2] - Ranked[3][0] * Ranked[2][2];
            const T factor5 = Ranked[2][0] * Ranked[3][1] - Ranked[3][0] * Ranked[2][1];

            const Tensor<T, 4> coef(+(Ranked[1][1] * factor0 - Ranked[1][2] * factor1 + Ranked[1][3] * factor2),
                                    -(Ranked[1][0] * factor0 - Ranked[1][2] * factor3 + Ranked[1][3] * factor4),
                                    +(Ranked[1][0] * factor1 - Ranked[1][1] * factor3 + Ranked[1][3] * factor5),
                                    -(Ranked[1][0] * factor2 - Ranked[1][1] * factor4 + Ranked[1][2] * factor5));

            return Ranked[0][0] * coef[0] + Ranked[0][1] * coef[1] + Ranked[0][2] * coef[2] + Ranked[0][3] * coef[3];
        }
        else
        {
            T determinant{static_cast<T>(0)};
            for (usize i = 0; i < N0; i += 2)
                determinant += Ranked[0][i] * SubTensor(0, i).Determinant();
            for (usize i = 1; i < N0; i += 2)
                determinant -= Ranked[0][i] * SubTensor(0, i).Determinant();
            return determinant;
        }
    }

    constexpr Tensor Inverse()
        requires(Rank == 2 && ((N0 == N) && ...))
    {
        if constexpr (N0 == 1)
            return 1.f / Flat[0];
        else if constexpr (N0 == 2)
        {
            const T idet = static_cast<T>(1) / Determinant();
            Tensor<T, 4, 4> inverse;
            inverse.Flat[0] = idet * Flat[3];
            inverse.Flat[1] = -idet * Flat[1];
            inverse.Flat[2] = -idet * Flat[2];
            inverse.Flat[3] = idet * Flat[0];
            return inverse;
        }
        else if constexpr (N0 == 3)
        {
            const T idet = static_cast<T>(1) / Determinant();
            Tensor<T, 4, 4> inverse;

            inverse.Ranked[0][0] = (Ranked[1][1] * Ranked[2][2] - Ranked[2][1] * Ranked[1][2]) * idet;
            inverse.Ranked[1][0] = -(Ranked[1][0] * Ranked[2][2] - Ranked[2][0] * Ranked[1][2]) * idet;
            inverse.Ranked[2][0] = (Ranked[1][0] * Ranked[2][1] - Ranked[2][0] * Ranked[1][1]) * idet;
            inverse.Ranked[0][1] = -(Ranked[0][1] * Ranked[2][2] - Ranked[2][1] * Ranked[0][2]) * idet;
            inverse.Ranked[1][1] = (Ranked[0][0] * Ranked[2][2] - Ranked[2][0] * Ranked[0][2]) * idet;
            inverse.Ranked[2][1] = -(Ranked[0][0] * Ranked[2][1] - Ranked[2][0] * Ranked[0][1]) * idet;
            inverse.Ranked[0][2] = (Ranked[0][1] * Ranked[1][2] - Ranked[1][1] * Ranked[0][2]) * idet;
            inverse.Ranked[1][2] = -(Ranked[0][0] * Ranked[1][2] - Ranked[1][0] * Ranked[0][2]) * idet;
            inverse.Ranked[2][2] = (Ranked[0][0] * Ranked[1][1] - Ranked[1][0] * Ranked[0][1]) * idet;
            return inverse;
        }
        else if constexpr (N0 == 4)
        {
            const T coef00 = Ranked[2][2] * Ranked[3][3] - Ranked[3][2] * Ranked[2][3];
            const T coef02 = Ranked[1][2] * Ranked[3][3] - Ranked[3][2] * Ranked[1][3];
            const T coef03 = Ranked[1][2] * Ranked[2][3] - Ranked[2][2] * Ranked[1][3];

            const T coef04 = Ranked[2][1] * Ranked[3][3] - Ranked[3][1] * Ranked[2][3];
            const T coef06 = Ranked[1][1] * Ranked[3][3] - Ranked[3][1] * Ranked[1][3];
            const T coef07 = Ranked[1][1] * Ranked[2][3] - Ranked[2][1] * Ranked[1][3];

            const T coef08 = Ranked[2][1] * Ranked[3][2] - Ranked[3][1] * Ranked[2][2];
            const T coef10 = Ranked[1][1] * Ranked[3][2] - Ranked[3][1] * Ranked[1][2];
            const T coef11 = Ranked[1][1] * Ranked[2][2] - Ranked[2][1] * Ranked[1][2];

            const T coef12 = Ranked[2][0] * Ranked[3][3] - Ranked[3][0] * Ranked[2][3];
            const T coef14 = Ranked[1][0] * Ranked[3][3] - Ranked[3][0] * Ranked[1][3];
            const T coef15 = Ranked[1][0] * Ranked[2][3] - Ranked[2][0] * Ranked[1][3];

            const T coef16 = Ranked[2][0] * Ranked[3][2] - Ranked[3][0] * Ranked[2][2];
            const T coef18 = Ranked[1][0] * Ranked[3][2] - Ranked[3][0] * Ranked[1][2];
            const T coef19 = Ranked[1][0] * Ranked[2][2] - Ranked[2][0] * Ranked[1][2];

            const T coef20 = Ranked[2][0] * Ranked[3][1] - Ranked[3][0] * Ranked[2][1];
            const T coef22 = Ranked[1][0] * Ranked[3][1] - Ranked[3][0] * Ranked[1][1];
            const T coef23 = Ranked[1][0] * Ranked[2][1] - Ranked[2][0] * Ranked[1][1];

            const Tensor<T, 4> fac0(coef00, coef00, coef02, coef03);
            const Tensor<T, 4> fac1(coef04, coef04, coef06, coef07);
            const Tensor<T, 4> fac2(coef08, coef08, coef10, coef11);
            const Tensor<T, 4> fac3(coef12, coef12, coef14, coef15);
            const Tensor<T, 4> fac4(coef16, coef16, coef18, coef19);
            const Tensor<T, 4> fac5(coef20, coef20, coef22, coef23);

            const Tensor<T, 4> vec0(Ranked[1][0], Ranked[0][0], Ranked[0][0], Ranked[0][0]);
            const Tensor<T, 4> vec1(Ranked[1][1], Ranked[0][1], Ranked[0][1], Ranked[0][1]);
            const Tensor<T, 4> vec2(Ranked[1][2], Ranked[0][2], Ranked[0][2], Ranked[0][2]);
            const Tensor<T, 4> vec3(Ranked[1][3], Ranked[0][3], Ranked[0][3], Ranked[0][3]);

            const Tensor<T, 4> inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
            const Tensor<T, 4> inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
            const Tensor<T, 4> inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
            const Tensor<T, 4> inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

            const Tensor<T, 4> SignA(+1, -1, +1, -1);
            const Tensor<T, 4> SignB(-1, +1, -1, +1);

            const Tensor<T, 4, 4> inverse{inv0 * SignA, inv1 * SignB, inv2 * SignA, inv3 * SignB};

            const Tensor<T, 4> row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);

            const Tensor<T, 4> dot0{Ranked[0] * row0};
            T Dot1 = (dot0.Flat[0] + dot0.Flat[1]) + (dot0.Flat[2] + dot0.Flat[3]);

            const T idet = static_cast<T>(1) / Dot1;

            return inverse * idet;
        }
        else
        {
            const T idet = static_cast<T>(1) / Determinant();
            return Cofactors().Transpose() * idet;
        }
    }

    constexpr Tensor<T, N..., N0> Transpose()
        requires(Rank == 2)
    {
        Tensor<T, N..., N0> transposed;
        for (usize i = 0; i < Tensor<T, N...>::Length; ++i)
            for (usize j = 0; j < N0; ++j)
                transposed[i][j] = Ranked[j][i];
        return transposed;
    }

    template <usize I0, usize... I> constexpr Permuted<I0, I...> Permute()
    {
        Permuted<I0, I...> permuted;
        constexpr Array<usize, Rank> dims = {N0, N...};
        constexpr Array<usize, Rank> pdims = {Axis<I0>, Axis<I>...};
        constexpr Array<usize, Rank> perm = {I0, I...};

        Array<usize, Rank> stride;
        Array<usize, Rank> pstride;
        stride[Rank - 1] = 1;
        pstride[Rank - 1] = 1;
        for (usize i = Rank - 1; i > 0; --i)
        {
            stride[i - 1] = stride[i] * dims[i];
            pstride[i - 1] = pstride[i] * pdims[i];
        }

        for (usize i = 0; i < Length; ++i)
        {
            Array<usize, Rank> pindices;
            for (usize j = 0; j < Rank; ++j)
                pindices[j] = (i / pstride[j]) % pdims[j];

            usize index = 0;
            for (usize j = 0; j < Rank; ++j)
                index += pindices[perm[j]] * stride[j];

            permuted.Flat[i] = Flat[index];
        }
        return permuted;
    }

    const T *GetData() const
    {
        return &Flat[0];
    }
    T *GetData()
    {
        return &Flat[0];
    }

    template <usize I = 0> constexpr const ChildType &At(const usize p_Index) const
    {
        static_assert(I < sizeof...(N) + 1, "[TOOLKIT][TENSOR] Axis index exceeds rank of tensor");
        if constexpr (I == 0)
        {
            TKIT_ASSERT(p_Index < N0, "[TOOLKIT][TENSOR] Index is out of bounds");
            return Ranked[p_Index];
        }
        else
            return At<I - 1>(Ranked[0], p_Index);
    }
    template <usize I = 0> constexpr ChildType &At(const usize p_Index)
    {
        static_assert(I < sizeof...(N) + 1, "[TOOLKIT][TENSOR] Axis index exceeds rank of tensor");
        if constexpr (I == 0)
        {
            TKIT_ASSERT(p_Index < N0, "[TOOLKIT][TENSOR] Index is out of bounds");
            return Ranked[p_Index];
        }
        else
            return At<I - 1>(Ranked[0], p_Index);
    }

    constexpr const ChildType &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr ChildType &operator[](const usize p_Index)
    {
        return At(p_Index);
    }

    friend constexpr Tensor operator*(const Tensor &p_Left, const Tensor &p_Right)
        requires(sizeof...(N) != 1)
    {
        Tensor tensor;
        for (usize i = 0; i < N0; ++i)
            tensor.Ranked[i] = p_Left.Ranked[i] * p_Right.Ranked[i];
    }

    friend constexpr Tensor operator-(const Tensor &p_Other)
    {
        Tensor tensor;
        for (usize i = 0; i < Length; ++i)
            tensor.Flat[i] = -p_Other.Flat[i];
        return tensor;
    }

#define CREATE_ARITHMETIC_OP(p_Op)                                                                                     \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const Tensor &p_Right)                                 \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Length; ++i)                                                                             \
            tensor.Flat[i] = p_Left.Flat[i] p_Op p_Right.Flat[i];                                                      \
        return tensor;                                                                                                 \
    }

    CREATE_ARITHMETIC_OP(+)
    CREATE_ARITHMETIC_OP(-)
    CREATE_ARITHMETIC_OP(/)
    CREATE_ARITHMETIC_OP(&)
    CREATE_ARITHMETIC_OP(|)

#define CREATE_SCALAR_OP(p_Op)                                                                                         \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T p_Right)                                       \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Length; ++i)                                                                             \
            tensor.Flat[i] = p_Left.Flat[i] p_Op p_Right;                                                              \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    friend constexpr Tensor operator p_Op(const T p_Left, const Tensor &p_Right)                                       \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Length; ++i)                                                                             \
            tensor.Flat[i] = p_Left p_Op p_Right.Flat[i];                                                              \
        return tensor;                                                                                                 \
    }

    CREATE_SCALAR_OP(+)
    CREATE_SCALAR_OP(-)
    CREATE_SCALAR_OP(*)
    CREATE_SCALAR_OP(/)
    CREATE_SCALAR_OP(&)
    CREATE_SCALAR_OP(|)

#define CREATE_BITSHIFT_OP(p_Op)                                                                                       \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T p_Shift)                                       \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Length; ++i)                                                                             \
            tensor.Flat[i] = p_Left.Flat[i] p_Op p_Shift;                                                              \
        return tensor;                                                                                                 \
    }

    CREATE_BITSHIFT_OP(>>)
    CREATE_BITSHIFT_OP(<<)

#define CREATE_SELF_OP(p_Op)                                                                                           \
    constexpr Tensor &operator p_Op##=(const Tensor & p_Other)                                                         \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_SELF_OP(+)
    CREATE_SELF_OP(-)
    CREATE_SELF_OP(*)
    CREATE_SELF_OP(/)
    CREATE_SELF_OP(&)
    CREATE_SELF_OP(|)
    CREATE_SELF_OP(>>)
    CREATE_SELF_OP(<<)

    union {
        ChildType Ranked[N0];
        T Flat[Length];
    };
};

#undef CREATE_ARITHMETIC_OP
#undef CREATE_SCALAR_OP
#undef CREATE_BITSHIFT_OP
#undef CREATE_SELF_OP

template <typename T, usize N, usize C2, usize R1>
constexpr Tensor<T, C2, R1> operator*(const Tensor<T, N, R1> &p_Left, const Tensor<T, C2, N> &p_Right)
{
    Tensor<T, C2, R1> tensor;
    for (usize i = 0; i < C2; ++i)
        for (usize j = 0; j < R1; ++j)
        {
            T sum{0};
            for (usize k = 0; k < N; ++k)
                sum += p_Left[k][j] * p_Right[i][k];
            tensor[i][j] = sum;
        }
    return tensor;
}

#undef CREATE_BINARY_OP
#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP

} // namespace Math
namespace Alias
{
// general
template <typename T, usize N0, usize... N> using ten = Math::Tensor<T, N0, N...>;
template <typename T, usize C, usize R> using mat = Math::Tensor<T, C, R>;
template <typename T, usize N> using vec = Math::Tensor<T, N>;

// tensor
template <usize N0, usize... N> using f32t = ten<f32, N0, N...>;
template <usize N0, usize... N> using f64t = ten<f64, N0, N...>;

template <usize N0, usize... N> using u8t = ten<u8, N0, N...>;
template <usize N0, usize... N> using u16t = ten<u16, N0, N...>;
template <usize N0, usize... N> using u32t = ten<u32, N0, N...>;
template <usize N0, usize... N> using u64t = ten<u64, N0, N...>;

template <usize N0, usize... N> using i8t = ten<i8, N0, N...>;
template <usize N0, usize... N> using i16t = ten<i16, N0, N...>;
template <usize N0, usize... N> using i32t = ten<i32, N0, N...>;
template <usize N0, usize... N> using i64t = ten<i64, N0, N...>;

// matrix
template <usize C, usize R> using f32m = f32t<C, R>;
template <usize C, usize R> using f64m = f64t<C, R>;

template <usize C, usize R> using u8m = u8t<C, R>;
template <usize C, usize R> using u16m = u16t<C, R>;
template <usize C, usize R> using u32m = u32t<C, R>;
template <usize C, usize R> using u64m = u64t<C, R>;

template <usize C, usize R> using i8m = i8t<C, R>;
template <usize C, usize R> using i16m = i16t<C, R>;
template <usize C, usize R> using i32m = i32t<C, R>;
template <usize C, usize R> using i64m = i64t<C, R>;

// 2x2
using f32m2 = f32t<2, 2>;
using f64m2 = f64t<2, 2>;

using u8m2 = u8t<2, 2>;
using u16m2 = u16t<2, 2>;
using u32m2 = u32t<2, 2>;
using u64m2 = u64t<2, 2>;

using i8m2 = i8t<2, 2>;
using i16m2 = i16t<2, 2>;
using i32m2 = i32t<2, 2>;
using i64m2 = i64t<2, 2>;

// 3x3
using f32m3 = f32t<3, 3>;
using f64m3 = f64t<3, 3>;

using u8m3 = u8t<3, 3>;
using u16m3 = u16t<3, 3>;
using u32m3 = u32t<3, 3>;
using u64m3 = u64t<3, 3>;

using i8m3 = i8t<3, 3>;
using i16m3 = i16t<3, 3>;
using i32m3 = i32t<3, 3>;
using i64m3 = i64t<3, 3>;

// 4x4
using f32m4 = f32t<4, 4>;
using f64m4 = f64t<4, 4>;

using u8m4 = u8t<4, 4>;
using u16m4 = u16t<4, 4>;
using u32m4 = u32t<4, 4>;
using u64m4 = u64t<4, 4>;

using i8m4 = i8t<4, 4>;
using i16m4 = i16t<4, 4>;
using i32m4 = i32t<4, 4>;
using i64m4 = i64t<4, 4>;

// 2×3 matrices (2 columns, 3 rows)
using f32m2x3 = f32t<2, 3>;
using f64m2x3 = f64t<2, 3>;

using u8m2x3 = u8t<2, 3>;
using u16m2x3 = u16t<2, 3>;
using u32m2x3 = u32t<2, 3>;
using u64m2x3 = u64t<2, 3>;

using i8m2x3 = i8t<2, 3>;
using i16m2x3 = i16t<2, 3>;
using i32m2x3 = i32t<2, 3>;
using i64m2x3 = i64t<2, 3>;

// 2×4
using f32m2x4 = f32t<2, 4>;
using f64m2x4 = f64t<2, 4>;

using u8m2x4 = u8t<2, 4>;
using u16m2x4 = u16t<2, 4>;
using u32m2x4 = u32t<2, 4>;
using u64m2x4 = u64t<2, 4>;

using i8m2x4 = i8t<2, 4>;
using i16m2x4 = i16t<2, 4>;
using i32m2x4 = i32t<2, 4>;
using i64m2x4 = i64t<2, 4>;

// 3×2
using f32m3x2 = f32t<3, 2>;
using f64m3x2 = f64t<3, 2>;

using u8m3x2 = u8t<3, 2>;
using u16m3x2 = u16t<3, 2>;
using u32m3x2 = u32t<3, 2>;
using u64m3x2 = u64t<3, 2>;

using i8m3x2 = i8t<3, 2>;
using i16m3x2 = i16t<3, 2>;
using i32m3x2 = i32t<3, 2>;
using i64m3x2 = i64t<3, 2>;

// 3×4
using f32m3x4 = f32t<3, 4>;
using f64m3x4 = f64t<3, 4>;

using u8m3x4 = u8t<3, 4>;
using u16m3x4 = u16t<3, 4>;
using u32m3x4 = u32t<3, 4>;
using u64m3x4 = u64t<3, 4>;

using i8m3x4 = i8t<3, 4>;
using i16m3x4 = i16t<3, 4>;
using i32m3x4 = i32t<3, 4>;
using i64m3x4 = i64t<3, 4>;

// 4×2
using f32m4x2 = f32t<4, 2>;
using f64m4x2 = f64t<4, 2>;

using u8m4x2 = u8t<4, 2>;
using u16m4x2 = u16t<4, 2>;
using u32m4x2 = u32t<4, 2>;
using u64m4x2 = u64t<4, 2>;

using i8m4x2 = i8t<4, 2>;
using i16m4x2 = i16t<4, 2>;
using i32m4x2 = i32t<4, 2>;
using i64m4x2 = i64t<4, 2>;

// 4×3
using f32m4x3 = f32t<4, 3>;
using f64m4x3 = f64t<4, 3>;

using u8m4x3 = u8t<4, 3>;
using u16m4x3 = u16t<4, 3>;
using u32m4x3 = u32t<4, 3>;
using u64m4x3 = u64t<4, 3>;

using i8m4x3 = i8t<4, 3>;
using i16m4x3 = i16t<4, 3>;
using i32m4x3 = i32t<4, 3>;
using i64m4x3 = i64t<4, 3>;

// vector
template <usize N> using f32v = f32t<N>;
template <usize N> using f64v = f64t<N>;

template <usize N> using u8v = u8t<N>;
template <usize N> using u16v = u16t<N>;
template <usize N> using u32v = u32t<N>;
template <usize N> using u64v = u64t<N>;

template <usize N> using i8v = i8t<N>;
template <usize N> using i16v = i16t<N>;
template <usize N> using i32v = i32t<N>;
template <usize N> using i64v = i64t<N>;

using f32v2 = f32t<2>;
using f64v2 = f64t<2>;

using u8v2 = u8t<2>;
using u16v2 = u16t<2>;
using u32v2 = u32t<2>;
using u64v2 = u64t<2>;

using i8v2 = i8t<2>;
using i16v2 = i16t<2>;
using i32v2 = i32t<2>;
using i64v2 = i64t<2>;

using f32v3 = f32t<3>;
using f64v3 = f64t<3>;

using u8v3 = u8t<3>;
using u16v3 = u16t<3>;
using u32v3 = u32t<3>;
using u64v3 = u64t<3>;

using i8v3 = i8t<3>;
using i16v3 = i16t<3>;
using i32v3 = i32t<3>;
using i64v3 = i64t<3>;

using f32v4 = f32t<4>;
using f64v4 = f64t<4>;

using u8v4 = u8t<4>;
using u16v4 = u16t<4>;
using u32v4 = u32t<4>;
using u64v4 = u64t<4>;

using i8v4 = i8t<4>;
using i16v4 = i16t<4>;
using i32v4 = i32t<4>;
using i64v4 = i64t<4>;

} // namespace Alias
} // namespace TKit
