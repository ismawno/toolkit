#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/math/quaternion.hpp"

namespace TKit
{
template <typename T> struct YamlCodec<qua<T>>
{
    static void Encode(YamlNode &node, const qua<T> &instance)
    {
        for (usize i = 0; i < 4; ++i)
            node.Append(instance[i]);
        node |= YamlNodeFlag_FlowSingleLine;
    }

    TKIT_NO_DISCARD static YamlReadResult Decode(const ConstYamlNode &node, qua<T> &instance)
    {
        if (!node.IsSequence())
            return YamlReadResult::Error(
                node.GetId(), YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a quaternion: expected a "
                                                 "sequence of 4 components, but got {}",
                                                 YamlGeneratePath(node), YamlDescribeNode(node)));

        if (node.GetChildCount() != 4)
            return YamlReadResult::Error(
                node.GetId(), YamlString::Format("[TOOLKIT][YAML] Failed to decode '{}' as a quaternion: expected 4 "
                                                 "components, but found {}",
                                                 YamlGeneratePath(node), node.GetChildCount()));

        for (usize i = 0; i < 4; ++i)
        {
            TKIT_RETURN_IF_FAILED(node[i].TryRead(instance[i]));
        }
        return YamlReadResult::Ok();
    }
};
} // namespace TKit
