#include "tkit/preprocessor/system.hpp"
#ifdef TKIT_OS_WINDOWS
#    define NOMINMAX
#    include <windows.h>
#    undef TRANSPARENT
#endif
