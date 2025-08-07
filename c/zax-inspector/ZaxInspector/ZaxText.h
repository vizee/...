#ifndef ZAX_TEXT_H
#define ZAX_TEXT_H

//ui window class & text
#define UI_CLASSNAME	TEXT("ZaxInspector")
#define UI_WINDOWTEXT	TEXT("Zax Inspector")

//tab control item
#define UI_TC_SSDT		TEXT("SSDT")
#define	UI_TC_SSDTS		TEXT("SSDT Shadow")
#define UI_TC_IDT		TEXT("IDT")
#define UI_TC_KERNEL	TEXT("Kernel")

//ssdt page
#define UI_SSDT_INDEX	TEXT("Index")
#define UI_SSDT_FUNC	TEXT("Hook")
#define UI_SSDT_ORIG	TEXT("Orignal Address")
#define UI_SSDT_HOOK	TEXT("Current Address")

//ssdt shadow page
#define UI_SSDTS_INDEX	TEXT("Index")
#define UI_SSDTS_FUNC	TEXT("Hook")
#define UI_SSDTS_ORIG	TEXT("Orignal Address")
#define UI_SSDTS_HOOK	TEXT("Current Address")

//idt page
#define UI_IDT_INDEX	TEXT("Index")
#define UI_IDT_TYPE		TEXT("Type")
#define UI_IDT_OFFS		TEXT("Offset")
#define UI_IDT_DPL		TEXT("DPL")

//kernel page
#define UI_KERNEL_ADDR	TEXT("Address")
#define UI_KERNEL_LEN	TEXT("Length")
#define UI_KERNEL_CUR	TEXT("Current Data")
#define UI_KERNEL_ORIG	TEXT("Orignal Data")

//system menu
#define UI_MENU_ABOUT	TEXT("About Zax...")

//ssdt menu
#define MENU_SSDT_REFRESH		TEXT("Refresh")
#define MENU_SSDT_EXPORT		TEXT("Export")
#define MENU_SSDT_RESTOREALL	TEXT("Restore all")

//ssdt shadow menu
#define MENU_SSDTS_REFRESH		TEXT("Refresh")
#define MENU_SSDTS_EXPORT		TEXT("Export")
#define MENU_SSDTS_RESTOREALL	TEXT("Restore all")

//idt menu
#define MENU_IDT_REFRESH		TEXT("Refresh")
#define MENU_IDT_EXPORT			TEXT("Export")

//kernel menu
#define MENU_KERNEL_REFRESH		TEXT("Refresh")
#define MENU_KERNEL_EXPORT		TEXT("Export")
#define MENU_KERNEL_RESTOREALL	TEXT("Restore all")

//about message

#define S_EMPTY			TEXT("")
#define S_ABOUT_TEXT	TEXT("Zax Inspector(build 2013)\n")\
						TEXT("\t\tby vizee")
#define S_ABOUT_TITLE	TEXT("About Zax Inspector")

//message box
#define S_MB_TITLE_NOR	TEXT("Zax Inspector")
#define S_MB_TITLE_ERR	TEXT("Error")
#define S_MB_TITLE_INFO	TEXT("Information")
#define S_MB_TITLE_WARN	TEXT("Warning")
#define S_MB_TITLE_QUES	TEXT("Question")
#define S_MB_INITFAIL	TEXT("Failed to initialize")
#define S_MB_INITSUC	TEXT("Initialize successfully")
#define S_MB_RESTSUC	TEXT("Restore hooks successfully")
#define S_MB_RESTFAIL	TEXT("Failed torestore hooks")
#define S_MB_DISCONDRV	TEXT("Disconnecting to driver")
#define S_MB_EXPCOMP	TEXT("Export completed")
#define S_MB_EXPFAIL	TEXT("failed to Export")

#define S_SAVETITLE		TEXT("Select a file to export")
#define S_SAVEFILTER	TEXT("Text File (*.txt)\0*.txt\0")\
						TEXT("All File (*.*)\0*.*\0")\
						TEXT("\0")
#define S_NULLCHAR		TEXT('\x0')

#endif
