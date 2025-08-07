#ifndef ZAX_DRIVER_H
#define ZAX_DRIVER_H

#include <Windows.h>
#include "../zik/ds.h"

BOOL drv_callkernel(DWORD dwIoCode, LPVOID lpBuffer, DWORD dwBufferSize);
BOOL drv_initialize();
VOID drv_deinitialize(BOOL bUninstall);

#endif
