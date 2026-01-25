#define TKIT_UNUSED(arg) (void)(arg)
#define TKIT_IDENTITY(...) __VA_ARGS__
#define TKIT_CONCAT(a, b) a##b
#define TKIT_STRINGIFY(...) #__VA_ARGS__

#define TKIT_UNWRAP(...) __VA_ARGS__
#define TKIT_WRAP(...) (__VA_ARGS__)

#define __TKIT_IF_ELSE_0(true, false) false
#define __TKIT_IF_ELSE_1(true, false) true

#define TKIT_IF_ELSE(condition) TKIT_CONCAT(__TKIT_IF_ELSE_, condition)
#define TKIT_FIRST_ARG(first, ...) first