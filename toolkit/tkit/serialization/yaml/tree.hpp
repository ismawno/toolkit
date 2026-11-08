#pragma once

#include "tkit/container/span.hpp"
#include "tkit/serialization/yaml/node.hpp"
#include "tkit/filesystem/filesystem.hpp"

namespace TKit
{
class YamlTree
{
  public:
    YamlTree();
    ~YamlTree();

    YamlTree(const YamlTree &other);
    YamlTree(YamlTree &&other);

    YamlTree &operator=(const YamlTree &other);
    YamlTree &operator=(YamlTree &&other);

    static YamlTree FromString(StringView str);
    TKIT_NO_DISCARD static FileSystemResult<YamlTree> FromFile(const YamlPath &path);

    template <typename Str> Str ToString() const;
    TKIT_NO_DISCARD FileSystemResult<> ToFile(const YamlPath &path) const;

    ConstYamlNode GetRoot() const;
    YamlNode GetRoot();

  private:
    const ryml::Tree &get() const
    {
        return m_Data.Get<ryml::Tree>();
    }
    ryml::Tree &get()
    {
        return m_Data.Get<ryml::Tree>();
    }
    StaticStorage<256> m_Data;
};
} // namespace TKit
