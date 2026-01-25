#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/math/quaternion.hpp"

namespace TKit::Yaml
{
template <typename T> struct Codec<qua<T>>
{
    static Node Encode(const qua<T> &instance)
    {
        Node node;
        for (usize i = 0; i < 4; ++i)
            node.push_back(instance[i]);
        node.SetStyle(YAML::EmitterStyle::Flow);
        return node;
    }

    static bool Decode(const Node &node, qua<T> &instance)
    {
        if (!node.IsSequence() || node.size() != 4)
            return false;

        for (usize i = 0; i < 4; ++i)
            instance[i] = node[i].as<T>();
        return true;
    }
};
} // namespace TKit::Yaml
