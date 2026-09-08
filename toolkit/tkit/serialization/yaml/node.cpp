#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/node.hpp"
#include <ryml.hpp>

namespace TKit
{
static c4::csubstr toNative(const StringView str)
{
    return c4::csubstr{str.GetData(), str.GetSize()};
}
static StringView fromNative(const c4::csubstr str)
{
    return StringView{str.data(), usize(str.size())};
}
ConstYamlNode::ConstYamlNode()
{
    m_Data.Construct<ryml::ConstNodeRef>();
}
ConstYamlNode::ConstYamlNode(const ryml::ConstNodeRef &ref)
{
    m_Data.Construct<ryml::ConstNodeRef>(ref);
}
ConstYamlNode::ConstYamlNode(const ryml::NodeRef &ref)
{
    m_Data.Construct<ryml::ConstNodeRef>(ref);
}

ConstYamlNode ConstYamlNode::ByKey(const StringView key) const
{
    return (*get())[toNative(key)];
}

ConstYamlNode ConstYamlNode::operator[](const u32 idx) const
{
    return (*get())[idx];
}

bool ConstYamlNode::HasChild(const StringView key) const
{
    return get()->has_child(toNative(key));
}

ConstYamlNode ConstYamlNode::GetParent() const
{
    return get()->parent();
}

StringView ConstYamlNode::GetKey() const
{
    return fromNative(get()->key());
}
StringView ConstYamlNode::GetValue() const
{
    return fromNative(get()->val());
}
u32 ConstYamlNode::GetChildCount() const
{
    return get()->num_children();
}
u32 ConstYamlNode::GetId() const
{
    return get()->id();
}

ConstYamlNode ConstYamlNode::FirstChild() const
{
    return get()->first_child();
}
ConstYamlNode ConstYamlNode::NextSibling() const
{
    return get()->next_sibling();
}

template <typename T> YamlReadResult ConstYamlNode::TryReadKey(T &key) const
{
    const ryml::ReadResult res = get()->deserialize_key(&key);
    if (!res)
        return YamlReadResult::Error(res.node, "Failed to deserialize key");
    return YamlReadResult::Ok();
}

YamlNodeFlags ConstYamlNode::GetFlags() const
{
    return YamlNodeFlags(get()->type());
}

YamlNodeFlags ConstYamlNode::GetKeyFlags() const
{
    return get()->key_style();
}

ConstYamlNode::operator bool() const
{
    return get()->readable();
}

bool ConstYamlNode::IsMap() const
{
    return get()->is_map();
}
bool ConstYamlNode::IsSequence() const
{
    return get()->is_seq();
}
bool ConstYamlNode::IsContainer() const
{
    return get()->is_container();
}
bool ConstYamlNode::IsScalar() const
{
    return get()->has_val() && !get()->is_container();
}
bool ConstYamlNode::HasKey() const
{
    return get()->has_key();
}
bool ConstYamlNode::HasValue() const
{
    return get()->has_val();
}
bool ConstYamlNode::IsKeyValue() const
{
    return get()->is_keyval();
}

const ryml::Tree *ConstYamlNode::getHandle() const
{
    return get()->tree();
}

YamlNode::YamlNode()
{
    m_Data.Construct<ryml::NodeRef>();
}
YamlNode::YamlNode(const ryml::NodeRef &ref)
{
    m_Data.Construct<ryml::NodeRef>(ref);
}

ConstYamlNode YamlNode::ByKey(const StringView key) const
{
    return (*get())[toNative(key)];
}
YamlNode YamlNode::ByKey(const StringView key, const bool copyKey)
{
    get()->set_map();

    c4::csubstr k = toNative(key);
    if (copyKey)
        k = get()->tree()->copy_to_arena(k);
    return (*get())[k];
}

ConstYamlNode YamlNode::operator[](const u32 idx) const
{
    return (*get())[idx];
}

YamlNode YamlNode::operator[](const u32 idx)
{
    get()->set_seq();
    return (*get())[idx];
}

YamlNode YamlNode::Append()
{
    get()->set_seq();
    return get()->append_child();
}

ConstYamlNode YamlNode::GetParent() const
{
    return get()->parent();
}
YamlNode YamlNode::GetParent()
{
    return get()->parent();
}
bool YamlNode::HasChild(const StringView key) const
{
    return get()->has_child(toNative(key));
}

StringView YamlNode::GetKey() const
{
    return fromNative(get()->key());
}
StringView YamlNode::GetValue() const
{
    return fromNative(get()->val());
}
u32 YamlNode::GetChildCount() const
{
    return get()->num_children();
}
u32 YamlNode::GetId() const
{
    return get()->id();
}

ConstYamlNode YamlNode::FirstChild() const
{
    return get()->first_child();
}
ConstYamlNode YamlNode::NextSibling() const
{
    return get()->next_sibling();
}

YamlNode YamlNode::FirstChild()
{
    return get()->first_child();
}
YamlNode YamlNode::NextSibling()
{
    return get()->next_sibling();
}

void YamlNode::SetValue(const StringView val)
{
    get()->set_val(toNative(val));
}
void YamlNode::SetKey(const StringView key, const bool copy)
{
    c4::csubstr k = toNative(key);
    if (copy)
        k = get()->tree()->copy_to_arena(k);
    get()->set_key(k);
}

template <typename T> YamlReadResult YamlNode::TryReadKey(T &key) const
{
    const ryml::ReadResult res = get()->deserialize_key(&key);
    if (!res)
        return YamlReadResult::Error(res.node, "Failed to deserialize key");
    return YamlReadResult::Ok();
}
template <typename T> void YamlNode::WriteKey(const T &key)
{
    get()->save_key(key);
}

YamlNodeFlags YamlNode::GetFlags() const
{
    return YamlNodeFlags(get()->type());
}
void YamlNode::SetFlags(const YamlNodeFlags flags)
{
    if (flags & YamlNodeFlag_Style)
    {
        if (get()->has_val())
            get()->set_val_style(flags);
        else if (get()->is_container())
            get()->set_container_style(flags);
    }
    if (flags & ~YamlNodeFlag_Style)
        get()->change_type(flags);
}

YamlNodeFlags YamlNode::GetKeyStyle() const
{
    return get()->key_style();
}

void YamlNode::SetKeyStyle(const YamlNodeFlags flags)
{
    get()->set_key_style(flags);
}

YamlNode::operator bool() const
{
    return get()->readable();
}

bool YamlNode::IsMap() const
{
    return get()->is_map();
}
bool YamlNode::IsSequence() const
{
    return get()->is_seq();
}
bool YamlNode::IsContainer() const
{
    return get()->is_container();
}
bool YamlNode::IsScalar() const
{
    return get()->has_val() && !get()->is_container();
}
bool YamlNode::HasKey() const
{
    return get()->has_key();
}
bool YamlNode::HasValue() const
{
    return get()->has_val();
}
bool YamlNode::IsKeyValue() const
{
    return get()->is_keyval();
}
const ryml::Tree *YamlNode::getHandle() const
{
    return get()->tree();
}
template YamlReadResult ConstYamlNode::TryReadKey(u8 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(u16 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(u32 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(u64 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(i8 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(i16 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(i32 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(i64 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(f32 &) const;
template YamlReadResult ConstYamlNode::TryReadKey(f64 &) const;

template YamlReadResult YamlNode::TryReadKey(u8 &) const;
template YamlReadResult YamlNode::TryReadKey(u16 &) const;
template YamlReadResult YamlNode::TryReadKey(u32 &) const;
template YamlReadResult YamlNode::TryReadKey(u64 &) const;
template YamlReadResult YamlNode::TryReadKey(i8 &) const;
template YamlReadResult YamlNode::TryReadKey(i16 &) const;
template YamlReadResult YamlNode::TryReadKey(i32 &) const;
template YamlReadResult YamlNode::TryReadKey(i64 &) const;
template YamlReadResult YamlNode::TryReadKey(f32 &) const;
template YamlReadResult YamlNode::TryReadKey(f64 &) const;

template void YamlNode::WriteKey(const u8 &);
template void YamlNode::WriteKey(const u16 &);
template void YamlNode::WriteKey(const u32 &);
template void YamlNode::WriteKey(const u64 &);
template void YamlNode::WriteKey(const i8 &);
template void YamlNode::WriteKey(const i16 &);
template void YamlNode::WriteKey(const i32 &);
template void YamlNode::WriteKey(const i64 &);
template void YamlNode::WriteKey(const f32 &);
template void YamlNode::WriteKey(const f64 &);
} // namespace TKit

#ifdef TKIT_ENABLE_ENSURE
namespace TKit::Detail
{
void CheckYamlReadResult(const YamlReadResult &res, const ryml::Tree *tree)
{
    if (tree)
    {
        TKIT_ENSURE(res, "[TOOLKIT][YAML] Failed to read node with message - '{}' - Value: {} - Faulty id: {}",
                    fromNative(tree->val(res.GetError().NodeId)), res.GetError().Message, res.GetError().NodeId);
    }
    else
    {
        TKIT_ENSURE(res, "[TOOLKIT][YAML] Failed to read node with message - '{}' - Faulty id: {}",
                    res.GetError().Message, res.GetError().NodeId);
    }
}
} // namespace TKit::Detail
#endif
