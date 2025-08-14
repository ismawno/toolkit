#pragma once

#include "tkit/utils/alias.hpp"

namespace TKit::Topology
{
constexpr u32 Unknown = Limits<u32>::max();
void Initialize() noexcept;

void BuildAffinityOrder() noexcept;
void PinThread(u32 p_ThreadIndex) noexcept;

void SetThreadName(u32 p_ThreadIndex, const char *p_Name = nullptr) noexcept;

void Terminate() noexcept;

} // namespace TKit::Topology
