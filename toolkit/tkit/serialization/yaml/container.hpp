#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/container/fixed_array.hpp"
#include "tkit/container/array.hpp"
#include "tkit/container/span.hpp"

// Alias to the std are not included as yaml-cpp already includes them

namespace TKit
{
template <typename T, usize N> struct YamlCodec<FixedArray<T, N>>
{
    static void Encode(YamlNode &node, const FixedArray<T, N> &instance)
    {
        for (const T &element : instance)
            node.Append(element);
        if constexpr (Numeric<T>)
            node |= YamlFlagsBasedOnContainer(N);
    }

    TKIT_NO_DISCARD static YamlReadResult Decode(const ConstYamlNode &node, FixedArray<T, N> &instance)
    {
        if (!node.IsSequence())
            return YamlReadResult::Error(
                node.GetId(), YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a fixed array of {} "
                                                 "elements: expected a sequence, but got {}",
                                                 YamlGeneratePath(node), N, YamlDescribeNode(node)));

        if (node.GetChildCount() > N)
            return YamlReadResult::Error(
                node.GetId(), YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a fixed array: it has {} "
                                                 "elements, but the array can hold at most {}",
                                                 YamlGeneratePath(node), node.GetChildCount(), N));

        for (usize i = 0; i < node.GetChildCount(); ++i)
        {
            TKIT_RETURN_IF_FAILED(node[i].TryRead(instance[i]));
        }
        return YamlReadResult::Ok();
    }
};

template <typename T, typename AllocState> struct YamlCodec<Array<T, AllocState>>
{
    static void Encode(YamlNode &node, const Array<T, AllocState> &instance)
    {
        if constexpr (Array<T, AllocState>::IsString)
            node << std::string_view{instance.GetData(), instance.GetSize()};
        else if (!instance.IsEmpty())
        {
            for (const T &element : instance)
                node.Append(element);
            if constexpr (Numeric<T>)
                node |= YamlFlagsBasedOnContainer(instance.GetSize());
        }
    }

    TKIT_NO_DISCARD static YamlReadResult Decode(const ConstYamlNode &node, Array<T, AllocState> &instance)
    {
        instance.Clear();
        if constexpr (Array<T, AllocState>::IsString)
        {
            std::string str;
            const YamlReadResult res = node.TryRead(str);
            if (!res)
                return YamlReadResult::Error(
                    node.GetId(),
                    YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a string: got {} - {}",
                                       YamlGeneratePath(node), YamlDescribeNode(node), res.GetError().Message));

            if constexpr (Array<T, AllocState>::Type != Array_Static)
                instance.Reserve(usize(str.size()));
            else if (str.size() >= instance.GetCapacity())
                return YamlReadResult::Error(
                    node.GetId(),
                    YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a string: its length ({}) does not "
                                       "fit in a static string with capacity {}",
                                       YamlGeneratePath(node), str.size(), instance.GetCapacity()));
            for (const char c : str)
                instance.Append(c);
        }
        else
        {
            if (!node.IsSequence())
                return YamlReadResult::Error(
                    node.GetId(),
                    YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as an array: expected a sequence, but "
                                       "got {}",
                                       YamlGeneratePath(node), YamlDescribeNode(node)));

            if constexpr (Array<T, AllocState>::Type != Array_Static)
                instance.Reserve(usize(node.GetChildCount()));
            else if (node.GetChildCount() > instance.GetCapacity())
                return YamlReadResult::Error(
                    node.GetId(),
                    YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a static array: it has {} elements, "
                                       "but the array can hold at most {}",
                                       YamlGeneratePath(node), node.GetChildCount(), instance.GetCapacity()));

            for (const ConstYamlNode element : node)
            {
                T &elm = instance.Append();
                TKIT_RETURN_IF_FAILED(element.TryRead(elm));
            }
        }
        return YamlReadResult::Ok();
    }
};

template <typename T> struct YamlCodec<Span<T>>
{
    static void Encode(YamlNode &node, const Span<const T> &instance)
    {
        if constexpr (Span<T>::IsString)
            node << std::string_view{instance.GetData(), instance.GetSize()};
        else if (!instance.IsEmpty())
        {
            for (const T &element : instance)
                node.Append(element);
            if constexpr (Numeric<T>)
                node |= YamlFlagsBasedOnContainer(instance.GetSize());
        }
    }
};

} // namespace TKit
