#pragma once

#include "tkit/utils/alias.hpp"
#include "tkit/utils/limits.hpp"

namespace TKit::Topology
{
struct Handle;

constexpr u32 Unknown = Limits<u32>::Max();
const Handle *Initialize();

void BuildAffinityOrder(const Handle *p_Handle);
void PinThread(const Handle *p_Handle, u32 p_ThreadIndex);

void SetThreadName(u32 p_ThreadIndex, const char *p_Name = nullptr);

void Terminate(const Handle *p_Handle);

} // namespace TKit::Topology
