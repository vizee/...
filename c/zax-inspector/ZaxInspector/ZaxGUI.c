#include "ZaxGUI.h"
#include "ZaxText.h"
#include "ZaxPage.h"
#define ZAX_DIALOG_UI
#include "ZaxDialog.h"
#include "ZaxRT.h"
#include "ZaxDriver.h"

HWND g_hMain, g_hTabCtrl, g_hListView;
HMENU g_hPopupMenu[PAGE_COUNT];
int g_nCurrentPage;

//DESP: insert columns with string array
void ui_lv_insert_columns(const LVCOLPROP *LvColProp, int nCount)
{
	int i;
	LV_COLUMN LvColumn;

	ZeroMemory(&LvColumn, sizeof(LV_COLUMN));
	LvColumn.mask = LVCF_TEXT | LVCF_WIDTH;

	for(i = 0; i < nCount; i++) {
		LvColumn.pszText = LvColProp[i].lpColumn;
		LvColumn.cx = LvColProp[i].nWidth;
		ListView_InsertColumn(g_hListView, i, &LvColumn);
	}

	return;
}

//DESP: insert item to listview
void ui_lv_insert_item(int nIndex, const LPTSTR *szTexts, int nCount)
{
	LVITEM LvItem;
	int i;

	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = nIndex;
	LvItem.iSubItem = 0;
	LvItem.pszText = szTexts[0];
	//insert new item
	nIndex = ListView_InsertItem(g_hListView, &LvItem);

	if(nIndex != -1)
		for(i = 1; i < nCount; i++)
			ListView_SetItemText(g_hListView, nIndex, i, szTexts[i]);

	return;
}

//DESP: clear listview item
void ui_lv_clear_item()
{
	ListView_DeleteAllItems(g_hListView);
	return;
}

//DESP: clear listview columns
void ui_lv_clear_column()
{
	int nColumnCount;
	
	nColumnCount = Header_GetItemCount(ListView_GetHeader(g_hListView));
	
	//DO: delete all column
	while(nColumnCount--)
		ListView_DeleteColumn(g_hListView, 0);

	return;
}

//DESP: get current select item
int ui_lv_get_cursel()
{
	return ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
}

void ui_lv_select(int nItem)
{
	LVITEM LvItem;

	LvItem.iItem = nItem;
	LvItem.iSubItem = 0;
	LvItem.mask = LVIF_STATE;
	LvItem.stateMask = LVIS_FOCUSED;

	ListView_SetItem(g_hListView, &LvItem);
}

//DESP: get specific item text
BOOL ui_lv_get_item(int nIndex, LPTSTR *szText, int *nCchText, int nCount)
{
	LVITEM LvItem;
	int i;

	LvItem.iItem = nIndex;
	LvItem.mask = LVIF_TEXT;

	for(i = 0; i < nCount; i++) {
		LvItem.iSubItem = i;
		LvItem.pszText = szText[i];
		LvItem.cchTextMax = nCchText[i];
		if(!ListView_GetItem(g_hListView, &LvItem))
			return FALSE;
	}

	return TRUE;
}

//DESP: on window close
void ui_on_close()
{
	int i;

	drv_deinitialize(FALSE);
	for(i = 0; i < PAGE_COUNT; i++)
		DestroyMenu(g_hPopupMenu[i]);

	return;
}

//DESP: on tab control change
void ui_on_tabctrl_change(int nCurrentPage)
{
	g_nCurrentPage = nCurrentPage;
	page_chage(nCurrentPage);
	return;
}

//DESP: on resize controls
void ui_on_resize(int nWidth, int nHeigth)
{
	HDWP hDwp;
	RECT ClientRect;

	SetRect(&ClientRect, 0, 0, nWidth, nHeigth);
	TabCtrl_AdjustRect(g_hTabCtrl, FALSE, &ClientRect);
	hDwp = BeginDeferWindowPos(2);
	DeferWindowPos(
		hDwp,
		g_hTabCtrl,
		NULL,
		0,
		0,
		nWidth,
		nHeigth,
		SWP_NOMOVE | SWP_NOZORDER);
	DeferWindowPos(
		hDwp,
		g_hListView,
		HWND_TOP,
		ClientRect.left + 2,
		ClientRect.top + 2,
		ClientRect.right - ClientRect.left - 4,
		ClientRect.bottom - ClientRect.top - 4,
		0);
	EndDeferWindowPos(hDwp);
	return;
}

//DESP: on listview right click
void ui_on_lv_rightclick(LPNMITEMACTIVATE pNMRClick)
{
	if(pNMRClick->iItem < 0)
		return;

	ClientToScreen(g_hListView, &pNMRClick->ptAction);
	//DO: track menu
	TrackPopupMenu(
		g_hPopupMenu[g_nCurrentPage],
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pNMRClick->ptAction.x,
		pNMRClick->ptAction.y,
		0,
		g_hMain,
		NULL);
	return;
}

//DESP: on window item command
void ui_on_command(DWORD dwControlId)
{
	//DO: check control id
	if(dwControlId < ZAX_ID(10) || dwControlId > ZAX_ID(50))
		return;
	else if(dwControlId < ZAX_ID(20)) {
		//ssdt

		switch(dwControlId) {
			//refresh
			ZAX_ONSWITCH(
				IDM_SSDT_REFRESH,
				page_refresh(g_nCurrentPage, INVALID_HANDLE_VALUE));

			//export
			ZAX_ONSWITCH(
				IDM_SSDT_EXPORT,
				page_export(g_nCurrentPage));

			//restore all
			ZAX_ONSWITCH(
				IDM_SSDT_RESTOREALL,
				page_ssdt_restoreall());

		}

	} else if(dwControlId < ZAX_ID(30)) {
		//ssdt shadow

		switch(dwControlId) {
			//refresh
			ZAX_ONSWITCH(
				IDM_SSDTS_REFRESH,
				page_refresh(g_nCurrentPage, INVALID_HANDLE_VALUE));

			//export
			ZAX_ONSWITCH(
				IDM_SSDTS_EXPORT,
				page_export(g_nCurrentPage));

			//restore all
			ZAX_ONSWITCH(
				IDM_SSDTS_RESTOREALL,
				page_ssdts_restoreall());

		}

	} else if(dwControlId < ZAX_ID(40)) {
		//idt
		
		switch(dwControlId) {
			//refresh
			ZAX_ONSWITCH(
				IDM_IDT_REFRESH,
				page_refresh(g_nCurrentPage, INVALID_HANDLE_VALUE));

			//export
			ZAX_ONSWITCH(
				IDM_IDT_EXPORT,
				page_export(g_nCurrentPage));

		}

	} else if(dwControlId < ZAX_ID(50)) {
		//kernel
		
		switch(dwControlId) {
			//refresh
			ZAX_ONSWITCH(
				IDM_KERNEL_REFRESH,
				page_refresh(g_nCurrentPage, INVALID_HANDLE_VALUE));

			//export
			ZAX_ONSWITCH(
				IDM_KERNEL_EXPORT,
				page_export(g_nCurrentPage));

			//restore all
			ZAX_ONSWITCH(
				IDM_KERNEL_RESTOREALL,
				page_kernel_restoreall());

		}

	}

	return;
}

//DESP: wndproc
LRESULT CALLBACK ui_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//DO: handle message
	switch(uMsg) {
	case WM_NOTIFY:
		
		//WM_NOTIFY message
		if((int)wParam == IDC_LISTVIEW && ((NMHDR *)lParam)->code == NM_RCLICK)
			ui_on_lv_rightclick((LPNMITEMACTIVATE)lParam);
		else if((int)wParam == IDC_TABCTRL && ((NMHDR *)lParam)->code == TCN_SELCHANGE)
			ui_on_tabctrl_change(TabCtrl_GetCurSel(g_hTabCtrl));
		else
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		
		break;
	case WM_COMMAND:

		//WM_COMMAND message
		ui_on_command((DWORD)wParam);
		break;
	case WM_SIZE:

		//WM_SIZE message
		ui_on_resize((int)LOWORD(lParam), (int)HIWORD(lParam));
		break;
	case WM_CLOSE:

		//WM_CLOSE message
		ui_on_close();
		DestroyWindow(g_hMain);
		break;
	case WM_DESTROY:

		//WM_DESTROY message
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:

		if(LOWORD(wParam) == IDM_SYSABOUT)
			MessageBox(hWnd, S_ABOUT_TEXT, S_ABOUT_TITLE, MB_ICONASTERISK);

	default:

		//default
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

//DESP: append sub menu to menu
void ui_append_menus(HMENU hMenu, const MENUPROP *pMenus, int nCount)
{
	int i;

	for(i = 0; i < nCount; i++)
		AppendMenu(hMenu, pMenus[i].uFlag, pMenus[i].uItemId, pMenus[i].lpItem);
	
	return;
}

//DESP: before create
BOOL ui_on_before_create()
{
	BOOL bSuccess = TRUE;
	bSuccess = rt_check_verion();
	if (bSuccess)
		bSuccess = rt_is_administrator();
	if (bSuccess)
		bSuccess = rt_enable_privilege(PRIVILEGE_LOW_DEBUG);
	if (bSuccess)
		bSuccess = drv_initialize();
	
	if (!bSuccess)
		MessageBox(NULL, S_MB_INITFAIL, S_MB_TITLE_ERR, MB_ICONHAND);

	return bSuccess;
}

//DESP: after create
void ui_on_after_create()
{
	int i;
	TCITEM TCItem;

	//DO: attach menu to sysmenu
	ui_append_menus(GetSystemMenu(g_hMain, FALSE), SysMenuProp, SYSMENU_ABOUT_COUNT);
	
	//TODO: create popup menus
	for(i = 0;i < PAGE_COUNT; i++)
		g_hPopupMenu[i] = CreatePopupMenu();

	//TODO: append menu sub items
	ui_append_menus(g_hPopupMenu[PAGE_SSDT], SSDTMenuProp, MENU_SSDT_COUNT);
	ui_append_menus(g_hPopupMenu[PAGE_SSDTS], SSDTSMenuProp, MENU_SSDTS_COUNT);
	ui_append_menus(g_hPopupMenu[PAGE_IDT], IDTMenuProp, MENU_IDT_COUNT);
	ui_append_menus(g_hPopupMenu[PAGE_KERNEL], KernelMenuProp, MENU_KERNEL_COUNT);

	//DO: initialize tab control
	ZeroMemory(&TCItem, sizeof(TCItem));
	TCItem.mask = TCIF_TEXT;

	for(i = 0; i < TAB_COUNT; i++) {
		TCItem.pszText = TabCtrlText[i];
		TCItem.cchTextMax = lstrlen(TabCtrlText[i]);
		TabCtrl_InsertItem(g_hTabCtrl, i, &TCItem);
	}

	//DO: initialize listview
	ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_FULLROWSELECT);
	page_chage(TabCtrl_GetCurSel(g_hTabCtrl));

	return ;
}

//DESP: create window & controls
BOOL ui_create(HINSTANCE hInstance)
{
	RECT ClientRect;
	WNDCLASSEX WndClass;
	HFONT hFont;

	//before create
	if(!ui_on_before_create())
		return FALSE;

	//DO: register class
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = ui_wndproc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hIconSm = WndClass.hIcon;
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = UI_CLASSNAME;

	if(!RegisterClassEx(&WndClass))
		return FALSE;
	
	//DO: common controls
	InitCommonControls();

	//DO: create window
	hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	g_hMain = CreateWindowEx(
		MainProp.dwExStyle,
		MainProp.szClass,
		MainProp.szText,
		MainProp.dwStyle,
		MainProp.nX,
		MainProp.nY,
		MainProp.nWidth,
		MainProp.nHeigth,
		NULL,
		MainProp.hMenu,
		hInstance,
		NULL);

	if(!g_hMain)
		return FALSE;

	if(!GetClientRect(g_hMain, &ClientRect))
		return FALSE;

	//DO: create tab control
	g_hTabCtrl = CreateWindowEx(
		TabCtrlProp.dwExStyle,
		TabCtrlProp.szClass,
		TabCtrlProp.szText,
		TabCtrlProp.dwStyle,
		ClientRect.left,
		ClientRect.top,
		ClientRect.right,
		ClientRect.bottom,
		g_hMain,
		TabCtrlProp.hMenu,
		hInstance,
		NULL);

	if(!g_hTabCtrl)
		return FALSE;

	SetWindowFont(g_hTabCtrl, hFont, FALSE);
	TabCtrl_AdjustRect(g_hTabCtrl, FALSE, &ClientRect);

	//DO: create listview control
	g_hListView = CreateWindowEx(
		ListViewProp.dwExStyle,
		ListViewProp.szClass,
		ListViewProp.szText,
		ListViewProp.dwStyle,
		ClientRect.left + 2,
		ClientRect.top + 2,
		ClientRect.right - 4,
		ClientRect.bottom - ClientRect.top - 4,
		g_hMain,
		ListViewProp.hMenu,
		hInstance,
		NULL);

	if(!g_hListView)
		return FALSE;

	SetWindowFont(g_hListView, hFont, FALSE);

	//after create
	ui_on_after_create();

	return TRUE;
}

//DESP: ui main
int ui_main(HINSTANCE hInstance, int nCmdShow)
{
	MSG Msg;

	//DO: create window
	if(!ui_create(hInstance))
		return 0;

	if(ShowWindow(g_hMain, nCmdShow))
		return 0;

	UpdateWindow(g_hMain);

	//DO: message loop
	while(GetMessage(&Msg, NULL, 0, 0)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return (int)Msg.wParam;
}
