#ifndef ZAX_GUI_H
#define ZAX_GUI_H

#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")

#define ZAX_ONSWITCH(_id, _handle) {case (_id):{(_handle);break;}}

typedef struct _WNDPROP {
	LPCTSTR szClass;
	LPCTSTR szText;
	DWORD dwExStyle;
	DWORD dwStyle;
	int nX;
	int nY;
	int nWidth;
	int nHeigth;
	HMENU hMenu;
} WNDPROP, *PWNDPROP;

typedef struct _MENUPROP {
	UINT uFlag;
	UINT_PTR uItemId;
	LPTSTR lpItem;
} MENUPROP, *PMENUPROP;

typedef struct _LVCOLPROP {
	LPTSTR lpColumn;
	int nWidth;
} LVCOLPROP, *PLVCOLPROP;

void ui_lv_insert_columns(const LVCOLPROP *LvColProp, int nCount);
void ui_lv_insert_item(int nIndex, const LPTSTR *szTexts, int nCount);
void ui_lv_clear_item();
void ui_lv_clear_column();
int ui_lv_get_cursel();
BOOL ui_lv_get_item(int nIndex, LPTSTR *szText, int *nCchText, int nCount);
int ui_main(HINSTANCE hInstance, int nCmdShow);

#endif
