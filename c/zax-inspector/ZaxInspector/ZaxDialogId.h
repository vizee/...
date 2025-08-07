#ifndef ZAX_DIALOGID_H
#define ZAX_DIALOGID_H

#define ZAX_IDBASE	(1000)
#define ZAX_ID(_i)	(ZAX_IDBASE + (_i))

#define IDC_TABCTRL		ZAX_ID(1)
#define IDC_LISTVIEW	ZAX_ID(2)
#define IDM_SYSABOUT	ZAX_ID(3)

//menu
#define SYSMENU_ABOUT_COUNT		(2)

//ssdt menu
#define MENU_SSDT_COUNT			(5)
#define IDM_SSDT_REFRESH		ZAX_ID(10)
#define IDM_SSDT_EXPORT			ZAX_ID(11)
#define IDM_SSDT_RESTOREALL		ZAX_ID(12)

//ssdt shadow menu
#define MENU_SSDTS_COUNT		(5)
#define IDM_SSDTS_REFRESH		ZAX_ID(20)
#define IDM_SSDTS_EXPORT		ZAX_ID(21)
#define IDM_SSDTS_RESTOREALL	ZAX_ID(22)

//idt menu
#define MENU_IDT_COUNT			(3)
#define IDM_IDT_REFRESH			ZAX_ID(30)
#define IDM_IDT_EXPORT			ZAX_ID(31)

//kernel menu
#define MENU_KERNEL_COUNT		(5)
#define IDM_KERNEL_REFRESH		ZAX_ID(40)
#define IDM_KERNEL_EXPORT		ZAX_ID(41)
#define IDM_KERNEL_RESTOREALL	ZAX_ID(42)

#endif
