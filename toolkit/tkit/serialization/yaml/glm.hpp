#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/utils/glm.hpp"

namespace TKit::Yaml
{
template <glm::length_t L, typename T, glm::qualifier Q> struct Codec<glm::vec<L, T, Q>>
{
    static Node Encode(const glm::vec<L, T, Q> &p_Instance) noexcept
    {
        Node node;
        for (usize i = 0; i < L; ++i)
            node.push_back(p_Instance[i]);
        node.SetStyle(YAML::EmitterStyle::Flow);
        return node;
    }

    static bool Decode(const Node &p_Node, glm::vec<L, T, Q> &p_Instance) noexcept
    {
        if (!p_Node.IsSequence() || p_Node.size() != L)
            return false;

        for (usize i = 0; i < L; ++i)
            p_Instance[i] = p_Node[i].as<T>();
        return true;
    }
};
} // namespace TKit::Yaml