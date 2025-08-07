#ifndef KERNEL_H
#define KERNEL_H

#include <ntddk.h>
#include "ds.h"

NTSTATUS EnumKernel(ULONG uRestoreAll, PKERNEL_INFO KernelInfo);

#endif
