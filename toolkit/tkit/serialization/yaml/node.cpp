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

YamlNode YamlNode::operator[](const StringView str) const
{
    TKIT_ASSERT(GetFlags() & YamlNodeFlag_Map,
                "[TOOLKIT][YAML] To access a read-only node by key, parent node must be a map");
    const u32 id = m_Tree->find_child(m_Id, toNative(str));
    return {m_Tree, id};
}
YamlNode YamlNode::operator[](const u32 idx) const
{
    TKIT_ASSERT(GetFlags() & YamlNodeFlag_Sequence,
                "[TOOLKIT][YAML] To access a read-only node by index, parent node must be a sequence");
    const u32 id = m_Tree->child(m_Id, idx);
    TKIT_ASSERT(id != ryml::NONE, "[TOOLKIT][YAML] Child with index {} not found", idx);
    return {m_Tree, id};
}

YamlNode YamlNode::operator[](const StringView str)
{
    TKIT_ASSERT(!(GetFlags() & YamlNodeFlag_Sequence), "[TOOLKIT][YAML] Cannot access a sequence node by key");
    if (!(GetFlags() & YamlNodeFlag_Map))
        AddFlags(YamlNodeFlag_Map);

    const c4::csubstr cstr = toNative(str);
    u32 id = m_Tree->find_child(m_Id, cstr);
    if (id == ryml::NONE)
    {
        id = m_Tree->append_child(m_Id);
        m_Tree->set_key(id, cstr);
    }
    return {m_Tree, id};
}
YamlNode YamlNode::operator[](const u32 idx)
{
    TKIT_ASSERT(!(GetFlags() & YamlNodeFlag_Map), "[TOOLKIT][YAML] Cannot access a map node by index");
    if (!(GetFlags() & YamlNodeFlag_Sequence))
        AddFlags(YamlNodeFlag_Sequence);

    const u32 id = m_Tree->child(m_Id, idx);
    TKIT_ASSERT(id != ryml::NONE, "[TOOLKIT][YAML] Child with index {} not found", idx);
    return {m_Tree, id};
}

YamlNode YamlNode::Append()
{
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

void YamlNode::SetValue(const StringView str)
{
    m_Tree->set_val(m_Id, toNative(str));
}
void YamlNode::SetKey(const StringView str)
{
    m_Tree->set_key(m_Id, toNative(str));
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
