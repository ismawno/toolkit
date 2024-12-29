#pragma once

#include "tkit/core/alias.hpp"
#include "tkit/utilities/dimension.hpp"

// TODO: Introduce intrinsics for SIMD operations
// This file is a way for me to unify my glm includes in all my projects. (Only in case I need it. You may ignore this
// file)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/hash.hpp>
