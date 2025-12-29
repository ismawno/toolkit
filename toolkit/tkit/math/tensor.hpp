#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/utils/debug.hpp"
#include "tkit/container/fixed_array.hpp"
#include <math.h>

TKIT_COMPILER_WARNING_IGNORE_PUSH()
TKIT_MSVC_WARNING_IGNORE(4146)

namespace TKit::Math
{
template <typename T, usize N0, usize... N>
// requires((N0 > 0) && ... && (N > 0))
struct Tensor;
}

namespace TKit::Detail
{
template <usize I, usize N0, usize... N> constexpr usize GetAxisSize()
{
    static_assert(I <= sizeof...(N), "[TOOLKIT][TENSOR] Axis index exceeds rank of tensor");
    if constexpr (I == 0)
        return N0;
    else
        return GetAxisSize<I - 1, N...>();
}
template <usize N0, usize... N> constexpr usize GetDiagonalStride()
{
    if constexpr (sizeof...(N) == 0)
        return 1;
    else
        return (N * ... * 1) + GetDiagonalStride<N...>();
}
template <typename T, usize... N> struct Child
{
    using Type = Math::Tensor<T, N...>;
};
template <typename T> struct Child<T>
{
    using Type = T;
};
template <typename T, usize N0, usize... N> struct Parent
{
    using Type = Math::Tensor<T, N0, N...>;
};

template <typename T> constexpr T SquareRoot(const T p_Value)
{
#ifdef TKIT_COMPILER_MSVC
    if constexpr (std::same_as<T, f32>)
        return std::sqrtf(p_Value);
    else if constexpr (std::same_as<T, f64>)
        return std::sqrt(p_Value);
    else
        return std::sqrtf(static_cast<f32>(p_Value));
#else
    return std::sqrt(p_Value);
#endif
}

} // namespace TKit::Detail

namespace TKit::Math
{
template <typename T, usize N0, usize... N>
// requires((N0 > 0) && ... && (N > 0))
struct Tensor
{
    using ValueType = T;
    using ChildType = typename Detail::Child<T, N...>::Type;

    template <usize NP> using ParentType = typename Detail::Parent<T, NP, N0, N...>::Type;

    static constexpr usize ChildSize = N0;
    static constexpr usize Size = (N0 * ... * N);
    static constexpr usize Rank = sizeof...(N) + 1;

    template <usize I>
        requires(I < Rank)
    static constexpr usize AxisSize = Detail::GetAxisSize<I, N0, N...>();

    template <usize I0, usize... I>
        requires(sizeof...(I) == sizeof...(N) && ((I0 < Rank) && ... && (I < Rank)))
    using Permuted = Tensor<T, AxisSize<I0>, AxisSize<I>...>;

    constexpr Tensor() = default;
    constexpr Tensor(const Tensor &) = default;

    template <std::convertible_to<T> U, usize M0, usize... M>
        requires(sizeof...(M) == sizeof...(N) && ((M0 >= N0) && ... && (M >= N)))
    constexpr Tensor(const Tensor<U, M0, M...> &p_Tensor)
    {
        *this = Slice<T, N0, N...>(p_Tensor);
    }

    template <std::convertible_to<T> U> explicit constexpr Tensor(const U p_Value)
    {
        for (usize i = 0; i < Size; ++i)
            Flat[i] = static_cast<T>(p_Value);
    }

    template <typename... Args>
        requires((sizeof...(Args) == Size) && ... && std::convertible_to<Args, T>)
    constexpr explicit Tensor(const Args... p_Args) : Flat{static_cast<T>(p_Args)...}
    {
    }
    template <typename... Args>
    constexpr explicit Tensor(const Args &...p_Args)
        requires(!std::same_as<ChildType, T> && sizeof...(Args) == N0 && (std::convertible_to<Args, ChildType> && ...))
        : Ranked{static_cast<ChildType>(p_Args)...}
    {
    }

#ifndef TKIT_COMPILER_MSVC
    template <std::convertible_to<T> U, typename... Args>
        requires((sizeof...(Args) > 0 && sizeof...(Args) < N0) && ... && std::convertible_to<Args, ChildType>)
    constexpr Tensor(const Tensor<U, N0 - sizeof...(Args), N...> &p_Tensor, const Args &...p_Args)
        requires(N0 > 1)
    {
        usize i = 0;
        for (; i < N0 - sizeof...(Args); ++i)
            Ranked[i] = static_cast<ChildType>(p_Tensor[i]);
        ((Ranked[i++] = static_cast<ChildType>(p_Args)), ...);
    }
#else
    template <typename Ten, typename... Args>
        requires((sizeof...(Args) > 0 && sizeof...(Args) < N0 && Ten::ChildSize == N0 - sizeof...(Args)) && ... &&
                 std::convertible_to<Args, ChildType>)
    constexpr Tensor(const Ten &p_Tensor, const Args &...p_Args)
        requires(N0 > 1)
    {
        usize i = 0;
        for (; i < N0 - sizeof...(Args); ++i)
            Ranked[i] = static_cast<ChildType>(p_Tensor[i]);
        ((Ranked[i++] = static_cast<ChildType>(p_Args)), ...);
    }
#endif

    template <std::convertible_to<T> U, std::convertible_to<ChildType> C>
    constexpr Tensor(const C &p_Value, const Tensor<U, N0 - 1, N...> &p_Tensor)
        requires(N0 > 1)
    {
        Ranked[0] = static_cast<ChildType>(p_Value);
        for (usize i = 0; i < N0 - 1; ++i)
            Ranked[i + 1] = static_cast<ChildType>(p_Tensor[i]);
    }

    constexpr Tensor &operator=(const Tensor &) = default;
    template <std::convertible_to<T> U, usize M0, usize... M>
        requires(sizeof...(M) == sizeof...(N) && ((M0 >= N0) && ... && (M >= N)))
    constexpr Tensor &operator=(const Tensor<U, M0, M...> &p_Tensor)
    {
        *this = Slice<T, N0, N...>(p_Tensor);
        return *this;
    }

    template <std::convertible_to<T> U, usize M0, usize... M>
        requires(sizeof...(M) == sizeof...(N) && ((N0 >= M0) && ... && (N >= M)))
    constexpr operator Tensor<U, M0, M...>()
    {
        return Slice<U, M0, M...>(*this);
    }

    template <std::convertible_to<T> U> static constexpr Tensor Identity(const U p_Value)
    {
        Tensor tensor{static_cast<T>(0)};
        constexpr usize stride = Detail::GetDiagonalStride<N0, N...>();
        for (usize i = 0; i < Size; i += stride)
            tensor.Flat[i] = static_cast<T>(p_Value);
        return tensor;
    }

    static constexpr Tensor Identity()
    {
        return Identity(1);
    }

    constexpr const T *GetData() const
    {
        return &Flat[0];
    }
    constexpr T *GetData()
    {
        return &Flat[0];
    }

    constexpr const ChildType &At(const usize p_Index) const
    {
        TKIT_ASSERT(p_Index < N0, "[TOOLKIT][TENSOR] Index is out of bounds: {} >= {}", p_Index, N0);
        return Ranked[p_Index];
    }
    constexpr ChildType &At(const usize p_Index)
    {
        TKIT_ASSERT(p_Index < N0, "[TOOLKIT][TENSOR] Index is out of bounds: {} >= {}", p_Index, N0);
        return Ranked[p_Index];
    }

    constexpr const ChildType &operator[](const usize p_Index) const
    {
        return At(p_Index);
    }
    constexpr ChildType &operator[](const usize p_Index)
    {
        return At(p_Index);
    }

    friend constexpr Tensor operator-(const Tensor &p_Tensor)
    {
        Tensor tensor;
        for (usize i = 0; i < Size; ++i)
            tensor.Flat[i] = -p_Tensor.Flat[i];
        return tensor;
    }

#define CREATE_ARITHMETIC_OP(p_Op, p_Requires)                                                                         \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const Tensor &p_Right) p_Requires                      \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Size; ++i)                                                                               \
            tensor.Flat[i] = p_Left.Flat[i] p_Op p_Right.Flat[i];                                                      \
        return tensor;                                                                                                 \
    }

    CREATE_ARITHMETIC_OP(+, )
    CREATE_ARITHMETIC_OP(-, )
    CREATE_ARITHMETIC_OP(/, )
    CREATE_ARITHMETIC_OP(&, requires(Integer<T>))
    CREATE_ARITHMETIC_OP(|, requires(Integer<T>))

#define CREATE_SCALAR_OP(p_Op, p_Requires)                                                                             \
    template <std::convertible_to<T> U>                                                                                \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const U p_Right) p_Requires                            \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Size; ++i)                                                                               \
            tensor.Flat[i] = p_Left.Flat[i] p_Op static_cast<T>(p_Right);                                              \
        return tensor;                                                                                                 \
    }                                                                                                                  \
    template <std::convertible_to<T> U>                                                                                \
    friend constexpr Tensor operator p_Op(const U p_Left, const Tensor &p_Right) p_Requires                            \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Size; ++i)                                                                               \
            tensor.Flat[i] = static_cast<T>(p_Left) p_Op p_Right.Flat[i];                                              \
        return tensor;                                                                                                 \
    }

    CREATE_SCALAR_OP(+, )
    CREATE_SCALAR_OP(-, )
    CREATE_SCALAR_OP(*, )
    CREATE_SCALAR_OP(/, )
    CREATE_SCALAR_OP(&, requires(Integer<T>))
    CREATE_SCALAR_OP(|, requires(Integer<T>))

#define CREATE_BITSHIFT_OP(p_Op)                                                                                       \
    template <std::convertible_to<T> U>                                                                                \
    friend constexpr Tensor operator p_Op(const Tensor &p_Left, const U p_Shift)                                       \
        requires(Integer<T>)                                                                                           \
    {                                                                                                                  \
        Tensor tensor;                                                                                                 \
        for (usize i = 0; i < Size; ++i)                                                                               \
            tensor.Flat[i] = p_Left.Flat[i] p_Op static_cast<T>(p_Shift);                                              \
        return tensor;                                                                                                 \
    }

    CREATE_BITSHIFT_OP(>>)
    CREATE_BITSHIFT_OP(<<)

#define CREATE_CMP_OP(p_Op, p_Cmp, p_Start)                                                                            \
    friend constexpr bool operator p_Op(const Tensor &p_Left, const Tensor &p_Right)                                   \
    {                                                                                                                  \
        bool result = p_Start;                                                                                         \
        for (usize i = 0; i < Size; ++i)                                                                               \
            result p_Cmp p_Left.Flat[i] p_Op p_Right.Flat[i];                                                          \
        return result;                                                                                                 \
    }

    CREATE_CMP_OP(==, &=, true)
    CREATE_CMP_OP(!=, |=, false)

#define CREATE_SELF_OP(p_Op, p_Requires)                                                                               \
    constexpr Tensor &operator p_Op## = (const Tensor &p_Other)p_Requires                                              \
    {                                                                                                                  \
        *this = *this p_Op p_Other;                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
    template <std::convertible_to<T> U> constexpr Tensor &operator p_Op## = (const U p_Value)p_Requires                \
    {                                                                                                                  \
        *this = *this p_Op p_Value;                                                                                    \
        return *this;                                                                                                  \
    }

    CREATE_SELF_OP(+, )
    CREATE_SELF_OP(-, )
    CREATE_SELF_OP(*, )
    CREATE_SELF_OP(/, )
    CREATE_SELF_OP(&, requires(Integer<T>))
    CREATE_SELF_OP(|, requires(Integer<T>))
    CREATE_SELF_OP(>>, requires(Integer<T>))
    CREATE_SELF_OP(<<, requires(Integer<T>))

    union {
        ChildType Ranked[N0];
        T Flat[Size];
    };
};

#undef CREATE_ARITHMETIC_OP
#undef CREATE_SCALAR_OP
#undef CREATE_BITSHIFT_OP
#undef CREATE_CMP_OP
#undef CREATE_SELF_OP

#define SIZE Tensor<T, N0, N...>::Size
#define RANK Tensor<T, N0, N...>::Rank

template <typename T, usize N0, usize... N>
constexpr T Dot(const Tensor<T, N0, N...> &p_Left, const Tensor<T, N0, N...> &p_Right)
{
    T result{static_cast<T>(0)};

    for (usize i = 0; i < SIZE; ++i)
        result += p_Left.Flat[i] * p_Right.Flat[i];
    return result;
}

template <typename T, usize N0, usize... N> constexpr T NormSquared(const Tensor<T, N0, N...> &p_Tensor)
{
    return Dot(p_Tensor, p_Tensor);
}

template <typename T, usize N0, usize... N> constexpr T Norm(const Tensor<T, N0, N...> &p_Tensor)
{
    return Detail::SquareRoot(Dot(p_Tensor, p_Tensor));
}
template <typename T, usize N0, usize... N>
constexpr T DistanceSquared(const Tensor<T, N0, N...> &p_Left, const Tensor<T, N0, N...> &p_Right)
{
    const Tensor<T, N0, N...> diff = p_Right - p_Left;
    return Dot(diff, diff);
}
template <typename T, usize N0, usize... N>
constexpr T Distance(const Tensor<T, N0, N...> &p_Left, const Tensor<T, N0, N...> &p_Right)
{
    return std::sqrt(DistanceSquared(p_Left, p_Right));
}
template <typename T, usize N0, usize... N> constexpr Tensor<T, N0, N...> Normalize(const Tensor<T, N0, N...> &p_Tensor)
{
    return p_Tensor / Norm(p_Tensor);
}

template <usize R0, usize... R, typename T, usize N0, usize... N>
    requires((R0 * ... * R) == (N0 * ... * N))
constexpr Tensor<T, R0, R...> Reshape(const Tensor<T, N0, N...> &p_Tensor)
{
    Tensor<T, R0, R...> reshaped;
    for (usize i = 0; i < SIZE; ++i)
        reshaped.Flat[i] = p_Tensor.Flat[i];
    return reshaped;
}

template <typename T, usize N0, usize... N, typename I0, typename... I>
    requires(sizeof...(I) == sizeof...(N) && (std::convertible_to<I0, usize> && ... && std::convertible_to<I, usize>))
constexpr void SubTensorImpl(const Tensor<T, N0, N...> &p_Tensor, Tensor<T, N0 - 1, (N - 1)...> &p_Minor,
                             const I0 p_First, const I... p_Rest)
    requires((N0 > 1) && ... && (N0 == N))
{
    const usize first = static_cast<usize>(p_First);
    TKIT_ASSERT(first < N0, "[TOOLKIT][TENSOR] Index is out of bounds: {} >= {}", first, N0);
    for (usize i = 0, j = 0; i < N0; ++i)
        if (i != first)
        {
            if constexpr (sizeof...(I) == 0)
                p_Minor.Flat[j] = p_Tensor.Flat[i];
            else
                SubTensorImpl(p_Tensor[i], p_Minor[j], p_Rest...);
            ++j;
        }
}

template <typename T, usize N0, usize... N, typename I0, typename... I>
    requires(sizeof...(I) == sizeof...(N) && (std::convertible_to<I0, usize> && ... && std::convertible_to<I, usize>))
constexpr Tensor<T, N0 - 1, (N - 1)...> SubTensor(const Tensor<T, N0, N...> &p_Tensor, const I0 p_First,
                                                  const I... p_Rest)
    requires((N0 > 1) && ... && (N0 == N))
{
    Tensor<T, N0 - 1, (N - 1)...> minor;
    SubTensorImpl(p_Tensor, minor, p_First, p_Rest...);
    return minor;
}

template <usize I0, usize... I, typename T, usize N0, usize... N>
constexpr typename Tensor<T, N0, N...>::template Permuted<I0, I...> Permute(const Tensor<T, N0, N...> &p_Tensor)
{
    typename Tensor<T, N0, N...>::template Permuted<I0, I...> permuted;
    constexpr FixedArray<usize, RANK> dims = {N0, N...};
    constexpr FixedArray<usize, RANK> pdims = {Detail::GetAxisSize<I0, N0, N...>(),
                                               Detail::GetAxisSize<I, N0, N...>()...};
    constexpr FixedArray<usize, RANK> perm = {I0, I...};

    FixedArray<usize, RANK> stride;
    FixedArray<usize, RANK> pstride;
    stride[RANK - 1] = 1;
    pstride[RANK - 1] = 1;
    for (usize i = RANK - 1; i > 0; --i)
    {
        stride[i - 1] = stride[i] * dims[i];
        pstride[i - 1] = pstride[i] * pdims[i];
    }

    for (usize i = 0; i < SIZE; ++i)
    {
        FixedArray<usize, RANK> indices;
        for (usize j = 0; j < RANK; ++j)
            indices[j] = (i / stride[j]) % dims[j];

        usize pindex = 0;
        for (usize j = 0; j < RANK; ++j)
            pindex += indices[perm[j]] * pstride[j];

        permuted.Flat[pindex] = p_Tensor.Flat[i];
    }
    return permuted;
}

template <typename U, usize M0, usize... M, typename T, usize N0, usize... N>
    requires(std::convertible_to<T, U> && sizeof...(M) == sizeof...(N) && ((N0 >= M0) && ... && (N >= M)))
constexpr void SliceImpl(const Tensor<T, N0, N...> &p_Tensor, Tensor<U, M0, M...> &p_Sliced)
{
    for (usize i = 0; i < M0; ++i)
        if constexpr (sizeof...(M) == 0)
            p_Sliced.Flat[i] = static_cast<U>(p_Tensor.Flat[i]);
        else
            SliceImpl(p_Tensor[i], p_Sliced[i]);
}

template <typename U, usize M0, usize... M, typename T, usize N0, usize... N>
    requires(std::convertible_to<T, U> && sizeof...(M) == sizeof...(N) && ((N0 >= M0) && ... && (N >= M)))
constexpr Tensor<U, M0, M...> Slice(const Tensor<T, N0, N...> &p_Tensor)
{
    Tensor<U, M0, M...> sliced;
    SliceImpl(p_Tensor, sliced);
    return sliced;
}
template <typename T, usize N0, usize... N> constexpr const T *AsPointer(const Tensor<T, N0, N...> &p_Tensor)
{
    return &p_Tensor.Flat[0];
}
template <typename T, usize N0, usize... N> constexpr T *AsPointer(Tensor<T, N0, N...> &p_Tensor)
{
    return &p_Tensor.Flat[0];
}

} // namespace TKit::Math
namespace TKit
{

#undef SIZE
#undef RANK

namespace Alias
{
// general
template <typename T, usize N0, usize... N> using ten = Math::Tensor<T, N0, N...>;
template <typename T, usize C, usize R = C> using mat = Math::Tensor<T, C, R>;
template <typename T, usize N> using vec = Math::Tensor<T, N>;

template <typename T> using vec2 = ten<T, 2>;
template <typename T> using vec3 = ten<T, 3>;
template <typename T> using vec4 = ten<T, 4>;

template <typename T> using mat2 = ten<T, 2, 2>;
template <typename T> using mat3 = ten<T, 3, 3>;
template <typename T> using mat4 = ten<T, 4, 4>;

template <typename T> using mat2x3 = ten<T, 2, 3>;
template <typename T> using mat2x4 = ten<T, 2, 4>;

template <typename T> using mat3x2 = ten<T, 3, 2>;
template <typename T> using mat3x4 = ten<T, 3, 4>;

template <typename T> using mat4x2 = ten<T, 4, 2>;
template <typename T> using mat4x3 = ten<T, 4, 3>;

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
template <usize C, usize R = C> using f32m = f32t<C, R>;
template <usize C, usize R = C> using f64m = f64t<C, R>;

template <usize C, usize R = C> using u8m = u8t<C, R>;
template <usize C, usize R = C> using u16m = u16t<C, R>;
template <usize C, usize R = C> using u32m = u32t<C, R>;
template <usize C, usize R = C> using u64m = u64t<C, R>;

template <usize C, usize R = C> using i8m = i8t<C, R>;
template <usize C, usize R = C> using i16m = i16t<C, R>;
template <usize C, usize R = C> using i32m = i32t<C, R>;
template <usize C, usize R = C> using i64m = i64t<C, R>;

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
using namespace Alias;

namespace Math
{
template <typename T, usize N, usize C2, usize R1>
constexpr mat<T, C2, R1> operator*(const mat<T, N, R1> &p_Left, const mat<T, C2, N> &p_Right)
{
    mat<T, C2, R1> result;
    for (usize i = 0; i < C2; ++i)
        for (usize j = 0; j < R1; ++j)
        {
            T sum{static_cast<T>(0)};
            for (usize k = 0; k < N; ++k)
                sum += p_Left[k][j] * p_Right[i][k];
            result[i][j] = sum;
        }
    return result;
}

template <typename T, usize N, usize R>
constexpr vec<T, R> operator*(const mat<T, N, R> &p_Left, const vec<T, N> &p_Right)
{
    vec<T, R> result;
    for (usize i = 0; i < R; ++i)
    {
        T sum{static_cast<T>(0)};
        for (usize j = 0; j < N; ++j)
            sum += p_Left[j][i] * p_Right.Flat[j];
        result.Flat[i] = sum;
    }
    return result;
}

template <typename T, usize N> constexpr vec<T, N> operator*(const vec<T, N> &p_Left, const vec<T, N> &p_Right)
{
    vec<T, N> result;
    for (usize i = 0; i < N; ++i)
        result.Flat[i] = p_Left.Flat[i] * p_Right.Flat[i];
    return result;
}

template <typename T, usize N>
    requires(N == 2 || N == 3)
constexpr auto Cross(const vec<T, N> &p_Left, const vec<T, N> &p_Right)
{
    if constexpr (N == 2)
        return p_Left.Flat[0] * p_Right.Flat[1] - p_Left.Flat[1] * p_Right.Flat[0];
    else if constexpr (N == 3)
    {
        vec<T, 3> result;
        result.Flat[0] = p_Left.Flat[1] * p_Right.Flat[2] - p_Left.Flat[2] * p_Right.Flat[1];
        result.Flat[1] = p_Left.Flat[2] * p_Right.Flat[0] - p_Left.Flat[0] * p_Right.Flat[2];
        result.Flat[2] = p_Left.Flat[0] * p_Right.Flat[1] - p_Left.Flat[1] * p_Right.Flat[0];
        return result;
    }
    else
    {
        TKIT_UNREACHABLE();
    }
}
template <typename T, usize N> constexpr T DiagonalDeterminant(const mat<T, N> &p_Matrix)
{
    T sum = static_cast<T>(1);
    for (usize i = 0; i < N; ++i)
        sum *= p_Matrix[i][i];
    return sum;
}
template <typename T, usize N> constexpr T Determinant(const mat<T, N> &p_Matrix)
{
    if constexpr (N == 1)
        return p_Matrix.Flat[0];
    else if constexpr (N == 2)
        return p_Matrix.Flat[0] * p_Matrix.Flat[3] - p_Matrix.Flat[1] * p_Matrix.Flat[2];
    else if constexpr (N == 3)
        return p_Matrix.Flat[0] * p_Matrix.Flat[4] * p_Matrix.Flat[8] +
               p_Matrix.Flat[3] * p_Matrix.Flat[7] * p_Matrix.Flat[2] +
               p_Matrix.Flat[1] * p_Matrix.Flat[5] * p_Matrix.Flat[6] -
               p_Matrix.Flat[6] * p_Matrix.Flat[4] * p_Matrix.Flat[2] -
               p_Matrix.Flat[1] * p_Matrix.Flat[3] * p_Matrix.Flat[8] -
               p_Matrix.Flat[0] * p_Matrix.Flat[7] * p_Matrix.Flat[5];
    else if constexpr (N == 4)
    {
        const T factor0 = p_Matrix[2][2] * p_Matrix[3][3] - p_Matrix[3][2] * p_Matrix[2][3];
        const T factor1 = p_Matrix[2][1] * p_Matrix[3][3] - p_Matrix[3][1] * p_Matrix[2][3];
        const T factor2 = p_Matrix[2][1] * p_Matrix[3][2] - p_Matrix[3][1] * p_Matrix[2][2];
        const T factor3 = p_Matrix[2][0] * p_Matrix[3][3] - p_Matrix[3][0] * p_Matrix[2][3];
        const T factor4 = p_Matrix[2][0] * p_Matrix[3][2] - p_Matrix[3][0] * p_Matrix[2][2];
        const T factor5 = p_Matrix[2][0] * p_Matrix[3][1] - p_Matrix[3][0] * p_Matrix[2][1];

        const vec4<T> coef(+(p_Matrix[1][1] * factor0 - p_Matrix[1][2] * factor1 + p_Matrix[1][3] * factor2),
                           -(p_Matrix[1][0] * factor0 - p_Matrix[1][2] * factor3 + p_Matrix[1][3] * factor4),
                           +(p_Matrix[1][0] * factor1 - p_Matrix[1][1] * factor3 + p_Matrix[1][3] * factor5),
                           -(p_Matrix[1][0] * factor2 - p_Matrix[1][1] * factor4 + p_Matrix[1][2] * factor5));

        return p_Matrix[0][0] * coef[0] + p_Matrix[0][1] * coef[1] + p_Matrix[0][2] * coef[2] +
               p_Matrix[0][3] * coef[3];
    }
    else
    {
        T determinant{static_cast<T>(0)};
        for (usize i = 0; i < N; i += 2)
            determinant += p_Matrix[0][i] * Determinant(SubTensor(p_Matrix, 0, i));
        for (usize i = 1; i < N; i += 2)
            determinant -= p_Matrix[0][i] * Determinant(SubTensor(p_Matrix, 0, i));
        return determinant;
    }
}

template <typename T, usize N> constexpr mat<T, N> Cofactors(const mat<T, N> &p_Matrix)
{
    mat<T, N> cofactors;
    for (usize i = 0; i < N; ++i)
        for (usize j = 0; j < N; ++j)
            cofactors[i][j] = (1 - 2 * (static_cast<i32>(i + j) & 1)) * Determinant(SubTensor(p_Matrix, i, j));
    return cofactors;
}

template <typename T, usize N> constexpr mat<T, N> Inverse(const mat<T, N> &p_Matrix)
{
    if constexpr (N == 1)
        return 1.f / p_Matrix.Flat[0];
    else if constexpr (N == 2)
    {
        const T idet = static_cast<T>(1) / Determinant(p_Matrix);
        mat2<T> inverse;
        inverse.Flat[0] = idet * p_Matrix.Flat[3];
        inverse.Flat[1] = -idet * p_Matrix.Flat[1];
        inverse.Flat[2] = -idet * p_Matrix.Flat[2];
        inverse.Flat[3] = idet * p_Matrix.Flat[0];
        return inverse;
    }
    else if constexpr (N == 3)
    {
        const T idet = static_cast<T>(1) / Determinant(p_Matrix);
        mat3<T> inverse;

        inverse[0][0] = (p_Matrix[1][1] * p_Matrix[2][2] - p_Matrix[2][1] * p_Matrix[1][2]) * idet;
        inverse[1][0] = -(p_Matrix[1][0] * p_Matrix[2][2] - p_Matrix[2][0] * p_Matrix[1][2]) * idet;
        inverse[2][0] = (p_Matrix[1][0] * p_Matrix[2][1] - p_Matrix[2][0] * p_Matrix[1][1]) * idet;
        inverse[0][1] = -(p_Matrix[0][1] * p_Matrix[2][2] - p_Matrix[2][1] * p_Matrix[0][2]) * idet;
        inverse[1][1] = (p_Matrix[0][0] * p_Matrix[2][2] - p_Matrix[2][0] * p_Matrix[0][2]) * idet;
        inverse[2][1] = -(p_Matrix[0][0] * p_Matrix[2][1] - p_Matrix[2][0] * p_Matrix[0][1]) * idet;
        inverse[0][2] = (p_Matrix[0][1] * p_Matrix[1][2] - p_Matrix[1][1] * p_Matrix[0][2]) * idet;
        inverse[1][2] = -(p_Matrix[0][0] * p_Matrix[1][2] - p_Matrix[1][0] * p_Matrix[0][2]) * idet;
        inverse[2][2] = (p_Matrix[0][0] * p_Matrix[1][1] - p_Matrix[1][0] * p_Matrix[0][1]) * idet;
        return inverse;
    }
    else if constexpr (N == 4)
    {
        const T coef00 = p_Matrix[2][2] * p_Matrix[3][3] - p_Matrix[3][2] * p_Matrix[2][3];
        const T coef02 = p_Matrix[1][2] * p_Matrix[3][3] - p_Matrix[3][2] * p_Matrix[1][3];
        const T coef03 = p_Matrix[1][2] * p_Matrix[2][3] - p_Matrix[2][2] * p_Matrix[1][3];

        const T coef04 = p_Matrix[2][1] * p_Matrix[3][3] - p_Matrix[3][1] * p_Matrix[2][3];
        const T coef06 = p_Matrix[1][1] * p_Matrix[3][3] - p_Matrix[3][1] * p_Matrix[1][3];
        const T coef07 = p_Matrix[1][1] * p_Matrix[2][3] - p_Matrix[2][1] * p_Matrix[1][3];

        const T coef08 = p_Matrix[2][1] * p_Matrix[3][2] - p_Matrix[3][1] * p_Matrix[2][2];
        const T coef10 = p_Matrix[1][1] * p_Matrix[3][2] - p_Matrix[3][1] * p_Matrix[1][2];
        const T coef11 = p_Matrix[1][1] * p_Matrix[2][2] - p_Matrix[2][1] * p_Matrix[1][2];

        const T coef12 = p_Matrix[2][0] * p_Matrix[3][3] - p_Matrix[3][0] * p_Matrix[2][3];
        const T coef14 = p_Matrix[1][0] * p_Matrix[3][3] - p_Matrix[3][0] * p_Matrix[1][3];
        const T coef15 = p_Matrix[1][0] * p_Matrix[2][3] - p_Matrix[2][0] * p_Matrix[1][3];

        const T coef16 = p_Matrix[2][0] * p_Matrix[3][2] - p_Matrix[3][0] * p_Matrix[2][2];
        const T coef18 = p_Matrix[1][0] * p_Matrix[3][2] - p_Matrix[3][0] * p_Matrix[1][2];
        const T coef19 = p_Matrix[1][0] * p_Matrix[2][2] - p_Matrix[2][0] * p_Matrix[1][2];

        const T coef20 = p_Matrix[2][0] * p_Matrix[3][1] - p_Matrix[3][0] * p_Matrix[2][1];
        const T coef22 = p_Matrix[1][0] * p_Matrix[3][1] - p_Matrix[3][0] * p_Matrix[1][1];
        const T coef23 = p_Matrix[1][0] * p_Matrix[2][1] - p_Matrix[2][0] * p_Matrix[1][1];

        const vec4<T> fac0(coef00, coef00, coef02, coef03);
        const vec4<T> fac1(coef04, coef04, coef06, coef07);
        const vec4<T> fac2(coef08, coef08, coef10, coef11);
        const vec4<T> fac3(coef12, coef12, coef14, coef15);
        const vec4<T> fac4(coef16, coef16, coef18, coef19);
        const vec4<T> fac5(coef20, coef20, coef22, coef23);

        const vec4<T> vec0(p_Matrix[1][0], p_Matrix[0][0], p_Matrix[0][0], p_Matrix[0][0]);
        const vec4<T> vec1(p_Matrix[1][1], p_Matrix[0][1], p_Matrix[0][1], p_Matrix[0][1]);
        const vec4<T> vec2(p_Matrix[1][2], p_Matrix[0][2], p_Matrix[0][2], p_Matrix[0][2]);
        const vec4<T> vec3(p_Matrix[1][3], p_Matrix[0][3], p_Matrix[0][3], p_Matrix[0][3]);

        const vec4<T> inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
        const vec4<T> inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
        const vec4<T> inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
        const vec4<T> inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

        const vec4<T> signA(+1, -1, +1, -1);
        const vec4<T> signB(-1, +1, -1, +1);

        const mat4<T> inverse{inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB};

        const vec4<T> row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);

        const vec4<T> dot0{p_Matrix[0] * row0};
        T Dot1 = (dot0.Flat[0] + dot0.Flat[1]) + (dot0.Flat[2] + dot0.Flat[3]);

        const T idet = static_cast<T>(1) / Dot1;

        return inverse * idet;
    }
    else
    {
        const T idet = static_cast<T>(1) / Determinant(p_Matrix);
        return Transpose(Cofactors(p_Matrix)) * idet;
    }
}

template <typename T, usize C, usize R> constexpr mat<T, R, C> Transpose(const mat<T, C, R> &p_Matrix)
{
    mat<T, R, C> transposed;
    for (usize i = 0; i < R; ++i)
        for (usize j = 0; j < C; ++j)
            transposed[i][j] = p_Matrix[j][i];
    return transposed;
}

} // namespace Math
} // namespace TKit
TKIT_COMPILER_WARNING_IGNORE_POP()
