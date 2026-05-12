#pragma once

#ifdef TKIT_DEFAULT_STRING_ARENA
#    include "tkit/container/arena_array.hpp"
#elif defined(TKIT_DEFAULT_STRING_STACK)
#    include "tkit/container/stack_array.hpp"
#elif defined(TKIT_DEFAULT_STRING_STATIC)
#    ifndef TKIT_DEFAULT_STRING_CAPACITY
#        define TKIT_DEFAULT_STRING_CAPACITY 32
#    endif
#    include "tkit/container/static_array.hpp"
#elif defined(TKIT_DEFAULT_STRING_DYNAMIC)
#    include "tkit/container/dynamic_array.hpp"
#elif defined(TKIT_DEFAULT_STRING_TIER)
#    include "tkit/container/tier_array.hpp"
#endif

namespace TKit
{
#ifdef TKIT_DEFAULT_STRING_ARENA
using String = ArenaString;
#elif defined(TKIT_DEFAULT_STRING_STACK)
using String = StackString;
#elif defined(TKIT_DEFAULT_STRING_STATIC)
using String = StaticString<TKIT_DEFAULT_STRING_CAPACITY>;
#elif defined(TKIT_DEFAULT_STRING_DYNAMIC)
using String = DynamicString;
#elif defined(TKIT_DEFAULT_STRING_TIER)
using String = TierString;
#endif
} // namespace TKit
