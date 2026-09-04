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

namespace TKit
{
template <typename T, usize N> struct Codec<FixedArray<T, N>>
{
    static void Encode(YamlNode &node, const FixedArray<T, N> &instance)
    {
        for (const T &element : instance)
            node.Append(element);
    }

    static YamlReadResult Decode(const YamlNode &node, FixedArray<T, N> &instance)
    {
        if (!(node.GetFlags() & YamlNodeFlag_Sequence) || node.GetChildCount() > N)
            return YamlReadResult::Error(node.GetId(),
                                         TierString::Format("Failed to decode: Child count ({}) is greater than array "
                                                            "capacity ({}) or the node is not a sequence",
                                                            node.GetChildCount(), N));

        for (usize i = 0; i < N; ++i)
        {
            TKIT_RETURN_IF_FAILED(node[i].Read(instance[i]));
        }
        return YamlReadResult::Ok();
    }
};

template <typename T, typename AllocState> struct Codec<Array<T, AllocState>>
{
    static void Encode(YamlNode &node, const Array<T, AllocState> &instance)
    {
        if constexpr (Array<T, AllocState>::IsString)
            node << instance.GetData();
        else
            for (const T &element : instance)
                node.Append(element);
    }

    static YamlReadResult Decode(const YamlNode &node, Array<T, AllocState> &instance)
    {
        if constexpr (Array<T, AllocState>::IsString)
        {
            std::string str;
            TKIT_RETURN_IF_FAILED(node.TryRead(str));
            if constexpr (Array<T, AllocState>::Type != Array_Static)
                instance.Reserve(usize(str.size()));
            else if (str.size() >= instance.GetCapacity())
                return YamlReadResult::Error(
                    node.GetId(), TierString::Format("Not enough capacity ({}) to deserialize string of size {}",
                                                     instance.GetCapacity(), str.size()));
            for (const char c : str)
                instance.Append(c);
        }
        else
        {
            if (!(node.GetFlags() & YamlNodeFlag_Sequence))
                return YamlReadResult::Error(node.GetId(), "Node must be a sequence to decode array");

            if constexpr (Array<T, AllocState>::Type != Array_Static)
                instance.Reserve(usize(node.GetChildCount()));
            else if (node.GetChildCount() >= instance.GetCapacity())
                return YamlReadResult::Error(
                    node.GetId(), TierString::Format("Not enough capacity ({}) to deserialize array of size {}",
                                                     instance.GetCapacity(), node.GetChildCount()));

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

} // namespace TKit
