#include "WohnUI.h"
#include "WohnText.h"
#include "WohnCore.h"
#include "resource.h"

#define MAX_KEY_LEN (63)
#define MIN_KEY_LEN (8)
#define MAX_SSID_LEN (32)
#define IDM_ABOUT (100)
#define MT_ERROR	(1)
#define MT_WARNING (2)
#define MT_INFORMATION (3)

HWND gMainUI;
bool gHNStarted,gNCShared;
WCHAR sWlanSSID[MAX_SSID_LEN + 1],sWlanPasswd[MAX_KEY_LEN + 1];

void
	ui_log_clear
	(void)
{
	HWND hList;
	DWORD dwCount;

	hList = GetDlgItem(gMainUI,IDC_LST_LOG);
	dwCount = SendMessage(hList,LB_GETCOUNT,NULL,NULL);
	while(dwCount) {
		SendMessage(
			hList,
			LB_DELETESTRING,
			(WPARAM)--dwCount,NULL);
	}
	return;
}

void
	ui_log_add
	(LPWSTR sLog)
{
	SYSTEMTIME tSystemTime;
	WCHAR sText[256];
	
	if(lstrlen(sLog) > 160)
		return;
	if(sText) {
		GetLocalTime(&tSystemTime);
		StringCchPrintf(
			sText,255,SLOG_FORMAT,
			tSystemTime.wYear,
			tSystemTime.wMonth,
			tSystemTime.wDay,
			tSystemTime.wHour,
			tSystemTime.wMinute,
			tSystemTime.wSecond,
			sLog);
		SendDlgItemMessage( 
			gMainUI,
			IDC_LST_LOG,
			LB_ADDSTRING,
			NULL,(LPARAM)sText);
	}
	return;
}

void
	ui_msgbox
	(LPWSTR sText,DWORD dwType)
{
	UINT uIcon;
	LPWSTR sTitle;

	switch(dwType) {
	case MT_ERROR://error
		uIcon = MB_ICONERROR;
		sTitle = SMB_ERROR;
		break;
	case MT_WARNING://warning
		uIcon = MB_ICONWARNING;
		sTitle = SMB_WARNING;
		break;
	case MT_INFORMATION://information
		uIcon = MB_ICONINFORMATION;
		sTitle = SMB_INFORMATION;
		break;
	default:
		uIcon = 0;
		sTitle = SMB_DEFAULT;
	}
	MessageBox(gMainUI,sText,sTitle,uIcon);
	return;
}

int CALLBACK
	ui_enum_conn
	(NETCON_PROPERTIES *pNetconProps,LPVOID lpContext)
{
	SendMessage(
		(HWND)lpContext,
		CB_ADDSTRING,
		NULL,(LPARAM)pNetconProps->pszwName);
	return 0;
}

//wlan callback
void CALLBACK
	ui_on_hn_unavailable
	(LPVOID lpContext)
{
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_SSID),
		FALSE);
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_PASSWD),
		FALSE);
	EnableWindow(
		GetDlgItem(gMainUI,IDC_BTN_HN),
		FALSE);
	ui_log_add(SLOG_WLANUNAVL);
	gHNStarted = false;
	return;
}

void CALLBACK
	ui_on_hn_active
	(LPVOID lpContext)
{
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_SSID),
		TRUE);
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_PASSWD),
		TRUE);
	EnableWindow(
		GetDlgItem(gMainUI,IDC_BTN_HN),
		TRUE);
	ui_log_add(SLOG_WLANACTED);
	gHNStarted = false;
	return;
}

void CALLBACK
	ui_on_hn_started
	(LPVOID lpContext)
{
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_SSID),
		FALSE);
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_PASSWD),
		FALSE);
	SendMessage(
		GetDlgItem(gMainUI,IDC_BTN_HN),
		WM_SETTEXT,
		NULL,(LPARAM)SBTN_STOP);
	ui_log_add(SLOG_WLANACTED);
	gHNStarted = true;
	return;
}

void CALLBACK
	ui_on_hn_stoped
	(LPVOID lpContext)
{
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_SSID),
		TRUE);
	EnableWindow(
		GetDlgItem(gMainUI,IDC_EDT_PASSWD),
		TRUE);
	SendMessage(
		GetDlgItem(gMainUI,IDC_BTN_HN),
		WM_SETTEXT,
		NULL,(LPARAM)SBTN_START);
	gHNStarted = false;
	return;
}

void CALLBACK
	ui_on_hn_join
	(LPVOID lpContext)
{
	WCHAR szJoinLog[200];
	DOT11_MAC_ADDRESS tMacAddress;

	RtlCopyMemory(tMacAddress,lpContext,sizeof(DOT11_MAC_ADDRESS));
	StringCchPrintf(
		szJoinLog,199,
		SLOG_WLANJOIN,
		tMacAddress[0],
		tMacAddress[1],
		tMacAddress[2],
		tMacAddress[3],
		tMacAddress[4],
		tMacAddress[5]);
	ui_log_add(szJoinLog);
	return;
}

//ui
void
	ui_on_lst_log_dblclick
	(void)
{
	HWND hList;
	INT nIndex;
	WCHAR sText[256];
	DWORD dwLength;

	hList = GetDlgItem(gMainUI,IDC_LST_LOG);
	nIndex = SendMessage(
		hList,
		LB_GETCURSEL,
		NULL,NULL);
	if(nIndex >= 0) {
		dwLength = SendMessage(
			hList,
			LB_GETTEXT,
			(WPARAM)nIndex,(LPARAM)sText);
		if(dwLength > 0 && dwLength <= 255)
			ui_msgbox(sText,MT_INFORMATION);
	}
	return;
}

void
	ui_on_btn_hn_click
	(void)
{
	HWND hButton,hEditSSID,hEditPasswd;
	DWORD dwSSIDLen,dwPasswdLen;
	WCHAR sSSID[MAX_SSID_LEN + 1],sPasswd[MAX_KEY_LEN + 1];

	hButton = GetDlgItem(gMainUI,IDC_BTN_HN);
	hEditSSID = GetDlgItem(gMainUI,IDC_EDT_SSID);
	hEditPasswd = GetDlgItem(gMainUI,IDC_EDT_PASSWD);
	switch(gHNStarted) {
	case false: //start hosted network
		dwSSIDLen = SendMessage(
			hEditSSID,
			WM_GETTEXTLENGTH,
			NULL,NULL);
		if(dwSSIDLen == 0 || dwSSIDLen > MAX_SSID_LEN) {
			ui_msgbox(SMB_SSID_LEN,MT_ERROR);
			SetFocus(hEditSSID);
			break;
		}
		SendMessage(
			hEditSSID,
			WM_GETTEXT,
			(WPARAM)dwSSIDLen + 1,(LPARAM)sSSID);
		if(lstrcmp(sSSID,sWlanSSID)) {
			if(cor_wlan_set_ssid(sSSID))
				lstrcpy(sWlanSSID,sSSID);
			else {
				ui_msgbox(SMB_SSID_NOUPD,MT_ERROR);
				SetFocus(hEditSSID);
				break;
			}
		}
		dwPasswdLen = SendMessage(
			hEditPasswd,
			WM_GETTEXTLENGTH,
			NULL,NULL);
		if(dwPasswdLen < MIN_KEY_LEN || dwPasswdLen > MAX_KEY_LEN) {
			ui_msgbox(SMB_PWD_LEN,MT_ERROR);
			SetFocus(hEditPasswd);
			break;
		}
		SendMessage(
			hEditPasswd,
			WM_GETTEXT,
			(WPARAM)dwPasswdLen + 1,(LPARAM)sPasswd);
		if(lstrcmp(sPasswd,sWlanPasswd)) {
			if(cor_wlan_set_ssid(sPasswd))
				lstrcpy(sWlanPasswd,sPasswd);
			else {
				ui_msgbox(SMB_PWD_NOUPD,MT_ERROR);
				SetFocus(hEditPasswd);
				break;
			}
		}
		if(cor_wlan_start()) {
			EnableWindow(
				hEditSSID,
				FALSE);
			EnableWindow(
				hEditPasswd,
				FALSE);
			SendMessage(
				hButton,
				WM_SETTEXT,
				NULL,(LPARAM)SBTN_STOP);
			ui_log_add(SLOG_STARTWLAN);
			gHNStarted = true;
		} else
			ui_msgbox(SMB_HN_STARTF,MT_ERROR);
		break;
	case true: //stop hosted network
		if(cor_wlan_stop(true)) {
			EnableWindow(
				hEditSSID,
				TRUE);
			EnableWindow(
				hEditPasswd,
				TRUE);
			SendMessage(
				hButton,
				WM_SETTEXT,
				NULL,(LPARAM)SBTN_START);
			ui_log_add(SLOG_STOPWLAN);
			gHNStarted = false;
		}
	}
	return;
}

void
	ui_on_btn_ics_click
	(void)
{
	WCHAR sPublicName[256];
	HWND hButton;
	HWND hCombo;
	DWORD dwIndex,dwLength;

	hButton = GetDlgItem(gMainUI,IDC_BTN_ICS);
	switch(gNCShared) {
	case false: //enable ics
		hCombo = GetDlgItem(gMainUI,IDC_CMB_CONN);
		dwIndex = SendMessage(
			hCombo,
			CB_GETCURSEL,
			NULL,NULL);
		if(dwIndex == 0) {
			ui_msgbox(SMB_CMB_INV,MT_ERROR);
			break;
		}
		dwLength = SendMessage(
			hCombo,
			CB_GETLBTEXT,
			(WPARAM)dwIndex,(LPARAM)sPublicName);
		if(dwLength == 0 || dwLength > 255) {
			ui_log_add(SLOG_UNKNOWN);
			break;
		}
		cor_ics_enable(sPublicName);
		if(cor_ics_is_enabled()) {
			EnableWindow(
				GetDlgItem(gMainUI,IDC_CMB_CONN),
				FALSE);
			SendMessage(
				hButton,
				WM_SETTEXT,
				NULL,(LPARAM)SBTN_DISABLE);
			ui_log_add(SLOG_ENABLICS);
			gNCShared = true;
		} else
			ui_log_add(SLOG_ENABLICSF);
		break;
	case true: //disable ics
		cor_ics_disable();
		if(!cor_ics_is_enabled()) {
			EnableWindow(
				GetDlgItem(gMainUI,IDC_CMB_CONN),
				TRUE);
			SendMessage(
				hButton,
				WM_SETTEXT,
				NULL,(LPARAM)SBTN_ENABLE);
			ui_log_add(SLOG_DISABLICS);
			gNCShared = false;
		}
	}
	return;
}

void
	ui_on_btn_clr_click
	(void)
{
	ui_log_clear();
	return;
}

bool
	ui_on_initdialog
	(HWND hWnd,LPARAM lParam)
{
	int r;
	HWND hCombo;
	DWORD dwCount;
	HMENU hSysMenu;
	HICON hIcon;

	gMainUI = hWnd;
	//initialize variables
	gHNStarted = false;
	gNCShared = false;
	//initialize core
	r = cor_initialize();
	if(r != 0) {
		switch(r) {
		case 1:
			ui_msgbox(SMB_REQADMIN,MT_ERROR);
			break;
		case 2:
			ui_msgbox(SMB_REQADMIN,MT_ERROR);
			break;
		case 3:
			ui_msgbox(SMB_REQADMIN,MT_ERROR);
			break;
		case 4:
			ui_msgbox(SMB_REQADMIN,MT_ERROR);
			break;
		default:
			ui_msgbox(SMB_INITFAIL,MT_ERROR);
		}
		return false;
	}
	//ui initialize
	hSysMenu = GetSystemMenu(gMainUI,FALSE);
	if(hSysMenu) {
		AppendMenu(hSysMenu,MF_SEPARATOR,0,0);
		AppendMenu(hSysMenu,MF_STRING,IDM_ABOUT,SMENU_ABOUT);
	}
	//icon
	hIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICO));
	SendMessage(
		gMainUI,
		WM_SETICON,
		(WPARAM)TRUE,(LPARAM)hIcon);
	SendMessage(
		gMainUI,
		WM_SETICON,
		(WPARAM)FALSE,(LPARAM)hIcon);
	//edit ssid
	cor_wlan_get_ssid(sWlanSSID);
	SendDlgItemMessage(
		gMainUI,
		IDC_EDT_SSID,
		WM_SETTEXT,
		NULL,(LPARAM)sWlanSSID);
	SendDlgItemMessage(
		gMainUI,
		IDC_EDT_SSID,
		EM_LIMITTEXT,
		(WPARAM)MAX_SSID_LEN,0);
	//edit password
	cor_wlan_get_key(sWlanPasswd);
	SendDlgItemMessage(
		gMainUI,
		IDC_EDT_PASSWD,
		WM_SETTEXT,
		NULL,(LPARAM)sWlanPasswd);
	SendDlgItemMessage(
		gMainUI,
		IDC_EDT_PASSWD,
		EM_LIMITTEXT,
		(WPARAM)MAX_KEY_LEN,0);
	SendDlgItemMessage(
		gMainUI,
		IDC_EDT_PASSWD,
		EM_SETPASSWORDCHAR,
		(WPARAM)SPASSWORD_CHAR,NULL);
	//connection combox
	hCombo = GetDlgItem(gMainUI,IDC_CMB_CONN);
	dwCount = SendMessage(
		hCombo,
		CB_GETCOUNT,
		NULL,NULL);
	while(dwCount > 0) {
		SendMessage(
			hCombo,
			CB_DELETESTRING,
			(WPARAM)--dwCount,NULL);
	}
	SendMessage(
		hCombo,
		CB_ADDSTRING,
		NULL,(LPARAM)SCB_FIRST);
	SendMessage(
		hCombo,
		CB_SETCURSEL,
		(WPARAM)0,NULL);
	cor_ics_enum(ui_enum_conn,hCombo);
	gNCShared = cor_ics_is_enabled();
	if(gNCShared) {
		EnableWindow(
			GetDlgItem(gMainUI,IDC_CMB_CONN),
			FALSE);
		SendMessage(
			GetDlgItem(gMainUI,IDC_BTN_ICS),
			WM_SETTEXT,
			NULL,(LPARAM)SBTN_DISABLE);
		ui_log_add(SLOG_ICSENABED);
	}
	switch(cor_wlan_get_state()) {
	case wlan_hosted_network_active:
		ui_on_hn_started(NULL);
		ui_log_add(SLOG_HNSTARTED);
		break;
	case wlan_hosted_network_unavailable:
		ui_on_hn_unavailable(NULL);
		ui_msgbox(SMB_HN_UNAVL,MT_INFORMATION);
		break;
	case wlan_hosted_network_idle:
		ui_on_hn_stoped(NULL);
	}
	//register notification
	cor_register_notify(notify_available,ui_on_hn_active);
	cor_register_notify(notify_unavailable,ui_on_hn_unavailable);
	cor_register_notify(notify_started,ui_on_hn_started);
	cor_register_notify(notify_stoped,ui_on_hn_stoped);
	cor_register_notify(notify_join,ui_on_hn_join);
	ui_log_add(SLOG_INITSUCC);
	return true;
}

void
	ui_on_close
	(void)
{
	cor_deinitialize();
	return;
}

INT_PTR CALLBACK
	ui_dialogproc
	(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg) {
	case WM_COMMAND:
		switch((DWORD)LOWORD(wParam)) {
		case IDC_BTN_HN:
			ui_on_btn_hn_click();
			break;
		case IDC_BTN_ICS:
			ui_on_btn_ics_click();
			break;
		case IDC_LST_LOG:
			if(HIWORD(wParam) != LBN_DBLCLK)
				return FALSE;
			ui_on_lst_log_dblclick();
			break;
		case IDC_BTN_CLR:
			ui_on_btn_clr_click();
			break;
		default:
			return FALSE;
		}
		break;
	case WM_INITDIALOG:
		if(ui_on_initdialog(hWnd,lParam))
			break;
	case WM_CLOSE:
		ui_on_close();
		EndDialog(gMainUI,0);
		break;
	case WM_SYSCOMMAND:
		if(LOWORD(wParam) == IDM_ABOUT) {
			MessageBox(hWnd,SABOUT_TEXT,SABOUT_TITLE,MB_ICONINFORMATION);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

int WINAPI
	ui_build
	(void)
{
	return DialogBoxParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_WOHN),
		NULL,
		ui_dialogproc,NULL);
}

int WINAPI
	ui_cmd
	(void)
{
	return 0;
}