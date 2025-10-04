#include "tests/simd/wide.hpp"
#include "tkit/simd/wide.hpp"

using namespace TKit;
using namespace TKit::Detail;

TEST_CASE("SIMD: Base Wide u8", "[SIMD][Base][Wide][u8]")
{
    RunWideTests<Wide<u8, 32>>();
}
TEST_CASE("SIMD: Base Wide u16", "[SIMD][Base][Wide][u16]")
{
    RunWideTests<Wide<u16, 16>>();
}
TEST_CASE("SIMD: Base Wide u32", "[SIMD][Base][Wide][u32]")
{
    RunWideTests<Wide<u32, 8>>();
}
TEST_CASE("SIMD: Base Wide u64", "[SIMD][Base][Wide][u64]")
{
    RunWideTests<Wide<u64, 4>>();
}

TEST_CASE("SIMD: Base Wide i8", "[SIMD][Base][Wide][i8]")
{
    RunWideTests<Wide<i8, 32>>();
}
TEST_CASE("SIMD: Base Wide i16", "[SIMD][Base][Wide][i16]")
{
    RunWideTests<Wide<i16, 16>>();
}
TEST_CASE("SIMD: Base Wide i32", "[SIMD][Base][Wide][i32]")
{
    RunWideTests<Wide<i32, 8>>();
}
TEST_CASE("SIMD: Base Wide i64", "[SIMD][Base][Wide][i64]")
{
    RunWideTests<Wide<i64, 4>>();
}
