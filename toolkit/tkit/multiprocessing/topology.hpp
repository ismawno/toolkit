#pragma once

#include "tkit/utils/alias.hpp"

namespace TKit::Topology
{
struct Handle;

constexpr u32 Unknown = Limits<u32>::max();
const Handle *Initialize() noexcept;

void BuildAffinityOrder(const Handle *p_Handle) noexcept;
void PinThread(const Handle *p_Handle, u32 p_ThreadIndex) noexcept;

void SetThreadName(u32 p_ThreadIndex, const char *p_Name = nullptr) noexcept;

void Terminate(const Handle *p_Handle) noexcept;

} // namespace TKit::Topology
