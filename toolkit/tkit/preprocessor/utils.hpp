#define TKIT_UNUSED(p_Arg) (void)(p_Arg)
#define TKIT_IDENTITY(...) __VA_ARGS__
#define TKIT_CONCAT(p_A, p_B) p_A##p_B
#define TKIT_STRINGIFY(...) #__VA_ARGS__

#define TKIT_UNWRAP(...) __VA_ARGS__
#define TKIT_WRAP(...) (__VA_ARGS__)

#define __TKIT_IF_ELSE_0(p_True, p_False) p_False
#define __TKIT_IF_ELSE_1(p_True, p_False) p_True

#define TKIT_IF_ELSE(p_Condition) TKIT_CONCAT(__TKIT_IF_ELSE_, p_Condition)
#define TKIT_FIRST_ARG(p_First, ...) p_First