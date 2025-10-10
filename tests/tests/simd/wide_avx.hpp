#define TKIT_ALLOW_SCALAR_SIMD_FALLBACKS
#include "tests/simd/wide.hpp"
#include "tkit/simd/wide_avx.hpp"

using namespace TKit;
using namespace TKit::Detail;

TEST_CASE("SIMD: AVX Wide f32", "[SIMD][AVX][Wide][f32]")
{
    RunWideTests<AVX::Wide<f32>>();
}
TEST_CASE("SIMD: AVX Wide f64", "[SIMD][AVX][Wide][f64]")
{
    RunWideTests<AVX::Wide<f64>>();
}
#ifdef TKIT_SIMD_AVX2
TEST_CASE("SIMD: AVX Wide u8", "[SIMD][AVX][Wide][u8]")
{
    RunWideTests<AVX::Wide<u8>>();
}
TEST_CASE("SIMD: AVX Wide u16", "[SIMD][AVX][Wide][u16]")
{
    RunWideTests<AVX::Wide<u16>>();
}
TEST_CASE("SIMD: AVX Wide u32", "[SIMD][AVX][Wide][u32]")
{
    RunWideTests<AVX::Wide<u32>>();
}
TEST_CASE("SIMD: AVX Wide u64", "[SIMD][AVX][Wide][u64]")
{
    RunWideTests<AVX::Wide<u64>>();
}

TEST_CASE("SIMD: AVX Wide i8", "[SIMD][AVX][Wide][i8]")
{
    RunWideTests<AVX::Wide<i8>>();
}
TEST_CASE("SIMD: AVX Wide i16", "[SIMD][AVX][Wide][i16]")
{
    RunWideTests<AVX::Wide<i16>>();
}
TEST_CASE("SIMD: AVX Wide i32", "[SIMD][AVX][Wide][i32]")
{
    RunWideTests<AVX::Wide<i32>>();
}
TEST_CASE("SIMD: AVX Wide i64", "[SIMD][AVX][Wide][i64]")
{
    RunWideTests<AVX::Wide<i64>>();
}
#endif
