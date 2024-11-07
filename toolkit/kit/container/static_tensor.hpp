#pragma once

#include "kit/container/static_array.hpp"

namespace KIT
{
template <typename T, usize N, usize... Ns> class StaticTensor : public StaticArray<StaticArray<T, Ns...>, N>
{
    using StaticArray<StaticArray<T, Ns...>, N>::StaticArray;
};

template <typename T, usize N> class StaticTensor<T, N> : public StaticArray<T, N>
{
    using StaticArray<T, N>::StaticArray;
};
} // namespace KIT