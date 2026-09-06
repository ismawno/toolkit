#include "tkit/core/pch.hpp"
#include "tkit/serialization/yaml/tree.hpp"
#include "tkit/memory/tier_allocator.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/container/dynamic_array.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/container/span.hpp"
#include <ryml.hpp>
#include <fstream>

#define EXPAND_LOCATION(loc) fromNative(loc.name), loc.line, loc.col, loc.offset

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
static ryml::Callbacks createCallbacks()
{
#ifdef TKIT_ENABLE_ENSURE
    TierAllocator *tier = GetTier();
    ryml::Callbacks cbks;
    cbks.set_user_data(tier);
    cbks.set_allocate(
        [](const size_t size, void *, void *tier) { return scast<TierAllocator *>(tier)->Allocate(size); });
    cbks.set_free([](void *mem, const usz size, void *tier) { scast<TierAllocator *>(tier)->Deallocate(mem, size); });
    cbks.set_error_basic([](const c4::csubstr msg, const ryml::ErrorDataBasic &data, void *) {
        TKIT_PANIC("[TOOLKIT][YAML][BASIC] Message: {} - Name: {}, Line: {}, Col: {}, Offset: {}", fromNative(msg),
                   EXPAND_LOCATION(data.location));
    });
    cbks.set_error_parse([](const c4::csubstr msg, const ryml::ErrorDataParse &data, void *) {
        TKIT_PANIC("[TOOLKIT][YAML][PARSE] Message: {} - cpp - Name: {}, Line: {}, Col: {}, Offset: {} - yaml - Name: "
                   "{}, Line: {}, "
                   "Col: {}, Offset: {}",
                   fromNative(msg), EXPAND_LOCATION(data.cpploc), EXPAND_LOCATION(data.ymlloc));
    });
    cbks.set_error_visit([](const c4::csubstr msg, const ryml::ErrorDataVisit &data, void *) {
        TKIT_PANIC("[TOOLKIT][YAML][VISIT] Message: {} - Tree: {}, Node: {} - Name: {}, Line: {}, Col: {}, Offset: {}",
                   fromNative(msg), FormatPointer(data.tree), data.node, EXPAND_LOCATION(data.cpploc));
    });

    return cbks;
#else
    return ryml::get_callbacks();
#endif
}
YamlTree::YamlTree()
{
    TierAllocator *tier = GetTier();
    m_Tree = tier->Create<ryml::Tree>(createCallbacks());
}
YamlTree::~YamlTree()
{
    TierAllocator *tier = GetTier();
    tier->Destroy(m_Tree);
}

YamlTree YamlTree::FromString(const StringView str)
{
    YamlTree t{};
    ryml::parse_in_arena(toNative(str), t.m_Tree);
    return t;
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
    const c4::substr res = ryml::emit_yaml(*m_Tree, c4::substr{}, false);
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
    return str;
}

void YamlTree::ToFile(const fs::path &path) const
{
    std::ofstream file{path};

    const StackString str = ToString<StackString>();
    file << str.CString();
}

YamlNode YamlTree::GetRoot() const
{
    return {m_Tree, m_Tree->root_id()};
}

template StackString YamlTree::ToString() const;
template DynamicString YamlTree::ToString() const;
template ArenaString YamlTree::ToString() const;
template TierString YamlTree::ToString() const;
template std::string YamlTree::ToString() const;

} // namespace TKit
