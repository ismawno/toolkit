#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error                                                                                                             \
        "[TOOLKIT][YAML] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/container/array.hpp"
#include "tkit/container/span.hpp"

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
        if constexpr (Array<T, AllocState>::IsString)
            node = instance.GetData();
        else
            for (const T &element : instance)
                node.push_back(element);
        return node;
    }

    static bool Decode(const Node &node, Array<T, AllocState> &instance)
    {
        if constexpr (Array<T, AllocState>::IsString)
        {
            if (!node.IsScalar())
                return false;

            const std::string str = node.as<std::string>();
            instance.Reserve(usize(str.size()));
            for (const char c : str)
                instance.Append(c);
        }
        else
        {
            if (!node.IsSequence())
                return false;

            instance.Reserve(usize(node.size()));
            for (const Node &element : node)
                instance.Append(element.template as<T>());
        }
        return true;
    }
};

template <typename T> struct Codec<Span<T>>
{
    static Node Encode(const Span<const T> &instance)
    {
        Node node;
        if constexpr (Span<T>::IsString)
            node = instance.GetData();
        else
            for (const T &element : instance)
                node.push_back(element);
        return node;
    }
};

} // namespace TKit::Yaml
