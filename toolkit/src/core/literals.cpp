#include "core/pch.hpp"
#include "kit/core/literals.hpp"

KIT_NAMESPACE_BEGIN

namespace Literals
{
KIT_CONSTEVAL ulong operator"" _b(const ulong p_Value)
{
    return p_Value;
}

KIT_CONSTEVAL ulong operator"" _kb(const ulong p_Value)
{
    return p_Value * 1024_b;
}

KIT_CONSTEVAL ulong operator"" _mb(const ulong p_Value)
{
    return p_Value * 1024_kb;
}

KIT_CONSTEVAL ulong operator"" _gb(const ulong p_Value)
{
    return p_Value * 1024_mb;
}

KIT_CONSTEVAL ulong operator"" _tb(const ulong p_Value)
{
    return p_Value * 1024_gb;
}
} // namespace Literals

KIT_NAMESPACE_END
