#include "tkit/preprocessor/narg.hpp"
#include "tkit/preprocessor/utils.hpp"
#include <tuple>

#define __TKIT_ENUMERATE_FIELDS_1(p_ClassName, p_Field) std::make_pair(#p_Field, &p_ClassName::p_Field)
#define __TKIT_ENUMERATE_FIELDS_2(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_1(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_3(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_2(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_4(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_3(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_5(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_4(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_6(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_5(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_7(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_6(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_8(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_7(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_9(p_ClassName, p_Field, ...)                                                           \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_8(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_10(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_9(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_11(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_10(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_12(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_11(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_13(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_12(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_14(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_13(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_15(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_14(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_16(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_15(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_17(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_16(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_18(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_17(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_19(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_18(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_20(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_19(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_21(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_20(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_22(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_21(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_23(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_22(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_24(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_23(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_25(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_24(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_26(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_25(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_27(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_26(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_28(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_27(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_29(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_28(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_30(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_29(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_31(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_30(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_32(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_31(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_33(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_32(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_34(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_33(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_35(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_34(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_36(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_35(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_37(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_36(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_38(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_37(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_39(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_38(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_40(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_39(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_41(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_40(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_42(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_41(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_43(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_42(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_44(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_43(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_45(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_44(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_46(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_45(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_47(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_46(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_48(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_47(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_49(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_48(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_50(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_49(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_51(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_50(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_52(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_51(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_53(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_52(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_54(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_53(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_55(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_54(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_56(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_55(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_57(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_56(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_58(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_57(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_59(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_58(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_60(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_59(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_61(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_60(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_62(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_61(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_63(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_62(p_ClassName, __VA_ARGS__)
#define __TKIT_ENUMERATE_FIELDS_64(p_ClassName, p_Field, ...)                                                          \
    std::make_pair(#p_Field, &p_ClassName::p_Field), __TKIT_ENUMERATE_FIELDS_63(p_ClassName, __VA_ARGS__)

#define __TKIT_ENUMERATE_FIELDS_SELECTOR(p_N) __TKIT_ENUMERATE_FIELDS_##p_N
#define __TKIT_ENUMERATE_FIELDS_DISPATCH(p_N, ...) __TKIT_ENUMERATE_FIELDS_SELECTOR(p_N)(__VA_ARGS__)

#define __TKIT_ENUMERATE_FIELDS(p_ClassName, ...)                                                                      \
    __TKIT_ENUMERATE_FIELDS_DISPATCH(TKIT_NARG(__VA_ARGS__), p_ClassName, __VA_ARGS__)

#define TKIT_ENUMERATE_FIELDS(p_ClassName, p_MethodSuffix, ...)                                                        \
    static constexpr auto GetFields##p_MethodSuffix()                                                                  \
    {                                                                                                                  \
        return std::make_tuple(__TKIT_ENUMERATE_FIELDS(p_ClassName, __VA_ARGS__));                                     \
    }                                                                                                                  \
    template <typename F> void ForEachField##p_MethodSuffix(F &&p_Fun)                                                 \
    {                                                                                                                  \
        constexpr auto fields = GetFields##p_MethodSuffix();                                                           \
        std::apply(                                                                                                    \
            [this, &p_Fun](const auto &...p_Field) {                                                                   \
                (std::forward<F>(p_Fun)(p_Field.first, (*this).*p_Field.second), ...);                                 \
            },                                                                                                         \
            fields);                                                                                                   \
    }