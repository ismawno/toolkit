#pragma once

#include "tkit/core/api.hpp"
#include "tkit/core/alias.hpp"
#include <functional>
#include <concepts>

namespace TKit
{
template <typename T> using NoCVRef = std::remove_cvref_t<T>;

template <typename T>
concept Hashable = requires(T a) {
    {
        std::hash<NoCVRef<T>>()(a)
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

} // namespace TKit