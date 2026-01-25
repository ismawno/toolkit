#include "tkit/core/pch.hpp"
#include "tkit/utils/debug.hpp"

namespace TKit::Detail
{
void LogAndBreak(const char *level, const char *color, const char *file, const i32 line)
{
    Log("[TOOLKIT] Assertion failed!", level, color, file, line);
    TKIT_DEBUG_BREAK();
}
void CheckOutOfBounds(const char *level, const char *color, const char *file, const i32 line,
                      const usize index, const usize size, const std::string_view head)
{
    if (size > 0)
        Log("{}Out of bounds error. Trying to access a container with an illegal index ({} >= {}). Index must be "
            "smaller than size",
            level, color, file, line, head, index, size);
    else
        Log("{}Out of bounds error. Trying to index into an empty container with an index value of {}. Container "
            "must not be indexed until it has elements",
            level, color, file, line, head, index, size);
    TKIT_DEBUG_BREAK();
}
} // namespace TKit::Detail
