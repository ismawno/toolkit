#pragma once

#include "kit/core/alias.hpp"

namespace KIT
{
template <std::floating_point Float> bool Approximately(const Float a, const Float b)
{
    return ApproachesZero(a - b);
}

template <std::floating_point Float> bool ApproachesZero(const Float x)
{
    return std::abs(x) <= std::numeric_limits<Float>::epsilon();
}

template <typename Vec2> auto Cross2D(const Vec2 &a, const Vec2 &b)
{
    return a.x * b.y - a.y * b.x;
}

} // namespace KIT