#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/math/quaternion.hpp"

namespace TKit::Yaml
{
template <typename T> struct Codec<qua<T>>
{
    static Node Encode(const qua<T> &p_Instance)
    {
        Node node;
        for (usize i = 0; i < 4; ++i)
            node.push_back(p_Instance[i]);
        node.SetStyle(YAML::EmitterStyle::Flow);
        return node;
    }

    static bool Decode(const Node &p_Node, qua<T> &p_Instance)
    {
        if (!p_Node.IsSequence() || p_Node.size() != 4)
            return false;

        for (usize i = 0; i < 4; ++i)
            p_Instance[i] = p_Node[i].as<T>();
        return true;
    }
};
} // namespace TKit::Yaml
