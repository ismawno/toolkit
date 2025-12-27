#pragma once

#include "tkit/utils/result.hpp"

namespace TKit
{
template <typename T> using Optional = Result<T, void>;
}
