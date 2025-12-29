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
} // namespace TKit
