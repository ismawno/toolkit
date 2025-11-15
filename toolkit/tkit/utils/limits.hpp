#pragma once

#include "tkit/utils/alias.hpp"
#include <limits>

namespace TKit
{
template <typename T> struct Limits
{
    static constexpr T Min()
    {
        return std::numeric_limits<T>::min();
    }
    static constexpr T Max()
    {
        return std::numeric_limits<T>::max();
    }
    static constexpr T Epsilon()
    {
        return std::numeric_limits<T>::epsilon();
    }
};
template <typename T> constexpr bool ApproachesZero(const T p_Value)
{
    if constexpr (Float<T>)
        return std::abs(p_Value) <= Limits<T>::Epsilon();
    else
        return p_Value == static_cast<T>(0);
}

template <typename T> constexpr bool Approximately(const T p_Left, const T p_Right)
{
    return ApproachesZero(p_Left - p_Right);
}
} // namespace TKit
