#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/node.hpp"
#include <ryml.hpp>

namespace TKit
{
namespace ryml = c4::yml;
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
    const u32 id = m_Tree->find_child(m_Id, toNative(str));
    TKIT_ASSERT(id != ryml::NONE, "[TOOLKIT][YAML] Child '{}' not found", str);
    return {m_Tree, id};
}
YamlNode YamlNode::operator[](const u32 idx) const
{
    const u32 id = m_Tree->child(m_Id, idx);
    TKIT_ASSERT(id != ryml::NONE, "[TOOLKIT][YAML] Child with index {} not found", idx);
    return {m_Tree, id};
}

YamlNode YamlNode::operator[](const StringView str)
{
    u32 id = m_Tree->find_child(m_Id, toNative(str));
    if (id == ryml::NONE)
        id = m_Tree->append_child(m_Id);
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
    return YamlNode{m_Tree, m_Tree->append_child(m_Id)};
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

void YamlNode::SetValue(const StringView str)
{
    m_Tree->set_val(m_Id, toNative(str));
}

YamlNodeFlags YamlNode::GetFlags() const
{
    return YamlNodeFlags(m_Tree->type(m_Id));
}
void YamlNode::SetKeyFlags(const YamlNodeFlags flags)
{
    ryml::NodeRef r;
    m_Tree->set_key_style(m_Id, flags);
}
void YamlNode::SetValueFlags(const YamlNodeFlags flags)
{
    m_Tree->set_val_style(m_Id, flags);
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
