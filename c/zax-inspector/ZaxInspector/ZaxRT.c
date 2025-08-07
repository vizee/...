#include "ZaxRT.h"
#include "ZaxText.h"
#include "ntdll.h"

//DESP: check os version
BOOL rt_check_verion()
{
	OSVERSIONINFOEX OSVersionInfo;
#ifdef RT_CHECK_WOW64
	FARPROC fnIsWow64Process;
	BOOL bIsWow64;

	bIsWow64 = FALSE;
	fnIsWow64Process = GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "IsWow64Process");

	if(fnIsWow64Process)
		if(((BOOL (WINAPI *)(HANDLE, PBOOL))fnIsWow64Process)(GetCurrentProcess(), &bIsWow64))
			if(bIsWow64)
				return FALSE;
#endif
	OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((LPOSVERSIONINFO)&OSVersionInfo))
		return FALSE;

	if(OSVersionInfo.wProductType == VER_NT_SERVER)
		return FALSE;

	switch(OSVersionInfo.dwBuildNumber) {
	case 2600: //Windwos XP 5.1.2600
	case 7601: //Windows 7 SP1
		return TRUE;
	}

	return FALSE;
}

//DESP: convert number to string
int rt_dwtos(DWORD dwNumber, int nRadix, int nFillZero, LPTSTR szOutText, int nOutLength)
{
	int i, nLength, nDigit;
	TCHAR szTemp[16];

	if(nRadix < 10 || nRadix > 16 || nFillZero >= nOutLength)
		return 0;

	nLength = 0;
	i = 0;

	if(dwNumber == 0)
		szTemp[i++] = TEXT('0');

	//DO: reverse number to 'szTemp'
	while(dwNumber > 0) {
		nDigit = dwNumber % nRadix;
		szTemp[i++] = nDigit + RT_IIF(nDigit >= 10, TEXT('A') - 10, TEXT('0'));
		dwNumber /= nRadix;
	}

	//DO: fill zero
	while(i < nFillZero)
		szTemp[i++] = TEXT('0');

	if(nOutLength > i) {
		
		//DO: reverse & copy string
		while(i > 0)
			szOutText[nLength++] = szTemp[--i];

		//null terminated
		szOutText[nLength] = 0;
	}
	
	return nLength;
}

int rt_btos(BYTE *pBytes, DWORD dwBytes, LPTSTR szText, DWORD dwLength)
{
	DWORD i, nLength;

	nLength = 0;
	
	for (i = 0; i < dwBytes; i++) {
		if (nLength >= dwLength) {
			nLength = dwLength - 1;
			break;
		}
		if (i)
			szText[nLength++] = TEXT(' ');
		rt_dwtos(pBytes[i], 16, 2, &szText[nLength], dwLength - nLength);
		nLength += 2;
	}
	szText[nLength] = TEXT('\0');
	return nLength;
}

//DESP: allocate virtual memory
PVOID rt_valloc(DWORD dwSize)
{
	PVOID pBase;
	NTSTATUS ntStatus;

	pBase = NULL;
	ntStatus = NtAllocateVirtualMemory(
		NtCurrentProcess(),
		&pBase,
		0,
		&dwSize,
		MEM_COMMIT,
		PAGE_READWRITE);
	return NT_SUCCESS(ntStatus)? pBase: NULL;
}

//DESP: free virtual memory
void rt_vfree(PVOID pBase)
{
	DWORD dwSize;

	dwSize = 0;
	NtFreeVirtualMemory(
		NtCurrentProcess(),
		&pBase,
		&dwSize,
		MEM_RELEASE);
	return;
}

//DESP: convert string to dword
int rt_stodw(LPTSTR szNumber, DWORD *pdwOut)
{
	int nLength;
	DWORD dwTemp, dwRadix;
	BOOL bIsHex;
	
	nLength = 0;
	dwTemp = 0;
	bIsHex = RT_IIF(szNumber[0] == TEXT('0') && szNumber[1] == TEXT('x'), TRUE, FALSE);

	if(bIsHex) {
		dwRadix = 16;
		nLength = 2;
	} else {
		dwRadix = 10;
		nLength = 0;
	}

	while(szNumber[nLength]) {
		dwTemp *= dwRadix;

		if(szNumber[nLength] >= TEXT('0') && szNumber[nLength] <= TEXT('9'))
			dwTemp += (szNumber[nLength] - TEXT('0'));
		else if(bIsHex) {
			
			if(szNumber[nLength] >= TEXT('A') && szNumber[nLength] <= TEXT('Z'))
				dwTemp += (szNumber[nLength] - TEXT('A'));
			else if(szNumber[nLength] >= TEXT('a') && szNumber[nLength] <= TEXT('z'))
				dwTemp += (szNumber[nLength] - TEXT('a'));
			else
				break; //invalid char

		} else
			break; //invalid char

		nLength++;
	}

	if(nLength > 0)
		*pdwOut = dwTemp;

	return nLength;
}

BOOL rt_enable_privilege(DWORD dwPrivilege)
{
	NTSTATUS ntStatus;
	HANDLE hToken;
	ULONG uLength;
	TOKEN_PRIVILEGES NewState, OldState;

	//DO: open process token
	ntStatus = NtOpenProcessToken(
		NtCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hToken);

	if(NT_SUCCESS(ntStatus)) {
		NewState.PrivilegeCount = 1;
		NewState.Privileges->Attributes = SE_PRIVILEGE_ENABLED;
		NewState.Privileges->Luid.HighPart = 0;
		NewState.Privileges->Luid.LowPart = dwPrivilege;
		ntStatus = NtAdjustPrivilegesToken(
			hToken,
			FALSE,
			&NewState,
			sizeof(TOKEN_PRIVILEGES),
			&OldState,
			&uLength);
		NtClose(hToken);
	}

	return NT_SUCCESS(ntStatus);
}

BOOL rt_is_administrator()
{
	BOOL bIsAdministrator;
	PSID adminsGroup;
	SID_IDENTIFIER_AUTHORITY tNtAuthority = SECURITY_NT_AUTHORITY;

	bIsAdministrator = AllocateAndInitializeSid(
		&tNtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0,0,0,0,0,0,
		&adminsGroup);
	if(bIsAdministrator) {
		if(!CheckTokenMembership(NULL,adminsGroup,&bIsAdministrator))
			bIsAdministrator = FALSE;
		FreeSid(adminsGroup);
	}
	return (bIsAdministrator != FALSE);
}

BOOL rt_save_dialog(LPTSTR szFile, DWORD dwMaxFile)
{
	OPENFILENAME pfn;
	
	ZeroMemory(&pfn, sizeof(OPENFILENAME));

	pfn.lStructSize = sizeof(OPENFILENAME);
	pfn.lpstrFilter = S_SAVEFILTER;
	pfn.lpstrFile = szFile;
	pfn.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
	pfn.nMaxFile = dwMaxFile;

	if (GetSaveFileName(&pfn))
		return TRUE;

	return FALSE;
}

VOID rt_write(HANDLE hFile, LPTSTR *szTexts, DWORD dwCount)
{
	DWORD i, n, l;
	CHAR szBuffer[MAX_PATH];
	LPSTR szText;

	if (hFile != INVALID_HANDLE_VALUE) {
		for (i = 0; i < dwCount; i++) {
			l = lstrlen(szTexts[i]);
#ifdef UNICODE
			szText = szBuffer;
			WideCharToMultiByte(
				CP_ACP,
				0,
				szTexts[i],
				l,
				szBuffer,
				MAX_PATH,
				NULL,
				NULL);
#else
			szText = szTexts[i];
#endif
			szText[l] = '\0';
			WriteFile(hFile, szText, l, &n, NULL);
			WriteFile(hFile, "\t", sizeof(CHAR), &n, NULL);
		}
		WriteFile(hFile, "\r\n", 2 * sizeof(CHAR), &n, NULL);
	}
}

BOOL rt_initialize()
{
	return TRUE;
}

void rt_deinitialize()
{
	return;
}
