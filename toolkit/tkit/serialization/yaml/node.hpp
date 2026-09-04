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
template <typename T> struct Codec;

namespace ryml = c4::yml;
using YamlNodeFlags = u8;
enum YamlNodeFlagBit : YamlNodeFlags
{
    YamlNodeFlag_NoType = 0,
    YamlNodeFlag_Key = 1u << 0,
    YamlNodeFlag_Value = 1u << 1,
    YamlNodeFlag_Map = 1u << 2,
    YamlNodeFlag_Sequence = 1u << 3,
    YamlNodeFlag_KeyValue = YamlNodeFlag_Key | YamlNodeFlag_Value,
    YamlNodeFlag_KeySequence = YamlNodeFlag_Key | YamlNodeFlag_Sequence,
    YamlNodeFlag_KeyMap = YamlNodeFlag_Key | YamlNodeFlag_Map,
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
class YamlNode
{
  public:
    YamlNode(c4::yml::Tree *tree, const u32 id) : m_Tree(tree), m_Id(id)
    {
    }

    YamlNode operator[](StringView str) const;
    YamlNode operator[](u32 idx) const;

    YamlNode operator[](StringView str);
    YamlNode operator[](u32 idx);

    YamlNode Append();
    template <typename T> YamlNode Append(const T &val)
    {
        YamlNode node = Append();
        node.Write(val);
        return node;
    }

    StringView GetKey() const;
    StringView GetValue() const;

    u32 GetChildCount() const;
    u32 GetId() const
    {
        return m_Id;
    }

    void SetValue(StringView str);

    YamlNodeFlags GetFlags() const;
    void SetKeyFlags(YamlNodeFlags flags);
    void SetValueFlags(YamlNodeFlags flags);

    template <typename T> YamlReadResult TryRead(T &val) const
    {
        return Codec<T>::Decode(*this, val);
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
        *this = Codec<T>::Encode(val);
    }

    template <typename T> YamlNode &operator<<(const T &val)
    {
        Write(val);
    }
    template <typename T> YamlNode &operator>>(T &val)
    {
        Read(val);
    }

  private:
    c4::yml::Tree *m_Tree;
    u32 m_Id;

    friend struct Codec<u8>;
    friend struct Codec<u16>;
    friend struct Codec<u32>;
    friend struct Codec<u64>;

    friend struct Codec<i8>;
    friend struct Codec<i16>;
    friend struct Codec<i32>;
    friend struct Codec<i64>;
};
} // namespace TKit
