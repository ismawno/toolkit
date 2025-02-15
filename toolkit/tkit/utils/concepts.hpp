#pragma once

#include "tkit/utils/alias.hpp"
#include <functional>
#include <concepts>

namespace TKit
{
template <typename T> using NoCVRef = std::remove_cvref_t<T>;

template <typename T>
concept Iterable = requires(T a) {
    {
        a.begin()
    } -> std::input_iterator;
    {
        a.end()
    } -> std::input_iterator;
};

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

} // namespace TKit