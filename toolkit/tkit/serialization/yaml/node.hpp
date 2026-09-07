#pragma once

#ifndef TKIT_ENABLE_YAML_SERIALIZATION
#    error                                                                                                             \
        "[TOOLKIT][YAML] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_YAML_SERIALIZATION"
#endif

#include "tkit/container/span.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/utils/result.hpp"

namespace c4::yml
{
class Tree;
}

namespace TKit
{
template <typename T> struct YamlCodec;

namespace ryml = c4::yml;
using YamlNodeFlags = u32;
enum YamlNodeFlagBit : YamlNodeFlags
{
    // Type flags
    YamlNodeFlag_NoType = 0,
    YamlNodeFlag_Key = 1u << 0,
    YamlNodeFlag_Value = 1u << 1,
    YamlNodeFlag_Map = 1u << 2,
    YamlNodeFlag_Sequence = 1u << 3,

    // Type combinations
    YamlNodeFlag_KeyValue = YamlNodeFlag_Key | YamlNodeFlag_Value,
    YamlNodeFlag_KeySequence = YamlNodeFlag_Key | YamlNodeFlag_Sequence,
    YamlNodeFlag_KeyMap = YamlNodeFlag_Key | YamlNodeFlag_Map,

    // Container style flags
    YamlNodeFlag_FlowSingleLine = 1u << 16,
    YamlNodeFlag_FlowMultiLine1 = 1u << 17,
    YamlNodeFlag_FlowMultiLineN = 1u << 18,
    YamlNodeFlag_FlowSpaced = 1u << 19,
    YamlNodeFlag_Block = 1u << 20,

    // Scalar style flags
    YamlNodeFlag_KeyLiteral = 1u << 21,
    YamlNodeFlag_ValLiteral = 1u << 22,
    YamlNodeFlag_KeyFolded = 1u << 23,
    YamlNodeFlag_ValFolded = 1u << 24,
    YamlNodeFlag_KeySingleQuote = 1u << 25,
    YamlNodeFlag_ValSingleQuote = 1u << 26,
    YamlNodeFlag_KeyDoubleQuote = 1u << 27,
    YamlNodeFlag_ValDoubleQuote = 1u << 28,
    YamlNodeFlag_KeyPlain = 1u << 29,
    YamlNodeFlag_ValPlain = 1u << 30,

    // Style combination masks
    YamlNodeFlag_FlowMultiLine = YamlNodeFlag_FlowMultiLine1 | YamlNodeFlag_FlowMultiLineN,
    YamlNodeFlag_ContainerFlow = YamlNodeFlag_FlowSingleLine | YamlNodeFlag_FlowMultiLine | YamlNodeFlag_FlowSpaced,
    YamlNodeFlag_ContainerBlock = YamlNodeFlag_Block,
    YamlNodeFlag_ContainerStyle = YamlNodeFlag_ContainerFlow | YamlNodeFlag_ContainerBlock,
    YamlNodeFlag_ScalarStyle = YamlNodeFlag_KeyLiteral | YamlNodeFlag_ValLiteral | YamlNodeFlag_KeyFolded |
                               YamlNodeFlag_ValFolded | YamlNodeFlag_KeySingleQuote | YamlNodeFlag_ValSingleQuote |
                               YamlNodeFlag_KeyDoubleQuote | YamlNodeFlag_ValDoubleQuote | YamlNodeFlag_KeyPlain |
                               YamlNodeFlag_ValPlain,
    YamlNodeFlag_Style = YamlNodeFlag_ScalarStyle | YamlNodeFlag_ContainerStyle,
};

struct YamlReadError
{
    u32 NodeId;
    TierString Message;
};

using YamlReadResult = Result<void, YamlReadError>;
} // namespace TKit

#ifdef TKIT_ENABLE_ENSURE

namespace TKit::Detail
{
void CheckYamlReadResult(const YamlReadResult &res);
} // namespace TKit::Detail

#    define TKIT_CHECK_YAML_RESULT(expr) TKit::Detail::CheckYamlReadResult(expr)
#else
#    define TKIT_CHECK_YAML_RESULT(expr) expr
#endif

namespace TKit
{
constexpr u32 NullYamlNodeId = TKIT_U32_MAX;
class YamlNode
{
  public:
    YamlNode(ryml::Tree *tree, const u32 id) : m_Tree(tree), m_Id(id)
    {
    }

    YamlNode ByKey(StringView key) const;
    YamlNode ByKey(u32 idx) const;

    YamlNode ByKey(StringView key);
    YamlNode ByKey(u32 idx);

    YamlNode operator[](const StringView key) const
    {
        return ByKey(key);
    }
    YamlNode operator[](u32 idx) const;

    YamlNode operator[](const StringView key)
    {
        return ByKey(key);
    }
    YamlNode operator[](u32 idx);

    YamlNode Append();
    template <typename T> YamlNode Append(const T &val)
    {
        YamlNode node = Append();
        node.Write(val);
        return node;
    }

    YamlNode GetParent() const;

    StringView GetKey() const;
    StringView GetValue() const;

    u32 GetChildCount() const;
    u32 GetId() const
    {
        return m_Id;
    }

    YamlNode FirstChild() const;
    YamlNode NextSibling() const;

    void SetValue(StringView val);
    void SetKey(StringView key);

    YamlNodeFlags GetFlags() const;
    void SetFlags(YamlNodeFlags flags);
    void AddFlags(const YamlNodeFlags flags)
    {
        SetFlags(GetFlags() | flags);
    }
    void RemoveFlags(const YamlNodeFlags flags)
    {
        SetFlags(GetFlags() & ~flags);
    }

    YamlNodeFlags GetKeyFlags() const;
    void SetKeyFlags(YamlNodeFlags flags);
    void AddKeyFlags(const YamlNodeFlags flags)
    {
        SetKeyFlags(GetKeyFlags() | flags);
    }
    void RemoveKeyFlags(const YamlNodeFlags flags)
    {
        SetKeyFlags(GetKeyFlags() & ~flags);
    }

    YamlNode &operator|=(const YamlNodeFlags flags)
    {
        AddFlags(flags);
        return *this;
    }
    YamlNode &operator&=(const YamlNodeFlags flags)
    {
        SetFlags(GetFlags() & flags);
        return *this;
    }

    template <typename T> YamlReadResult TryRead(T &val) const
    {
        return YamlCodec<T>::Decode(*this, val);
    }

    template <typename T> void Read(T &val) const
    {
        TKIT_CHECK_YAML_RESULT(TryRead(val));
    }
    template <typename T, typename... Args> T Read(Args &&...args) const
    {
        T val{std::forward<Args>(args)...};
        Read(val);
        return val;
    }
    template <typename T> void Write(const T &val)
    {
        YamlCodec<T>::Encode(*this, val);
    }

    template <typename T> YamlReadResult TryReadKey(T &key) const;

    template <typename T> void ReadKey(T &key) const
    {
        TKIT_CHECK_YAML_RESULT(TryReadKey(key));
    }
    template <typename T, typename... Args> T ReadKey(Args &&...args) const
    {
        T key{std::forward<Args>(args)...};
        ReadKey(key);
        return key;
    }
    template <typename T> void WriteKey(const T &key);

    template <typename T> YamlNode &operator<<(const T &val)
    {
        Write(val);
        return *this;
    }
    template <typename T> YamlNode &operator>>(T &val)
    {
        Read(val);
        return *this;
    }

    bool operator==(const YamlNode &node) const
    {
        return node.m_Id == m_Id && node.m_Tree == m_Tree;
    }
    bool operator!=(const YamlNode &node) const
    {
        return node.m_Id != m_Id || node.m_Tree != m_Tree;
    }

    YamlNode &operator++()
    {
        *this = NextSibling();
        return *this;
    }
    YamlNode operator++(int)
    {
        const YamlNode cpy = *this;
        ++(*this);
        return cpy;
    }

    const YamlNode &operator*()
    {
        return *this;
    }

    YamlNode begin() const
    {
        return FirstChild();
    }
    YamlNode end() const
    {
        return {m_Tree, NullYamlNodeId};
    }

    explicit operator bool() const
    {
        return m_Id != NullYamlNodeId;
    }

    bool IsMap() const
    {
        return GetFlags() & YamlNodeFlag_Map;
    }
    bool IsSequence() const
    {
        return GetFlags() & YamlNodeFlag_Sequence;
    }
    bool IsContainer() const
    {
        return GetFlags() & (YamlNodeFlag_Map | YamlNodeFlag_Sequence);
    }
    bool IsScalar() const
    {
        return (GetFlags() & YamlNodeFlag_Value) && !IsContainer();
    }
    bool HasKey() const
    {
        return GetFlags() & YamlNodeFlag_Key;
    }
    bool HasValue() const
    {
        return GetFlags() & YamlNodeFlag_Value;
    }
    bool IsKeyValue() const
    {
        return (GetFlags() & YamlNodeFlag_KeyValue) == YamlNodeFlag_KeyValue;
    }

  private:
    ryml::Tree *m_Tree;
    u32 m_Id;

    friend struct YamlCodec<u8>;
    friend struct YamlCodec<u16>;
    friend struct YamlCodec<u32>;
    friend struct YamlCodec<u64>;

    friend struct YamlCodec<i8>;
    friend struct YamlCodec<i16>;
    friend struct YamlCodec<i32>;
    friend struct YamlCodec<i64>;

    friend struct YamlCodec<f32>;
    friend struct YamlCodec<f64>;

    friend struct YamlCodec<bool>;

    friend struct YamlCodec<char *>;
    friend struct YamlCodec<std::string>;
    friend struct YamlCodec<std::string_view>;
};
} // namespace TKit
