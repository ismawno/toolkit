#include "perf/settings.hpp"

namespace TKit
{
template <typename MTX> void RecordThreadPoolSum(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads);
void RecordParallelSum(const ThreadPoolSumSettings &p_Settings, usize p_Maxthreads);
} // namespace TKit