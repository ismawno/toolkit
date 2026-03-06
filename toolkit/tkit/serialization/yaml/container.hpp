#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error                                                                                                             \
        "[TOOLKIT][YAML] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/container/array.hpp"
#include "tkit/container/span.hpp"
#include "tkit/container/weak_array.hpp"

// Alias to the std are not included as yaml-cpp already includes them

namespace TKit::Yaml
{
template <typename T, usize N> struct Codec<FixedArray<T, N>>
{
    static Node Encode(const FixedArray<T, N> &instance)
    {
        Node node;
        for (const T &element : instance)
            node.push_back(element);
        return node;
    }

    static bool Decode(const Node &node, FixedArray<T, N> &instance)
    {
        if (!node.IsSequence() || node.size() > N)
            return false;

        for (usize i = 0; i < N; ++i)
            instance[i] = node[i].template as<T>();
        return true;
    }
};

template <typename T, typename AllocState> struct Codec<Array<T, AllocState>>
{
    static Node Encode(const Array<T, AllocState> &instance)
    {
        Node node;
        for (const T &element : instance)
            node.push_back(element);
        return node;
    }

    static bool Decode(const Node &node, Array<T, AllocState> &instance)
    {
        if (!node.IsSequence())
            return false;

        instance.Reserve(usize(node.size()));
        for (const Node &element : node)
            instance.Append(element.template as<T>());
        return true;
    }
};

template <typename T, usize N> struct Codec<Span<T, N>>
{
    static Node Encode(const Span<const T, N> &instance)
    {
        Node node;
        for (const T &element : instance)
            node.push_back(element);
        return node;
    }
};

template <typename T, usize N> struct Codec<WeakArray<T, N>>
{
    static Node Encode(const WeakArray<const T, N> &instance)
    {
        Node node;
        for (const T &element : instance)
            node.push_back(element);
        return node;
    }
};

} // namespace TKit::Yaml
