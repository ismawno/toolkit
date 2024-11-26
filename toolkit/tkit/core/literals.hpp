#pragma once

#include "tkit/core/alias.hpp"

namespace TKit::Literals
{

TKIT_CONSTEVAL unsigned long long operator"" _b(const unsigned long long p_Value)
{
    return p_Value;
}

TKIT_CONSTEVAL unsigned long long operator"" _kb(const unsigned long long p_Value)
{
    return p_Value * 1024_b;
}

TKIT_CONSTEVAL unsigned long long operator"" _mb(const unsigned long long p_Value)
{
    return p_Value * 1024_kb;
}

TKIT_CONSTEVAL unsigned long long operator"" _gb(const unsigned long long p_Value)
{
    return p_Value * 1024_mb;
}

TKIT_CONSTEVAL unsigned long long operator"" _tb(const unsigned long long p_Value)
{
    return p_Value * 1024_gb;
}
}; // namespace TKit::Literals
