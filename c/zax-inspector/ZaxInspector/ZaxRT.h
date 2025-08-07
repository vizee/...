#ifndef ZAX_RT_H
#define ZAX_RT_H

#include <Windows.h>

#define PRIVILEGE_LOW_LOAD_DRIVER	(0x0a)
#define PRIVILEGE_LOW_DEBUG			(0x14)
#define RT_IIF(_condition, _true, _false) ((_condition)? (_true): (_false))

BOOL rt_enable_privilege(DWORD dwPrivilege);
BOOL rt_is_administrator();
BOOL rt_check_verion();
int rt_dwtos(DWORD dwNumber, int nRadix, int nFillZero, LPTSTR szOutText, int nOutLength);
int rt_stodw(LPTSTR szNumber, DWORD *pdwOut);
PVOID rt_valloc(DWORD dwSize);
void rt_vfree(PVOID pBase);
BOOL rt_save_dialog(LPTSTR szFile, DWORD dwMaxFile);
VOID rt_write(HANDLE hFile, LPTSTR *szTexts, DWORD dwCount);
int rt_btos(BYTE *pBytes, DWORD dwBytes, LPTSTR szText, DWORD dwLength);

#endif
