#pragma once

#include "tkit/serialization/yaml/codec.hpp"
#include "tkit/math/tensor.hpp"

namespace TKit
{
template <typename T, usize N0, usize... N> struct YamlCodec<ten<T, N0, N...>>
{
    static void Encode(YamlNode &node, const ten<T, N0, N...> &instance)
    {
        for (usize i = 0; i < (N0 * ... * N); ++i)
            node.Append(instance.Flat(i));
        node |= YamlNodeFlag_ContainerFlow;
    }

    static YamlReadResult Decode(const ConstYamlNode &node, ten<T, N0, N...> &instance)
    {
        constexpr usize size = (N0 * ... * N);
        if (!node.IsSequence() || node.GetChildCount() != size)
            return YamlReadResult::Error(
                node.GetId(), TierString::Format("Failed to decode: Child count ({}) is not equal to {} for tensor "
                                                 "or the node is not a sequence",
                                                 node.GetChildCount(), size));
        for (usize i = 0; i < (N0 * ... * N); ++i)
        {
            TKIT_RETURN_IF_FAILED(node[i].TryRead(instance.Flat(i)));
        }
        return YamlReadResult::Ok();
    }
};
} // namespace TKit
