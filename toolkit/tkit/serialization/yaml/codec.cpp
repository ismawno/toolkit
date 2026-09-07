#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include <ryml_std.hpp>

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

void Codec<std::string_view>::Encode(YamlNode &node, const std::string_view &instance)
{
    ryml::Tree *tree = node.m_Tree;
    tree->save(node.m_Id, instance);
}

template struct Codec<u8>;
template struct Codec<u16>;
template struct Codec<u32>;
template struct Codec<u64>;

template struct Codec<i8>;
template struct Codec<i16>;
template struct Codec<i32>;
template struct Codec<i64>;

template struct Codec<f32>;
template struct Codec<f64>;

template struct Codec<bool>;

template struct Codec<char *>;
template struct Codec<std::string>;
} // namespace TKit
