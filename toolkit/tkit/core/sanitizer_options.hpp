#include "tkit/preprocessor/system.hpp"

#define TKIT_SANITIZER_HOOK extern "C" __attribute__((visibility("default")))

// AddressSanitizer runtime flags.
#if defined(TKIT_ASAN_ENABLED) && defined(TKIT_ASAN_OPTIONS)
TKIT_SANITIZER_HOOK const char *__asan_default_options()
{
    return TKIT_ASAN_OPTIONS;
}
#endif

// LeakSanitizer flags. Read separately from ASan's, even when LSan runs
// as part of the ASan runtime.
#if defined(TKIT_ASAN_ENABLED) && defined(TKIT_LSAN_OPTIONS)
TKIT_SANITIZER_HOOK const char *__lsan_default_options()
{
    return TKIT_LSAN_OPTIONS;
}
#endif

#if defined(TKIT_ASAN_ENABLED) && defined(TKIT_LSAN_SUPPRESSIONS)
TKIT_SANITIZER_HOOK const char *__lsan_default_suppressions()
{
    return TKIT_LSAN_SUPPRESSIONS;
}
#endif

#if defined(TKIT_UBSAN_ENABLED) && defined(TKIT_UBSAN_OPTIONS)
TKIT_SANITIZER_HOOK const char *__ubsan_default_options()
{
    return TKIT_UBSAN_OPTIONS;
}
#endif

#if defined(TKIT_TSAN_ENABLED) && defined(TKIT_TSAN_OPTIONS)
TKIT_SANITIZER_HOOK const char *__tsan_default_options()
{
    return TKIT_TSAN_OPTIONS;
}
#endif

#if defined(TKIT_TSAN_ENABLED) && defined(TKIT_TSAN_SUPPRESSIONS)
TKIT_SANITIZER_HOOK const char *__tsan_default_suppressions()
{
    return TKIT_TSAN_SUPPRESSIONS;
}
#endif

#if defined(TKIT_MSAN_ENABLED) && defined(TKIT_MSAN_OPTIONS)
TKIT_SANITIZER_HOOK const char *__msan_default_options()
{
    return TKIT_MSAN_OPTIONS;
}
#endif
