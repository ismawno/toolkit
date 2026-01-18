#pragma once

// i hate windows and i hate msvc
#ifdef TKIT_OS_WINDOWS
#    undef min
#    undef max
#endif

#include "tkit/utils/alias.hpp"
#include <limits>
#include <math.h>

#define TKIT_F32_MIN TKit::Limits<f32>::Min()
#define TKIT_F64_MIN TKit::Limits<f64>::Min()

#define TKIT_U8_MIN TKit::Limits<u8>::Min()
#define TKIT_U16_MIN TKit::Limits<u16>::Min()
#define TKIT_U32_MIN TKit::Limits<u32>::Min()
#define TKIT_U64_MIN TKit::Limits<u64>::Min()
#define TKIT_USIZE_MIN TKit::Limits<usize>::Min()

#define TKIT_I8_MIN TKit::Limits<i8>::Min()
#define TKIT_I16_MIN TKit::Limits<i16>::Min()
#define TKIT_I32_MIN TKit::Limits<i32>::Min()
#define TKIT_I64_MIN TKit::Limits<i64>::Min()
#define TKIT_SSIZE_MIN TKit::Limits<ssize>::Min()

#define TKIT_F32_MAX TKit::Limits<f32>::Max()
#define TKIT_F64_MAX TKit::Limits<f64>::Max()

#define TKIT_U8_MAX TKit::Limits<u8>::Max()
#define TKIT_U16_MAX TKit::Limits<u16>::Max()
#define TKIT_U32_MAX TKit::Limits<u32>::Max()
#define TKIT_U64_MAX TKit::Limits<u64>::Max()
#define TKIT_USIZE_MAX TKit::Limits<usize>::Max()

#define TKIT_I8_MAX TKit::Limits<i8>::Max()
#define TKIT_I16_MAX TKit::Limits<i16>::Max()
#define TKIT_I32_MAX TKit::Limits<i32>::Max()
#define TKIT_I64_MAX TKit::Limits<i64>::Max()
#define TKIT_SSIZE_MAX TKit::Limits<ssize>::Max()

#define TKIT_F32_EPSILON TKit::Limits<f32>::Epsilon()
#define TKIT_F64_EPSILON TKit::Limits<f64>::Epsilon()

namespace TKit
{
template <typename T> struct Limits
{
    static constexpr T Min()
    {
        return std::numeric_limits<T>::min();
    }
    static constexpr T Max()
    {
        return std::numeric_limits<T>::max();
    }
    static constexpr T Epsilon()
    {
        return std::numeric_limits<T>::epsilon();
    }
};

#ifndef TKIT_MEMORY_MAX_STACK_ALLOCATION
#    define TKIT_MEMORY_MAX_STACK_ALLOCATION 1024
#endif

#ifndef TKIT_MAX_THREADS
#    define TKIT_MAX_THREADS 16
#endif

#ifndef TKIT_MAX_ALLOCATOR_PUSH_DEPTH
#    define TKIT_MAX_ALLOCATOR_PUSH_DEPTH 4
#endif

constexpr usize MaxStackAlloc = TKIT_MEMORY_MAX_STACK_ALLOCATION;
constexpr usize MaxThreads = TKIT_MAX_THREADS;
constexpr usize MaxAllocatorPushDepth = TKIT_MAX_ALLOCATOR_PUSH_DEPTH;

template <typename T> constexpr bool ApproachesZero(const T p_Value)
{
    if constexpr (Float<T>)
        return std::abs(p_Value) <= Limits<T>::Epsilon();
    else
        return p_Value == static_cast<T>(0);
}

template <typename T> constexpr bool Approximately(const T p_Left, const T p_Right)
{
    return ApproachesZero(p_Left - p_Right);
}
} // namespace TKit
