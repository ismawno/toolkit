#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include <ryml.hpp>

namespace TKit
{
template <BuiltInCodecable T> void Codec<T>::Encode(YamlNode &node, const T &instance)
{
    ryml::Tree *tree = node.m_Tree;
    tree->save(node.m_Id, instance);
}

template <BuiltInCodecable T> YamlReadResult Codec<T>::Decode(const YamlNode &node, T &instance)
{
    const ryml::Tree *tree = node.m_Tree;
    const ryml::ReadResult res = tree->deserialize(node.m_Id, &instance);
    if (!res)
        return YamlReadResult::Error(res.node, "Failed to deserialize built-in value");

    return YamlReadResult::Ok();
}
} // namespace TKit
