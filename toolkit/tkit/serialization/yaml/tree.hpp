#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error                                                                                                             \
        "[TOOLKIT][YAML] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/container/span.hpp"
#include "tkit/serialization/yaml/node.hpp"
#include <filesystem>

namespace TKit
{
namespace fs = std::filesystem;

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
    static YamlTree FromFile(const fs::path &path);

    template <typename Str> Str ToString() const;
    void ToFile(const fs::path &path) const;

    ConstYamlNode GetRoot() const;
    YamlNode GetRoot();

  private:
    const ryml::Tree *get() const
    {
        return m_Data.Get<ryml::Tree>();
    }
    ryml::Tree *get()
    {
        return m_Data.Get<ryml::Tree>();
    }
    RawStorage<256> m_Data;
};
} // namespace TKit
