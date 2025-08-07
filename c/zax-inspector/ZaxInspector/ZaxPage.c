#include "ZaxPage.h"
#include "ZaxGUI.h"
#include "ZaxText.h"
#include "ZaxDialogId.h"
#define ZAX_DIALOG_PAGE
#include "ZaxDialog.h"
#include "ZaxDriver.h"
#include "ZaxRT.h"
#include "../zik/ZaxIoCode.h"

//DESP: update items for ssdt page
void page_ssdt(HANDLE hFile)
{
	PSSDT_INFO pSSDTInfo;
	TCHAR szI1[16];
	TCHAR szI3[16];
	TCHAR szI4[16];
	LPTSTR szItems[4] = {szI1, NULL, szI3, szI4};
	DWORD i;

	pSSDTInfo = (PSSDT_INFO)rt_valloc(0x4000);
	if (drv_callkernel(ZIK_IO_SSDT_R, pSSDTInfo, 0x4000)) {
		for (i = 0; i < pSSDTInfo->Count; i++) {
			rt_dwtos(pSSDTInfo->Item[i].Index, 10, 0, szI1, 15);
			rt_dwtos((DWORD)pSSDTInfo->Item[i].OrigAddr, 16, 8, szI3, 15);
			rt_dwtos((DWORD)pSSDTInfo->Item[i].CurAddr, 16, 8, szI4, 15);
			if (pSSDTInfo->Item[i].OrigAddr != pSSDTInfo->Item[i].CurAddr)
				szItems[1] = TEXT("Hook");
			else
				szItems[1] = TEXT("-");

			ui_lv_insert_item(i, szItems, 4);

			if (hFile != INVALID_HANDLE_VALUE)
				rt_write(hFile, szItems, 4);
		}
	}
	rt_vfree(pSSDTInfo);
	return;
}

//DESP: restore all hooks
void page_ssdt_restoreall()
{
	drv_callkernel(ZIK_IO_SSDT_W, NULL, 0);
	page_ssdt(INVALID_HANDLE_VALUE);
	return;
}

//DESP: update items for ssdt shadow page
void page_ssdts(HANDLE hFile)
{
	PSSDT_INFO pSSDTInfo;
	TCHAR szI1[16];
	TCHAR szI3[16];
	TCHAR szI4[16];
	LPTSTR szItems[4] = {szI1, NULL, szI3, szI4};
	DWORD i;

	pSSDTInfo = (PSSDT_INFO)rt_valloc(0x4000);
	if (drv_callkernel(ZIK_IO_SSDTS_R, pSSDTInfo, 0x4000)) {
		for (i = 0; i < pSSDTInfo->Count; i++) {
			rt_dwtos(pSSDTInfo->Item[i].Index, 10, 0, szI1, 15);
			rt_dwtos((DWORD)pSSDTInfo->Item[i].OrigAddr, 16, 8, szI3, 15);
			rt_dwtos((DWORD)pSSDTInfo->Item[i].CurAddr, 16, 8, szI4, 15);
			if (pSSDTInfo->Item[i].OrigAddr != pSSDTInfo->Item[i].CurAddr)
				szItems[1] = TEXT("Hook");
			else
				szItems[1] = TEXT("-");

			ui_lv_insert_item(i, szItems, 4);

			if (hFile != INVALID_HANDLE_VALUE)
				rt_write(hFile, szItems, 4);
		}
	}
	rt_vfree(pSSDTInfo);
	return;
}

//DESP: restore all hooks
void page_ssdts_restoreall()
{
	drv_callkernel(ZIK_IO_SSDTS_W, NULL, 0);
	//reload
	page_ssdts(INVALID_HANDLE_VALUE);
	return;
}

//DESP: update items for idt page
void page_idt(HANDLE hFile)
{
	PIDT_INFO pIDTInfo;
	TCHAR szI1[16];
	TCHAR szI2[16];
	TCHAR szI3[16];
	TCHAR szI4[16];
	LPTSTR szItems[4] = {szI1, NULL, szI3, szI4};
	DWORD i;

	pIDTInfo = (PIDT_INFO)rt_valloc(0x20000);
	if (drv_callkernel(ZIK_IO_IDT_R, pIDTInfo, 0x20000)) {
		for (i = 0; i < 0x7ff; i++) {
			rt_dwtos(i, 16, 0, szI1, 15);
			rt_dwtos((DWORD)pIDTInfo[i].Offset, 16, 8, szI3, 15);
			rt_dwtos(pIDTInfo[i].DPL, 16, 0, szI4, 15);

			switch (pIDTInfo[i].Type) {
			case 0:
				szItems[1] = TEXT("-");
				break;
			case 4:
				szItems[1] = TEXT("i286 CALLGATE");
				break;
			case 5:
				szItems[1] = TEXT("i286/i486 TASKGATE");
				break;
			case 6:
				szItems[1] = TEXT("i286 INTGATE");
				break;
			case 7:
				szItems[1] = TEXT("i286 TRAPGATE");
				break;
			case 12:
				szItems[1] = TEXT("i486 CALLGATE");
				break;
			case 14:
				szItems[1] = TEXT("i486 INTGATE");
				break;
			case 15:
				szItems[1] = TEXT("i486 TRAPGATE");
				break;
			default:
				rt_dwtos(pIDTInfo[i].Type, 10, 0, szI2, 15);
				szItems[1] = szI2;
			}

			ui_lv_insert_item(i, szItems, 4);
		
			if (hFile != INVALID_HANDLE_VALUE)
				rt_write(hFile, szItems, 4);
		}
	}
	rt_vfree(pIDTInfo);
	return;
}

//DESP: update items for kernel page
void page_kernel(HANDLE hFile)
{
	PKERNEL_INFO pKernelInfo;
	TCHAR szI1[16];
	TCHAR szI2[16];
	TCHAR szI3[60];
	TCHAR szI4[60];
	DWORD i;
	LPTSTR szItems[4] = {szI1, szI2, szI3, szI4};

	pKernelInfo = (PKERNEL_INFO)rt_valloc(0x4000);
	if (drv_callkernel(ZIK_IO_KERNEL_R, pKernelInfo, 0x4000)) {
		for (i = 0; i < pKernelInfo->Count; i++) {
			rt_dwtos((DWORD)pKernelInfo->Item[i].Address, 16, 8, szI1, 15);
			rt_dwtos(pKernelInfo->Item[i].Length, 10, 0, szI2, 15);
			rt_btos(pKernelInfo->Item[i].CurBytes, pKernelInfo->Item[i].Length, szI3, 59);
			rt_btos(pKernelInfo->Item[i].OrigBytes, pKernelInfo->Item[i].Length, szI4, 59);
			ui_lv_insert_item(0, szItems, 4);

			if (hFile != INVALID_HANDLE_VALUE)
				rt_write(hFile, szItems, 4);
		}
	}
	rt_vfree(pKernelInfo);
	return;
}

//DESP: restore all hooks
void page_kernel_restoreall()
{
	drv_callkernel(ZIK_IO_KERNEL_W, NULL, 0);
	page_kernel(INVALID_HANDLE_VALUE);
	return;
}

//DESP: export report
void page_export(int nPageIndex)
{
	TCHAR szFileName[MAX_PATH];
	HANDLE hFile;

	szFileName[0] = S_NULLCHAR;
	rt_save_dialog(szFileName, MAX_PATH);

	hFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		MessageBox(NULL, S_MB_EXPFAIL, S_MB_TITLE_ERR, MB_ICONHAND);
	else {
		page_refresh(nPageIndex, hFile);
		CloseHandle(hFile);
		MessageBox(NULL, S_MB_EXPCOMP, S_MB_TITLE_INFO, MB_ICONASTERISK);
	}
	
	return ;
}

//DESP: refresh page
void page_refresh(int nPageIndex, HANDLE hFile)
{
	ui_lv_clear_item();

	switch(nPageIndex) {
	case PAGE_SSDT:

		//ssdt
		page_ssdt(hFile);
		break;
	case PAGE_SSDTS:

		//ssdt shadow
		page_ssdts(hFile);
		break;
	case PAGE_IDT:

		//idt
		page_idt(hFile);
		break;
	case PAGE_KERNEL:

		//kernel
		page_kernel(hFile);
		break;
	}

	return;
}

//DESP: change page
void page_chage(int nPageIndex)
{
	ui_lv_clear_column();

	//DO: change page by index
	switch(nPageIndex) {
	case PAGE_SSDT:

		//ssdt
		ui_lv_insert_columns(ColumnSSDT, COL_SSDT);
		break;
	case PAGE_SSDTS:

		//ssdt shadow
		ui_lv_insert_columns(ColumnSSDTS, COL_SSDTS);
		break;
	case PAGE_IDT:

		//idt
		ui_lv_insert_columns(ColumnIDT, COL_IDT);
		break;
	case PAGE_KERNEL:

		//kernel
		ui_lv_insert_columns(ColumnKernel, COL_KERNEL);
		break;
	}

	page_refresh(nPageIndex, INVALID_HANDLE_VALUE);
	return;
}
