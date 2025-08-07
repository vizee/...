#include "ZaxDriver.h"
#include <strsafe.h>
#include "ntdll.h"

#define DEVICE_NAME		TEXT("\\\\.\\zik")
#define SERVICE_NAME	TEXT("zik")
#define DRIVER_FILE		TEXT("zik.sys")

HANDLE g_hDevice;
SC_HANDLE g_hSCManager;

//DESP: obtain a object for access the driver device
HANDLE drv_link(LPWSTR szDevice)
{
	HANDLE hDevice;

	if(!szDevice)
		return INVALID_HANDLE_VALUE;

	hDevice = CreateFile(szDevice, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	return hDevice;
}

BOOL drv_start(LPTSTR szName)
{
	BOOL bSuccess;
	SC_HANDLE hService;

	bSuccess = FALSE;
	hService = OpenService(g_hSCManager, szName, SERVICE_ALL_ACCESS);
	if (hService)
		bSuccess = StartService(hService, 0, NULL);

	CloseServiceHandle(hService);
	return bSuccess;
}

BOOL drv_control(LPTSTR szName, DWORD dwControl)
{
	BOOL bSuccess;
	SC_HANDLE hService;
	SERVICE_STATUS ServiceStatus;

	bSuccess = FALSE;
	hService = OpenService(g_hSCManager, szName, SERVICE_ALL_ACCESS);
	if (hService)
		bSuccess = ControlService(hService, dwControl, &ServiceStatus);
	CloseServiceHandle(hService);
	return bSuccess;
}

//DESP: install driver
BOOL drv_install(LPTSTR szPath, LPTSTR szName, BOOL bDeleteOnly)
{
	
	SC_HANDLE hService;
	BOOL bSuccess;

	bSuccess = FALSE;
	

	hService = OpenService(g_hSCManager, szName, SERVICE_ALL_ACCESS);
	//always reinstall
	if (hService) {
		drv_control(SERVICE_NAME, SERVICE_CONTROL_STOP);
		DeleteService(hService);
		CloseServiceHandle(hService);

		if (bDeleteOnly)
			return TRUE;
	}

	hService = CreateService(
		g_hSCManager,
		szName,
		szName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);
	if (hService) {
		bSuccess = TRUE;
		CloseServiceHandle(hService);
	}

	return bSuccess;
}

//DESP: call the kernel driver
BOOL drv_callkernel(DWORD dwIoCode, LPVOID lpBuffer, DWORD dwBufferSize)
{
	DWORD dwRead;
	if (g_hDevice == NULL || g_hDevice == INVALID_HANDLE_VALUE)
		return FALSE;
	return DeviceIoControl(g_hDevice, dwIoCode, NULL, 0, lpBuffer, dwBufferSize, &dwRead, NULL);
}

BOOL drv_initialize()
{
	TCHAR szPath[MAX_PATH];
	DWORD dwLength;

	g_hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	dwLength = GetModuleFileName(NULL, szPath, MAX_PATH);
	while (dwLength--) {
		if (szPath[dwLength] == TEXT('\\')) {
			szPath[dwLength + 1] = TEXT('\0');
			break;
		}
	}
	StringCchCat(szPath, MAX_PATH, DRIVER_FILE);
	drv_install(szPath, SERVICE_NAME, FALSE);
	drv_start(SERVICE_NAME);
	g_hDevice = drv_link(DEVICE_NAME);
	return TRUE;
}

VOID drv_deinitialize(BOOL bUninstall)
{
	drv_control(SERVICE_NAME, SERVICE_CONTROL_STOP);
	if (bUninstall)
		drv_install(NULL, SERVICE_NAME, TRUE);

	if (g_hSCManager) {
		CloseServiceHandle(g_hSCManager);
		g_hSCManager = NULL;
	}
	if (g_hDevice && g_hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(g_hDevice);
		g_hDevice = NULL;
	}
	return;
}
