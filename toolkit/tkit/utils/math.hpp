#pragma once

namespace TKit
{
template <std::floating_point Float> bool ApproachesZero(const Float x)
{
    return std::abs(x) <= std::numeric_limits<Float>::epsilon();
}

template <std::floating_point Float> bool Approximately(const Float a, const Float b)
{
    return ApproachesZero(a - b);
}

template <typename Vec2> auto Cross2D(const Vec2 &a, const Vec2 &b)
{
    return a.x * b.y - a.y * b.x;
}

} // namespace TKit
