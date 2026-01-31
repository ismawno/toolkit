#pragma once

#include "perf/settings.hpp"

namespace TKit
{
void RecordVector(const ContainerSettings &settings);
void RecordStaticArray(const ContainerSettings &settings);
void RecordDynamicArray(const ContainerSettings &settings);
} // namespace TKit
