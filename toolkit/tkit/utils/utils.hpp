#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/preprocessor/system.hpp"

namespace TKit
{
template <typename T0, typename... T> constexpr bool HasDuplicateTypes()
{
    if constexpr (sizeof...(T) == 0)
        return false;
    else
        return (std::is_same_v<T0, T> || ...) || HasDuplicateTypes<T...>();
}

template <typename T0, typename P0, typename... P> constexpr bool IsTypeContained()
{
    if constexpr (sizeof...(P) == 0)
        return std::is_same_v<T0, P0>;
    else
        return std::is_same_v<T0, P0> || IsTypeContained<T0, P...>();
}

template <typename T0, typename P0, typename... P> constexpr usize GetTypeIndex()
{
    if constexpr (std::is_same_v<T0, P0>)
        return 0;
    else if constexpr (sizeof...(P) != 0)
        return 1 + GetTypeIndex<T0, P...>();
}

inline const char *GetEnvironmentVariable(const char *name)
{
    TKIT_COMPILER_WARNING_IGNORE_PUSH()
    TKIT_MSVC_WARNING_IGNORE(4996)
    TKIT_CLANG_WARNING_IGNORE("-Wdeprecated-declarations")
    return std::getenv(name);
    TKIT_COMPILER_WARNING_IGNORE_POP()
}

} // namespace TKit
