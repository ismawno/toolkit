#pragma once

#include "tkit/preprocessor/system.hpp"

#define TKIT_KIB(value) (value) * 1024
#define TKIT_MIB(value) TKIT_KIB(value) * 1024
#define TKIT_GIB(value) TKIT_MIB(value) * 1024

namespace TKit::Literals
{
TKIT_CONSTEVAL unsigned long long operator""_kib(const unsigned long long value)
{
    return value * 1024;
}

TKIT_CONSTEVAL unsigned long long operator""_mib(const unsigned long long value)
{
    return value * 1024_kib;
}

TKIT_CONSTEVAL unsigned long long operator""_gib(const unsigned long long value)
{
    return value * 1024_mib;
}
}; // namespace TKit::Literals

namespace TKit
{
using namespace Literals;
}
