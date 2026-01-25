#pragma once

// For a non copyable interface, I could just implement a non-virtual base class and delete the constructor (and thats
// how I did it in cpp-kit). I have decided to use a macro instead because it fits better with the new/delete override
// macros for example. Also, adding this macro to a base and derived class wont cause issues

#define TKIT_NON_COPYABLE(className)                                                                                   \
    className(const className &) = delete;                                                                             \
    className &operator=(const className &) = delete;
