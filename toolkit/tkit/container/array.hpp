#pragma once

#if !defined(TKIT_USE_STATIC_ARRAY) && !defined(TKIT_USE_DYNAMIC_ARRAY)
#    define TKIT_USE_DYNAMIC_ARRAY
#endif

#if defined(TKIT_USE_STATIC_ARRAY) && defined(TKIT_USE_DYNAMIC_ARRAY)
#    error                                                                                                             \
        "[TOOLKIT] Array usage mismatch! Both TKIT_USE_STATIC_ARRAY and TKIT_USE_DYNAMIC_ARRAY are defined. Cannot choose between which array to use"
#endif

#ifdef TKIT_USE_STATIC_ARRAY
#    include "tkit/container/static_array.hpp"
#elif defined(TKIT_USE_DYNAMIC_ARRAY)
#    include "tkit/container/dynamic_array.hpp"
#endif

namespace TKit
{
#ifdef TKIT_USE_STATIC_ARRAY
template <typename T, usize Capacity, typename Traits = Container::ArrayTraits<T>>
using Array = StaticArray<T, Capacity, Traits>;
#    define TKIT_ARRAY_RESERVE(p_Arr, p_Size)
#elif defined(TKIT_USE_DYNAMIC_ARRAY)
template <typename T, usize Capacity, typename Traits = Container::ArrayTraits<T>>
using Array = DynamicArray<T, Traits>;
#    define TKIT_ARRAY_RESERVE(p_Arr, p_Size) p_Arr.Reserve(p_Size)
#endif

template <typename T> using Array4 = Array<T, 4>;
template <typename T> using Array8 = Array<T, 8>;
template <typename T> using Array16 = Array<T, 16>;
template <typename T> using Array32 = Array<T, 32>;
template <typename T> using Array64 = Array<T, 64>;
template <typename T> using Array128 = Array<T, 128>;
template <typename T> using Array196 = Array<T, 196>;
template <typename T> using Array256 = Array<T, 256>;
template <typename T> using Array384 = Array<T, 384>;
template <typename T> using Array512 = Array<T, 512>;
template <typename T> using Array768 = Array<T, 768>;
template <typename T> using Array1024 = Array<T, 1024>;
} // namespace TKit
