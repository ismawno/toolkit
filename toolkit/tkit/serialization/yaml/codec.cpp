#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include <ryml_std.hpp>

namespace TKit
{
template <BuiltInCodecable T> void YamlCodec<T>::Encode(YamlNode &node, const T &instance)
{
    ryml::Tree *tree = node.m_Tree;
    tree->save(node.m_Id, instance);
}

template <BuiltInCodecable T> YamlReadResult YamlCodec<T>::Decode(const YamlNode &node, T &instance)
{
    const ryml::Tree *tree = node.m_Tree;
    const ryml::ReadResult res = tree->deserialize(node.m_Id, &instance);
    if (!res)
        return YamlReadResult::Error(res.node, "Failed to deserialize built-in value");

    return YamlReadResult::Ok();
}

void YamlCodec<std::string_view>::Encode(YamlNode &node, const std::string_view &instance)
{
    ryml::Tree *tree = node.m_Tree;
    tree->save(node.m_Id, instance);
}

template struct YamlCodec<u8>;
template struct YamlCodec<u16>;
template struct YamlCodec<u32>;
template struct YamlCodec<u64>;

template struct YamlCodec<i8>;
template struct YamlCodec<i16>;
template struct YamlCodec<i32>;
template struct YamlCodec<i64>;

template struct YamlCodec<f32>;
template struct YamlCodec<f64>;

template struct YamlCodec<bool>;

template struct YamlCodec<char *>;
template struct YamlCodec<std::string>;
} // namespace TKit
