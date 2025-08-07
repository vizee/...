#ifndef ZAX_PAGE_H
#define ZAX_PAGE_H

#include <Windows.h>

#define PAGE_SSDT		(0)
#define PAGE_SSDTS		(1)
#define PAGE_IDT		(2)
#define PAGE_KERNEL		(3)
#define PAGE_COUNT		(4)

void page_ssdt_restoreall();
void page_ssdts_restoreall();
void page_kernel_restoreall();
void page_export(int nPageIndex);
void page_refresh(int nPageIndex, HANDLE hFile);
void page_chage(int nPageIndex);

#endif
