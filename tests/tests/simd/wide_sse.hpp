#include "tests/simd/wide.hpp"
#include "tkit/simd/wide_sse.hpp"

using namespace TKit;
using namespace TKit::Detail;

TEST_CASE("SIMD: SSE Wide f32", "[SIMD][SSE][Wide][f32]")
{
    RunWideTests<SSE::Wide<f32>>();
}
TEST_CASE("SIMD: SSE Wide f64", "[SIMD][SSE][Wide][f64]")
{
    RunWideTests<SSE::Wide<f64>>();
}
#ifdef TKIT_SIMD_AVX2
TEST_CASE("SIMD: SSE Wide u8", "[SIMD][SSE][Wide][u8]")
{
    RunWideTests<SSE::Wide<u8>>();
}
TEST_CASE("SIMD: SSE Wide u16", "[SIMD][SSE][Wide][u16]")
{
    RunWideTests<SSE::Wide<u16>>();
}
TEST_CASE("SIMD: SSE Wide u32", "[SIMD][SSE][Wide][u32]")
{
    RunWideTests<SSE::Wide<u32>>();
}
TEST_CASE("SIMD: SSE Wide u64", "[SIMD][SSE][Wide][u64]")
{
    RunWideTests<SSE::Wide<u64>>();
}

TEST_CASE("SIMD: SSE Wide i8", "[SIMD][SSE][Wide][i8]")
{
    RunWideTests<SSE::Wide<i8>>();
}
TEST_CASE("SIMD: SSE Wide i16", "[SIMD][SSE][Wide][i16]")
{
    RunWideTests<SSE::Wide<i16>>();
}
TEST_CASE("SIMD: SSE Wide i32", "[SIMD][SSE][Wide][i32]")
{
    RunWideTests<SSE::Wide<i32>>();
}
TEST_CASE("SIMD: SSE Wide i64", "[SIMD][SSE][Wide][i64]")
{
    RunWideTests<SSE::Wide<i64>>();
}
#endif
