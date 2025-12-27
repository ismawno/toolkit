#pragma once

#include <cstdint>
#include <concepts>

#ifndef TKIT_USIZE_TYPE
#    define TKIT_USIZE_TYPE TKit::Alias::u32 // std::size_t
#endif
#ifndef TKIT_SSIZE_TYPE
#    define TKIT_SSIZE_TYPE TKit::Alias::i32 // std::ssize_t
#endif

#ifndef TKIT_DIFFERENCE_TYPE
#    define TKIT_DIFFERENCE_TYPE TKit::Alias::i32 // std::ptrdiff_t
#endif

namespace TKit::Alias
{
// Add a namespace so that other libraries can adopt them...
// About aliases:
// Regarding primitive types, I have always liked short and informative names such as u32, f32, etc. I havent seen
// this convention in many c++ projects around, so Im not sure if its the best option, especially for the size_t
// case, which doesnt have an immediate alias I can think of.

// Regarding container types, its nice to have some of them aliases in case you want to quickly change the container
// type or allocator. This is not the case for std::string or std::byte, which dont have a meaningful justification
// for an alias, but I have added them for "increased" consistency.

using f32 = float;
using f64 = double;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using usize = TKIT_USIZE_TYPE;
using ssize = TKIT_SSIZE_TYPE;
using uptr = std::uintptr_t;
using idiff = TKIT_DIFFERENCE_TYPE;

template <typename T>
concept Float = std::same_as<T, f32> || std::same_as<T, f64>;

template <typename T>
concept UnsignedInteger = std::same_as<T, u8> || std::same_as<T, u16> || std::same_as<T, u32> || std::same_as<T, u64>;
template <typename T>
concept SignedInteger = std::same_as<T, i8> || std::same_as<T, i16> || std::same_as<T, i32> || std::same_as<T, i64>;

template <typename T>
concept Integer = UnsignedInteger<T> || SignedInteger<T>;

template <typename T>
concept Arithmetic = Float<T> || Integer<T>;
} // namespace TKit::Alias

//... and use it immediately in the toolkit namespace
namespace TKit
{
using namespace Alias;
}

namespace TKit::Detail
{
template <usize Size> struct Primitive;

template <> struct Primitive<8>
{
    using Unsigned = u8;
    using Signed = i8;
};
template <> struct Primitive<16>
{
    using Unsigned = u16;
    using Signed = i16;
};
template <> struct Primitive<32>
{
    using Unsigned = u32;
    using Signed = i32;
    using Float = f32;
};
template <> struct Primitive<64>
{
    using Unsigned = u64;
    using Signed = i64;
    using Float = f64;
};

} // namespace TKit::Detail

namespace TKit::Alias
{
template <usize Size> using u = typename Detail::Primitive<Size>::Unsigned;
template <usize Size> using i = typename Detail::Primitive<Size>::Signed;
template <usize Size> using f = typename Detail::Primitive<Size>::Float;
} // namespace TKit::Alias
