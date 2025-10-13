#include "tests/simd/wide.hpp"
#include "tkit/simd/wide_neon.hpp"

using namespace TKit;
using namespace TKit::Simd;

TEST_CASE("SIMD: NEON Wide f32", "[SIMD][NEON][Wide][f32]")
{
    RunWideTests<NEON::Wide<f32>>();
}
// #ifdef TKIT_AARCH64
// TEST_CASE("SIMD: NEON Wide f64", "[SIMD][NEON][Wide][f64]")
// {
//     RunWideTests<NEON::Wide<f64>>();
// }
// #endif
// TEST_CASE("SIMD: NEON Wide u8", "[SIMD][NEON][Wide][u8]")
// {
//     RunWideTests<NEON::Wide<u8>>();
// }
// TEST_CASE("SIMD: NEON Wide u16", "[SIMD][NEON][Wide][u16]")
// {
//     RunWideTests<NEON::Wide<u16>>();
// }
// TEST_CASE("SIMD: NEON Wide u32", "[SIMD][NEON][Wide][u32]")
// {
//     RunWideTests<NEON::Wide<u32>>();
// }
// TEST_CASE("SIMD: NEON Wide u64", "[SIMD][NEON][Wide][u64]")
// {
//     RunWideTests<NEON::Wide<u64>>();
// }
//
// TEST_CASE("SIMD: NEON Wide i8", "[SIMD][NEON][Wide][i8]")
// {
//     RunWideTests<NEON::Wide<i8>>();
// }
// TEST_CASE("SIMD: NEON Wide i16", "[SIMD][NEON][Wide][i16]")
// {
//     RunWideTests<NEON::Wide<i16>>();
// }
// TEST_CASE("SIMD: NEON Wide i32", "[SIMD][NEON][Wide][i32]")
// {
//     RunWideTests<NEON::Wide<i32>>();
// }
// TEST_CASE("SIMD: NEON Wide i64", "[SIMD][NEON][Wide][i64]")
// {
//     RunWideTests<NEON::Wide<i64>>();
// }
