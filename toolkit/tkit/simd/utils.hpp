#pragma once

#include "tkit/utils/alias.hpp"

namespace TKit::Simd
{

template <usize Lanes> constexpr usize MaskSize()
{
    if constexpr (Lanes < 8)
        return 8;
    return Lanes;
}

} // namespace TKit::Simd
