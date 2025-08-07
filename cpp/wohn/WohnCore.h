#ifndef WOHN_CORE_H
#define WOHN_CORE_H

#include <Windows.h>
#include <ObjBase.h>
#include <NetCon.h>
#include <wlanapi.h>

#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"oleaut32.lib")
#pragma comment(lib,"wlanapi.lib")

typedef enum _COR_NOTIFY_TYPE {
	notify_unavailable,
	notify_available,
	notify_started,
	notify_stoped,
	notify_join,
	notify_leave,
	notify_change,
	notify_count
} COR_NOTIFY_TYPE;

typedef void (CALLBACK *COR_NOTIFY_CALLBACK)
	(LPVOID pContext);

typedef int (CALLBACK *COR_ENUM_CONN_CALLBACK)
	(NETCON_PROPERTIES *pNetconProps,LPVOID lpContext);

int cor_initialize(void);
void cor_deinitialize(void);
long cor_wlan_get_state(void);
bool cor_wlan_is_started(void);
bool cor_wlan_start(void);
bool cor_wlan_stop(bool bForce);
bool cor_register_notify(COR_NOTIFY_TYPE WlanState,COR_NOTIFY_CALLBACK pNotifyCallback);
void cor_unregister_notify(COR_NOTIFY_TYPE WlanState);
int	cor_wlan_get_key(LPWSTR sKey);
bool cor_wlan_set_key(LPWSTR sKey);
int cor_wlan_get_ssid(LPWSTR sSSID);
bool cor_wlan_set_ssid(LPWSTR sSSID);
bool cor_ics_enum(COR_ENUM_CONN_CALLBACK cor_enum_conn,LPVOID lpContext);
bool cor_ics_enable(LPWSTR sPublicConnection);
void cor_ics_disable(void);
bool cor_ics_is_enabled(void);

#endif
