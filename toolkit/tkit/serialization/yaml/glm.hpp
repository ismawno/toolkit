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
template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct Codec<glm::mat<C, R, T, Q>> template <typename T, glm::qualifier Q>
{
    static Node Encode(const glm::mat<C, R, T, Q> &p_Instance) noexcept
    {
        Node node;
        for (usize i = 0; i < C; ++i)
            node.push_back(p_Instance[i]);
        return node;
    }

    static bool Decode(const Node &p_Node, glm::mat<C, R, T, Q> &p_Instance) noexcept
    {
        if (!p_Node.IsSequence() || p_Node.size() != C)
            return false;

        for (usize i = 0; i < C; ++i)
            p_Instance[i] = p_Node[i].as<glm::vec<R, T, Q>>();
        return true;
    }
};
struct Codec<glm::quat<T, Q>>
{
    static Node Encode(const glm::quat<T, Q> &p_Instance) noexcept
    {
        Node node;
        node.push_back(p_Instance.x);
        node.push_back(p_Instance.y);
        node.push_back(p_Instance.z);
        node.push_back(p_Instance.w);
        node.SetStyle(YAML::EmitterStyle::Flow);
        return node;
    }

    static bool Decode(const Node &p_Node, glm::quat<T, Q> &p_Instance) noexcept
    {
        if (!p_Node.IsSequence() || p_Node.size() != L)
            return false;

        p_Instance.x = p_Node[0].as<T>();
        p_Instance.y = p_Node[1].as<T>();
        p_Instance.z = p_Node[2].as<T>();
        p_Instance.w = p_Node[3].as<T>();
        return true;
    }
};
} // namespace TKit::Yaml
