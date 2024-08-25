#pragma once

#include "kit/core/alias.hpp"

KIT_NAMESPACE_BEGIN

namespace Literals
{
KIT_CONSTEVAL unsigned long long operator"" _b(const unsigned long long p_Value)
{
    return p_Value;
}

KIT_CONSTEVAL unsigned long long operator"" _kb(const unsigned long long p_Value)
{
    return p_Value * 1024_b;
}

KIT_CONSTEVAL unsigned long long operator"" _mb(const unsigned long long p_Value)
{
    return p_Value * 1024_kb;
}

KIT_CONSTEVAL unsigned long long operator"" _gb(const unsigned long long p_Value)
{
    return p_Value * 1024_mb;
}

KIT_CONSTEVAL unsigned long long operator"" _tb(const unsigned long long p_Value)
{
    return p_Value * 1024_gb;
}
}; // namespace Literals

KIT_NAMESPACE_END