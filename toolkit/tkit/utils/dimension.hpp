#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/reflection/reflect.hpp"
#include "tkit/serialization/yaml/serialize.hpp"

// This utility is used is here because I usually create projects where 2D and 3D entities are involved, and it is handy
// to have a single place to define the dimension of the entities. It also lets me to define a 2D and a 3D API.

namespace TKit
{
TKIT_REFLECT_DECLARE_ENUM(Dimension)
TKIT_YAML_SERIALIZE_DECLARE_ENUM(Dimension)

enum Dimension : u8
{
    D2 = 2,
    D3 = 3
};
} // namespace TKit
