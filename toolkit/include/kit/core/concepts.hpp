#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include <concepts>

KIT_NAMESPACE_BEGIN

template <typename T>
concept Hashable = requires(T a) {
    {
        std::hash<T>()(a)
    } -> std::convertible_to<usz>;
};

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

template <typename T>
concept Integer = std::is_integral_v<T>;

template <typename T>
concept Float = std::is_floating_point_v<T>;

KIT_NAMESPACE_END