#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/utils/logging.hpp"

namespace TKit
{
namespace Math
{
template <typename T, usize N0, usize... N>
    requires((N0 > 0) && ((N > 0) && ...))
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

template <usize I, typename T, usize N0, usize... N>
constexpr const auto &At(const Tensor<T, N0, N...> &p_Tensor, const usize p_Index)
{
    static_assert(I < sizeof...(N) + 1, "[TOOLKIT][TENSOR] Axis index exceeds rank of tensor");
    if constexpr (I == 0)
    {
        TKIT_ASSERT(p_Index < N0, "[TOOLKIT][TENSOR] Index is out of bounds")
        return p_Tensor.Ranked[p_Index];
    }
    else
        return At<I - 1>(p_Tensor.Ranked[0], p_Index);
}
template <usize I, typename T, usize N0, usize... N>
constexpr auto &At(Tensor<T, N0, N...> &p_Tensor, const usize p_Index)
{
    static_assert(I < sizeof...(N) + 1, "[TOOLKIT][TENSOR] Axis index exceeds rank of tensor");
    if constexpr (I == 0)
    {
        TKIT_ASSERT(p_Index < N0, "[TOOLKIT][TENSOR] Index is out of bounds")
        return p_Tensor.Ranked[p_Index];
    }
    else
        return At<I - 1>(p_Tensor.Ranked[0], p_Index);
}

template <typename T, usize N0, usize N1, usize... N> struct Permutations
{
    template <usize I0, usize I1, usize... I>
        requires(sizeof...(I) == sizeof...(N))
    using Permute = Tensor<T, GetAxis<I0, N0, N1, N...>, GetAxis<I1, N0, N1, N...>, GetAxis<I, N0, N1, N...>...>;
};
} // namespace Detail

template <typename T, usize N> struct Tensor<T, N>
{
    using ValueType = T;
    using ChildType = T;
    static constexpr usize Length = N;
    static constexpr usize Rank = 1;

    constexpr Tensor() = default;
    template <std::convertible_to<T> U> Tensor(const Tensor<T, N - 1> &p_Small, U &&p_Value)
    {
    }

    template <std::convertible_to<T> U> constexpr Tensor(U &&p_Value)
    {
        for (usize i = 0; i < N; ++i)
            Ranked[i] = p_Value;
    }

    template <typename... Args>
    constexpr Tensor(Args &&...p_Args)
        requires(sizeof...(Args) == N && (std::convertible_to<Args, T> && ...))
        : Ranked{static_cast<T>(std::forward<Args>(p_Args))...}
    {
    }

    template <usize I> static constexpr usize GetAxis()
    {
        return Detail::GetAxis<I, N>();
    }

    template <usize I = 0> constexpr const ChildType &At(const usize p_Index) const
    {
        return Detail::At<I>(*this, p_Index);
    }
    template <usize I = 0> constexpr ChildType &At(const usize p_Index)
    {
        return Detail::At<I>(*this, p_Index);
    }

    constexpr const ChildType &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr ChildType &operator[](const usize p_Index)
    {
        return At(p_Index);
    }

#define CREATE_ARITHMETIC_OP(p_Op)                                                                                     \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const Tensor &p_Right)                                 \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Right.Ranked[i];                                                \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T &p_Right)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Right;                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    friend constexpr Tensor operator p_Op(const T &p_Left, const Tensor &p_Right)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left p_Op p_Right.Ranked[i];                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    constexpr Tensor &operator p_Op##=(const Tensor & p_Other)                                                         \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

#define CREATE_BITSHIFT_OP(p_Op)                                                                                       \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T &p_Shift)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Shift;                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    constexpr Tensor &operator p_Op##=(const Tensor & p_Other)                                                         \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_ARITHMETIC_OP(+)
    CREATE_ARITHMETIC_OP(-)
    CREATE_ARITHMETIC_OP(*)
    CREATE_ARITHMETIC_OP(/)
    CREATE_ARITHMETIC_OP(&)
    CREATE_ARITHMETIC_OP(|)

    CREATE_BITSHIFT_OP(<<)
    CREATE_BITSHIFT_OP(>>)

    friend constexpr Tensor operator-(const Tensor &p_Other)
    {
        Tensor tensor;
        for (usize i = 0; i < N; ++i)
            tensor.Ranked[i] = -p_Other.Ranked[i];
        return tensor;
    }

    T Ranked[N];
};
#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP

template <typename T, usize C, usize R> struct Tensor<T, C, R>
{
    using ValueType = T;
    using ChildType = Tensor<T, R>;

    template <usize I0, usize I1> using Permute = typename Detail::Permutations<T, C, R>::template Permute<I0, I1>;

    static constexpr usize Columns = C;
    static constexpr usize Rows = R;
    static constexpr usize Length = C * R;
    static constexpr usize Rank = 2;

    constexpr Tensor() = default;

    template <std::convertible_to<T> U> constexpr Tensor(U &&p_Value)
    {
        for (usize i = 0; i < C; ++i)
            Ranked[i] = ChildType{p_Value};
    }

    template <typename... Args>
    constexpr Tensor(Args &&...p_Args)
        requires(sizeof...(Args) == C && (std::convertible_to<Args, ChildType> && ...))
        : Ranked{static_cast<ChildType>(std::forward<Args>(p_Args))...}
    {
    }

    template <usize I> static constexpr usize GetAxis()
    {
        return Detail::GetAxis<I, C, R>();
    }

    template <usize I = 0> constexpr const ChildType &At(const usize p_Index) const
    {
        return Detail::At<I>(*this, p_Index);
    }
    template <usize I = 0> constexpr ChildType &At(const usize p_Index)
    {
        return Detail::At<I>(*this, p_Index);
    }

    constexpr const ChildType &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr ChildType &operator[](const usize p_Index)
    {
        return At(p_Index);
    }

#define CREATE_BINARY_OP(p_Op)                                                                                         \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const Tensor &p_Right)                                 \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < C; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Right.Ranked[i];                                                \
        return tensor;                                                                                                 \
    }

#define CREATE_ARITHMETIC_OP(p_Op)                                                                                     \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T &p_Right)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < C; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Right;                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    friend constexpr Tensor operator p_Op(const T &p_Left, const Tensor &p_Right)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < C; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left p_Op p_Right.Ranked[i];                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    constexpr Tensor &operator p_Op##=(const Tensor & p_Other)                                                         \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

#define CREATE_BITSHIFT_OP(p_Op)                                                                                       \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T &p_Shift)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < C; ++i)                                                                                  \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Shift;                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    constexpr Tensor &operator p_Op##=(const Tensor & p_Other)                                                         \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }
    CREATE_BINARY_OP(+)
    CREATE_BINARY_OP(-)
    CREATE_BINARY_OP(/)
    CREATE_BINARY_OP(&)
    CREATE_BINARY_OP(|)

    CREATE_ARITHMETIC_OP(+)
    CREATE_ARITHMETIC_OP(-)
    CREATE_ARITHMETIC_OP(*)
    CREATE_ARITHMETIC_OP(/)
    CREATE_ARITHMETIC_OP(&)
    CREATE_ARITHMETIC_OP(|)

    CREATE_BITSHIFT_OP(<<)
    CREATE_BITSHIFT_OP(>>)

    template <usize N, usize C2, usize R1>
    friend constexpr Tensor<T, C2, R1> operator*(const Tensor<T, N, R1> &p_Left, const Tensor<T, C2, N> &p_Right)
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
    union {
        ChildType Ranked[C];
        T Flat[Length];
    };
};
#undef CREATE_BINARY_OP
#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP

template <typename T, usize N0, usize N1, usize N2, usize... N> struct Tensor<T, N0, N1, N2, N...>
{
    using ValueType = T;
    using ChildType = Tensor<T, N1, N2, N...>;

    template <usize I0, usize I1, usize I2, usize... I>
    using Permute = typename Detail::Permutations<T, N0, N1, N2, N...>::template Permute<I0, I1, I2, I...>;

    static constexpr usize Length = N0 * N1 * N2 * (N * ...);
    static constexpr usize Rank = sizeof...(N) + 3;

    constexpr Tensor() = default;

    template <std::convertible_to<T> U> constexpr Tensor(U &&p_Value)
    {
        for (usize i = 0; i < N0; ++i)
            Ranked[i] = ChildType{p_Value};
    }

    template <typename... Args>
    constexpr Tensor(Args &&...p_Args)
        requires(sizeof...(Args) == N0 && (std::convertible_to<Args, ChildType> && ...))
        : Ranked{static_cast<ChildType>(std::forward<Args>(p_Args))...}
    {
    }

    template <usize I> static constexpr usize GetAxis()
    {
        return Detail::GetAxis<I, N0, N1, N2, N...>();
    }

    template <usize I = 0> constexpr const ChildType &At(const usize p_Index) const
    {
        return Detail::At<I>(*this, p_Index);
    }
    template <usize I = 0> constexpr ChildType &At(const usize p_Index)
    {
        return Detail::At<I>(*this, p_Index);
    }

    constexpr const ChildType &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr ChildType &operator[](const usize p_Index)
    {
        return At(p_Index);
    }

#define CREATE_ARITHMETIC_OP(p_Op)                                                                                     \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const Tensor &p_Right)                                 \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N0; ++i)                                                                                 \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Right.Ranked[i];                                                \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T &p_Right)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N0; ++i)                                                                                 \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Right;                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    friend constexpr Tensor operator p_Op(const T &p_Left, const Tensor &p_Right)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N0; ++i)                                                                                 \
            tensor.Ranked[i] = p_Left p_Op p_Right.Ranked[i];                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    constexpr Tensor &operator p_Op##=(const Tensor & p_Other)                                                         \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

#define CREATE_BITSHIFT_OP(p_Op)                                                                                       \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const T &p_Shift)                                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < N0; ++i)                                                                                 \
            tensor.Ranked[i] = p_Left.Ranked[i] p_Op p_Shift;                                                          \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    constexpr Tensor &operator p_Op##=(const Tensor & p_Other)                                                         \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_ARITHMETIC_OP(+)
    CREATE_ARITHMETIC_OP(-)
    CREATE_ARITHMETIC_OP(*)
    CREATE_ARITHMETIC_OP(/)
    CREATE_ARITHMETIC_OP(&)
    CREATE_ARITHMETIC_OP(|)

    CREATE_BITSHIFT_OP(<<)
    CREATE_BITSHIFT_OP(>>)

    friend constexpr Tensor operator-(const Tensor &p_Other)
    {
        Tensor tensor;
        for (usize i = 0; i < N0; ++i)
            tensor.Ranked[i] = -p_Other.Ranked[i];
        return tensor;
    }

    union {
        ChildType Ranked[N0];
        T Flat[Length];
    };
};
#undef CREATE_ARITHMETIC_OP
#undef CREATE_BITSHIFT_OP

// TODO: Add concept here as well
template <typename T, usize N0, usize... N> T *GetData(const Tensor<T, N0, N...> &p_Tensor)
{
    if constexpr (sizeof...(N) == 0)
        return &p_Tensor.Ranked[0];
    else
        return GetData(p_Tensor.Ranked[0]);
}

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
