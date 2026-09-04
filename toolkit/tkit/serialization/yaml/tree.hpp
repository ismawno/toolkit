#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error                                                                                                             \
        "[TOOLKIT][YAML] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/container/span.hpp"
#include "tkit/serialization/yaml/node.hpp"
#include "tkit/utils/non_copyable.hpp"
#include <filesystem>

namespace TKit
{
namespace fs = std::filesystem;

class YamlTree
{
    TKIT_NON_COPYABLE(YamlTree)
  public:
    YamlTree();
    YamlTree(c4::yml::Tree *tree) : m_Tree(tree)
    {
    }

    ~YamlTree();

    static YamlTree FromString(StringView str);
    static YamlTree FromFile(const fs::path &path);

    template <typename Str> Str ToString() const;
    void ToFile(const fs::path &path) const;

    YamlNode GetRoot() const;

  private:
    ryml::Tree *m_Tree;
};
} // namespace TKit
