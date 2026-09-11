#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/codec.hpp"
#include <ryml_std.hpp>

namespace TKit
{
namespace Detail
{
template <typename T> void EncodeBuiltIn(YamlNode &node, const T &instance)
{
    node.get()->save(instance);
}

template <typename T> YamlReadResult DecodeBuiltIn(const ConstYamlNode &node, T &instance)
{
    const ryml::ConstNodeRef *ref = node.get();
    const ryml::ReadResult res = ref->deserialize(&instance);
    if (!res)
        return YamlReadResult::Error(res.node, "Failed to deserialize built-in value");

    return YamlReadResult::Ok();
}

template void EncodeBuiltIn(YamlNode &, const u8 &);
template void EncodeBuiltIn(YamlNode &, const u16 &);
template void EncodeBuiltIn(YamlNode &, const u32 &);
template void EncodeBuiltIn(YamlNode &, const u64 &);
template void EncodeBuiltIn(YamlNode &, const i8 &);
template void EncodeBuiltIn(YamlNode &, const i16 &);
template void EncodeBuiltIn(YamlNode &, const i32 &);
template void EncodeBuiltIn(YamlNode &, const i64 &);
template void EncodeBuiltIn(YamlNode &, const f32 &);
template void EncodeBuiltIn(YamlNode &, const f64 &);
template void EncodeBuiltIn(YamlNode &, const bool &);
template void EncodeBuiltIn(YamlNode &, const std::string &);

template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, u8 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, u16 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, u32 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, u64 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, i8 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, i16 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, i32 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, i64 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, f32 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, f64 &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, bool &);
template YamlReadResult DecodeBuiltIn(const ConstYamlNode &, std::string &);
} // namespace Detail

void YamlCodec<const char *>::Encode(YamlNode &node, const char *str)
{
    node.get()->save(str);
}

void YamlCodec<std::string_view>::Encode(YamlNode &node, const std::string_view &instance)
{
    node.get()->save(instance);
}

} // namespace TKit
