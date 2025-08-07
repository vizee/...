#ifndef IDT_H
#define IDT_H

#include <ntddk.h>

#include "ds.h"

NTSTATUS EnumIDT(PVOID pBuffer);

#endif
