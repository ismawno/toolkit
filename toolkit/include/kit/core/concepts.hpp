#pragma once

#include "kit/core/core.hpp"
#include "kit/core/alias.hpp"
#include <concepts>

namespace KIT
{
template <typename T>
concept Hashable = requires(T a) {
    {
        std::hash<T>()(a)
    } -> std::convertible_to<usize>;
};

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

template <typename T>
concept Integer = std::is_integral_v<T>;

template <typename T>
concept Float = std::is_floating_point_v<T>;

template <typename T>
concept Mutex = requires(T a) {
    {
        a.lock()
    } -> std::same_as<void>;
    {
        a.unlock()
    } -> std::same_as<void>;
};

template <typename T> using NoCVRef = std::remove_cvref_t<T>;
} // namespace KIT