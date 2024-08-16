#pragma once

#include "kit/core/alias.hpp"

KIT_NAMESPACE_BEGIN

namespace Literals
{
consteval u64 operator"" _b(const u64 p_Value)
{
    return p_Value;
}

consteval u64 operator"" _kb(const u64 p_Value)
{
    return p_Value * 1024_b;
}

consteval u64 operator"" _mb(const u64 p_Value)
{
    return p_Value * 1024_kb;
}

consteval u64 operator"" _gb(const u64 p_Value)
{
    return p_Value * 1024_mb;
}

consteval u64 operator"" _tb(const u64 p_Value)
{
    return p_Value * 1024_gb;
}
}; // namespace Literals

KIT_NAMESPACE_END