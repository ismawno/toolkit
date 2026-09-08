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

    static YamlReadResult Decode(const ConstYamlNode &node, qua<T> &instance)
    {
        if (!node.IsSequence() || node.GetChildCount() != 4)
            return YamlReadResult::Error(
                node.GetId(), TierString::Format("Failed to decode: Child count ({}) is not equal to 4 for quaternion "
                                                 "or the node is not a sequence",
                                                 node.GetChildCount()));

        for (usize i = 0; i < 4; ++i)
        {
            TKIT_RETURN_IF_FAILED(node[i].TryRead(instance[i]));
        }
        return YamlReadResult::Ok();
    }
};
} // namespace TKit
