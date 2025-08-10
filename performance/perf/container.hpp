#pragma once

#include "perf/settings.hpp"

namespace TKit::Perf
{
void RecordVector(const ContainerSettings &p_Settings) noexcept;
void RecordStaticArray(const ContainerSettings &p_Settings) noexcept;
void RecordDynamicArray(const ContainerSettings &p_Settings) noexcept;

void RecordDeque(const ContainerSettings &p_Settings) noexcept;
void RecordStaticDeque(const ContainerSettings &p_Settings) noexcept;
void RecordDynamicDeque(const ContainerSettings &p_Settings) noexcept;
} // namespace TKit::Perf
