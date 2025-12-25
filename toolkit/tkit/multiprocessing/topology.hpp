#pragma once

#include "tkit/utils/alias.hpp"

namespace TKit::Topology
{
struct Handle;

Handle *Initialize();

void BuildAffinityOrder(const Handle *p_Handle);
void PinThread(const Handle *p_Handle, u32 p_ThreadIndex);

void SetThreadName(u32 p_ThreadIndex, const char *p_Name = nullptr);

void Terminate(Handle *p_Handle);

} // namespace TKit::Topology
