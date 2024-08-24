#pragma once

#include "kit/core/alias.hpp"

KIT_NAMESPACE_BEGIN

namespace Literals
{
using ulong = unsigned long long;
KIT_API KIT_CONSTEVAL ulong operator"" _b(const ulong p_Value);
KIT_API KIT_CONSTEVAL ulong operator"" _kb(const ulong p_Value);
KIT_API KIT_CONSTEVAL ulong operator"" _mb(const ulong p_Value);
KIT_API KIT_CONSTEVAL ulong operator"" _gb(const ulong p_Value);
KIT_API KIT_CONSTEVAL ulong operator"" _tb(const ulong p_Value);
}; // namespace Literals

KIT_NAMESPACE_END