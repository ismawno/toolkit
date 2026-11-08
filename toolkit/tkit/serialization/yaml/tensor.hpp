#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/math/tensor.hpp"

namespace TKit
{
template <typename T, usize N0, usize... N> struct YamlCodec<ten<T, N0, N...>>
{
    static void Encode(YamlNode &node, const ten<T, N0, N...> &instance)
    {
        constexpr usize size = (N0 * ... * N);
        for (usize i = 0; i < size; ++i)
            node.Append(instance.Flat(i));
        node |= YamlFlagsBasedOnContainer(size);
    }

    TKIT_NO_DISCARD static YamlReadResult Decode(const ConstYamlNode &node, ten<T, N0, N...> &instance)
    {
        constexpr usize size = (N0 * ... * N);
        if (!node.IsSequence())
            return YamlReadResult::Error(
                node.GetId(), YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a {} tensor: expected a "
                                                 "flat sequence of {} elements, but got {}",
                                                 YamlGeneratePath(node), shapeString(), size, YamlDescribeNode(node)));

        if (node.GetChildCount() != size)
            return YamlReadResult::Error(
                node.GetId(), YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a {} tensor: expected {} "
                                                 "elements, but found {}",
                                                 YamlGeneratePath(node), shapeString(), size, node.GetChildCount()));

        for (usize i = 0; i < size; ++i)
        {
            TKIT_RETURN_IF_FAILED(node[i].TryRead(instance.Flat(i)));
        }
        return YamlReadResult::Ok();
    }

  private:
    static YamlString shapeString()
    {
        YamlString shape = YamlString::Format("{}", N0);
        ((shape += YamlString::Format("x{}", N)), ...);
        return shape;
    }
};
} // namespace TKit
