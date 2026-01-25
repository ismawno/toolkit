#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/math/tensor.hpp"

namespace TKit::Yaml
{
template <typename T, usize N0, usize... N> struct Codec<ten<T, N0, N...>>
{
    static Node Encode(const ten<T, N0, N...> &instance)
    {
        Node node;
        for (usize i = 0; i < (N0 * ... * N); ++i)
            node.push_back(instance.Flat[i]);
        node.SetStyle(YAML::EmitterStyle::Flow);
        return node;
    }

    static bool Decode(const Node &node, ten<T, N0, N...> &instance)
    {
        if (!node.IsSequence() || node.size() != (N0 * ... * N))
            return false;

        for (usize i = 0; i < (N0 * ... * N); ++i)
            instance.Flat[i] = node[i].as<T>();
        return true;
    }
};
} // namespace TKit::Yaml
