#pragma once

#include "kit/core/alias.hpp"
#include "kit/utilities/dimension.hpp"

// TODO: Introduce intrinsics for SIMD operations

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/hash.hpp>

// This file includes all necessary glm headers and defines some useful aliases. Toolkit istelf cannot use it directly.
// Its just a utility header to have a glogab glm configuration.

namespace TKit::GLM
{
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

template <Dimension D> using vec = glm::vec<D, f32>;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

template <Dimension D> using mat = glm::mat<D + 1, D + 1, f32>;

using quat = glm::quat;

template <Dimension D> struct RotType;

template <> struct RotType<D2>
{
    using Type = f32;
    static constexpr f32 Identity = 0.f;
};
template <> struct RotType<D3>
{
    using Type = quat;
    static inline const quat Identity = quat{1.f, 0.f, 0.f, 0.f};
};

template <Dimension D> using rot = typename RotType<D>::Type;

} // namespace TKit::GLM