#pragma once

#ifndef TKIT_ENABLE_MULTIPROCESSING
#    error                                                                                                             \
        "[TOOLKIT][MULTIPROC] To include this file, the corresponding feature must be enabled in CMake with TOOLKIT_ENABLE_MULTIPROCESSING"
#endif

#include "tkit/utils/alias.hpp"

namespace TKit::Topology
{
struct Handle;

// This can only be used in thread pools that do not destroy their threads until the end of the program AND set the
// thread index at construction. This needs to be documented further. Look at thread pool as a valid usage example

usize GetThreadIndex();
void SetThreadIndex(usize p_ThreadIndex);
const Handle *Initialize();

void BuildAffinityOrder(const Handle *p_Handle);
void PinThread(const Handle *p_Handle, usize p_ThreadIndex);

void SetThreadName(usize p_ThreadIndex, const char *p_Name = nullptr);

void Terminate(const Handle *p_Handle);

} // namespace TKit::Topology
