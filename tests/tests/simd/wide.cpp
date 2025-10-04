#include "tests/simd/wide.hpp"
#include "tkit/simd/wide.hpp"

using namespace TKit;
using namespace TKit::Detail;

TEST_CASE("SIMD: Base Wide", "[SIMD][Base][Wide]")
{
    RunWideTests<Wide<u8, 32>>();
    RunWideTests<Wide<u16, 16>>();
    RunWideTests<Wide<u32, 8>>();
    RunWideTests<Wide<u64, 4>>();
}
