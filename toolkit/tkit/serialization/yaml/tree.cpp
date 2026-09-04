#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/tree.hpp"
#include "tkit/memory/tier_allocator.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/tier_array.hpp"
#include <ryml.hpp>
#include <fstream>

namespace TKit
{
YamlTree::YamlTree()
{
    TierAllocator *tier = GetTier();
    m_Tree = tier->Create<ryml::Tree>();
}
YamlTree::~YamlTree()
{
    TierAllocator *tier = GetTier();
    tier->Destroy(m_Tree);
}

YamlTree YamlTree::FromString(const StringView str)
{
    TierAllocator *tier = GetTier();
    return YamlTree{tier->Create<ryml::Tree>(ryml::parse_in_arena(str.GetData()))};
}

YamlTree YamlTree::FromFile(const fs::path &path)
{
    std::ifstream file{path, std::ios::ate};
    StackString contents{};
    contents.Resize(file.tellg(), 0);

    file.seekg(0);
    file.read(contents.GetData(), contents.GetSize());

    return FromString(contents);
}

template <typename Str> Str YamlTree::ToString() const
{
    const c4::substr res = ryml::emit_yaml(*m_Tree, c4::substr{});
    Str str{};

    if constexpr (std::is_same_v<Str, std::string>)
    {
        str.resize(res.len, 0);
        ryml::emit_yaml(*m_Tree, c4::substr{str.data(), str.size()});
    }
    else
    {
        str.Resize(res.len, 0);
        ryml::emit_yaml(*m_Tree, c4::substr{str.GetData(), str.GetSize()});
    }
    ryml::NodeRef ref;
    ref.key();
    return str;
}

void YamlTree::ToFile(const fs::path &path) const
{
    std::ofstream file{path};

    const StackString str = ToString<StackString>();
    file << str.CString();
}

template StackString YamlTree::ToString() const;
template DynamicString YamlTree::ToString() const;
template ArenaString YamlTree::ToString() const;
template TierString YamlTree::ToString() const;
template std::string YamlTree::ToString() const;

} // namespace TKit
