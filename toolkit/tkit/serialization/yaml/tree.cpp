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
    m_Data.Construct<ryml::Tree>(createCallbacks());
}
YamlTree::~YamlTree()
{
    m_Data.Destruct<ryml::Tree>();
}

YamlTree::YamlTree(const YamlTree &other)
{
    m_Data.Construct<ryml::Tree>(*other.get());
}
YamlTree::YamlTree(YamlTree &&other)
{
    m_Data.Construct<ryml::Tree>(std::move(*other.get()));
}

YamlTree &YamlTree::operator=(const YamlTree &other)
{
    if (this != &other)
        *get() = *other.get();
    return *this;
}
YamlTree &YamlTree::operator=(YamlTree &&other)
{
    if (this != &other)
        *get() = std::move(*get());
    return *this;
}

YamlTree YamlTree::FromString(const StringView str)
{
    YamlTree t{};
    ryml::parse_in_arena(toNative(str), t.get());
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
    const ryml::Tree *tree = get();
    const c4::substr res = ryml::emit_yaml(*tree, c4::substr{}, false);
    Str str{};

    if constexpr (std::is_same_v<Str, std::string>)
    {
        str.resize(res.len, 0);
        ryml::emit_yaml(*tree, c4::substr{str.data(), str.size()});
    }
    else
    {
        str.Resize(res.len, 0);
        ryml::emit_yaml(*tree, c4::substr{str.GetData(), str.GetSize()});
    }
    return str;
}

void YamlTree::ToFile(const fs::path &path) const
{
    std::ofstream file{path};

    const StackString str = ToString<StackString>();
    file << str.CString();
}

ConstYamlNode YamlTree::GetRoot() const
{
    return get()->rootref();
}
YamlNode YamlTree::GetRoot()
{
    return get()->rootref();
}

template StackString YamlTree::ToString() const;
template DynamicString YamlTree::ToString() const;
template ArenaString YamlTree::ToString() const;
template TierString YamlTree::ToString() const;
template std::string YamlTree::ToString() const;

} // namespace TKit
