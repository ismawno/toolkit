#pragma once

#include "tkit/preprocessor/system.hpp"

#define TKIT_KIB(p_Value) (p_Value) * 1024
#define TKIT_MIB(p_Value) TKIT_KIB(p_Value) * 1024
#define TKIT_GIB(p_Value) TKIT_MIB(p_Value) * 1024

namespace TKit::Literals
{
TKIT_CONSTEVAL unsigned long long operator""_kib(const unsigned long long p_Value)
{
    return p_Value * 1024;
}

TKIT_CONSTEVAL unsigned long long operator""_mib(const unsigned long long p_Value)
{
    return p_Value * 1024_kib;
}

TKIT_CONSTEVAL unsigned long long operator""_gib(const unsigned long long p_Value)
{
    return p_Value * 1024_mib;
}
}; // namespace TKit::Literals

namespace TKit
{
using namespace Literals;
}
