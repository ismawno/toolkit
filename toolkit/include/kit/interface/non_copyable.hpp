#pragma once

#define KIT_NON_COPYABLE(p_ClassName)                                                                                  \
    p_ClassName(const p_ClassName &) = delete;                                                                         \
    p_ClassName &operator=(const p_ClassName &) = delete;
