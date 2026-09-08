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

YamlNode YamlNode::ByKey(const StringView key) const
{
    TKIT_ASSERT(IsMap(), "[TOOLKIT][YAML] To access a read-only node by key, parent node must be a map");
    const u32 id = m_Tree->find_child(m_Id, toNative(key));
    return {m_Tree, id};
}
YamlNode YamlNode::ByKey(const StringView key)
{
    TKIT_ASSERT(!IsSequence(), "[TOOLKIT][YAML] Cannot access a sequence node by key");
    if (!IsMap())
        AddFlags(YamlNodeFlag_Map);

    c4::csubstr cstr = toNative(key);
    u32 id = m_Tree->find_child(m_Id, cstr);
    if (id == ryml::NONE)
    {
        cstr = m_Tree->copy_to_arena(cstr);
        id = m_Tree->append_child(m_Id);
        m_Tree->set_key(id, cstr);
        m_Tree->set_val(id, "~");
    }
    return {m_Tree, id};
}

YamlNode YamlNode::operator[](const u32 idx) const
{
    const u32 id = m_Tree->child(m_Id, idx);
    TKIT_ASSERT(id != ryml::NONE, "[ONYX][YAML] Child with index {} was not found", idx);
    return {m_Tree, id};
}

YamlNode YamlNode::operator[](const u32 idx)
{
    const u32 id = m_Tree->child(m_Id, idx);
    TKIT_ASSERT(id != ryml::NONE, "[TOOLKIT][YAML] Child with index {} not found", idx);
    return {m_Tree, id};
}

YamlNode YamlNode::Append()
{
    TKIT_ASSERT(!IsMap(), "[TOOLKIT][YAML] Cannot append children to a map");
    if (!IsSequence())
        AddFlags(YamlNodeFlag_Sequence);
    return YamlNode{m_Tree, m_Tree->append_child(m_Id)};
}

YamlNode YamlNode::GetParent() const
{
    return YamlNode{m_Tree, m_Tree->parent(m_Id)};
}

StringView YamlNode::GetKey() const
{
    return fromNative(m_Tree->key(m_Id));
}
StringView YamlNode::GetValue() const
{
    return fromNative(m_Tree->val(m_Id));
}
u32 YamlNode::GetChildCount() const
{
    return m_Tree->num_children(m_Id);
}

YamlNode YamlNode::FirstChild() const
{
    return {m_Tree, m_Tree->first_child(m_Id)};
}
YamlNode YamlNode::NextSibling() const
{
    return {m_Tree, m_Tree->next_sibling(m_Id)};
}

void YamlNode::SetValue(const StringView val)
{
    m_Tree->set_val(m_Id, toNative(val));
}
void YamlNode::SetKey(const StringView key)
{
    const c4::csubstr cstr = m_Tree->copy_to_arena(toNative(key));
    m_Tree->set_key(m_Id, cstr);
}

template <typename T> YamlReadResult YamlNode::TryReadKey(T &key) const
{
    const ryml::ReadResult res = m_Tree->deserialize_key(m_Id, &key);
    if (!res)
        return YamlReadResult::Error(res.node, "Failed to deserialize key");
    return YamlReadResult::Ok();
}
template <typename T> void YamlNode::WriteKey(const T &key)
{
    m_Tree->save_key(m_Id, key);
}

YamlNodeFlags YamlNode::GetFlags() const
{
    return YamlNodeFlags(m_Tree->type(m_Id));
}
void YamlNode::SetFlags(const YamlNodeFlags flags)
{
    if (m_Tree->has_val(m_Id))
        m_Tree->set_val_style(m_Id, flags);
    if (flags & ~YamlNodeFlag_Style)
        m_Tree->change_type(m_Id, flags);
}

YamlNodeFlags YamlNode::GetKeyFlags() const
{
    return m_Tree->key_style(m_Id);
}

void YamlNode::SetKeyFlags(const YamlNodeFlags flags)
{
    m_Tree->set_key_style(m_Id, flags);
}
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
void CheckYamlReadResult(const YamlReadResult &res)
{
    TKIT_ENSURE(res, "[TOOLKIT][YAML] Failed to read node with message - '{}' - Faulty id: {}", res.GetError().Message,
                res.GetError().NodeId);
}
} // namespace TKit::Detail
#endif
