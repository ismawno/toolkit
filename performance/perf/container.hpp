#pragma once

#include "perf/settings.hpp"

namespace TKit::Perf
{
void RecordVector(const ContainerSettings &p_Settings);
void RecordStaticArray(const ContainerSettings &p_Settings);
void RecordDynamicArray(const ContainerSettings &p_Settings);
} // namespace TKit::Perf
