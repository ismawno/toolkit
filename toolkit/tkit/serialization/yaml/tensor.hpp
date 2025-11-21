#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/math/tensor.hpp"

namespace TKit::Yaml
{
template <typename T, usize N0, usize... N> struct Codec<ten<T, N0, N...>>
{
    static Node Encode(const ten<T, N0, N...> &p_Instance)
    {
        Node node;
        for (usize i = 0; i < (N0 * ... * N); ++i)
            node.push_back(p_Instance.Flat[i]);
        node.SetStyle(YAML::EmitterStyle::Flow);
        return node;
    }

    static bool Decode(const Node &p_Node, ten<T, N0, N...> &p_Instance)
    {
        if (!p_Node.IsSequence() || p_Node.size() != (N0 * ... * N))
            return false;

        for (usize i = 0; i < (N0 * ... * N); ++i)
            p_Instance.Flat[i] = p_Node[i].as<T>();
        return true;
    }
};
} // namespace TKit::Yaml
