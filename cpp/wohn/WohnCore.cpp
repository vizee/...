#include "WohnCore.h"

#define REG_NC_KEY		L"SOFTWARE\\Policies\\Microsoft\\Windows\\Network Connections"
#define REG_NC_VAL		L"NC_ShowSharedAccessUI"
#define LIB_NETSHELL	L"netshell.dll"
#define FP_NPFREE		"NcFreeNetconProperties"
#define ERR_SUCCESS(_r)	((_r) == ERROR_SUCCESS)
#define NC_SUPPORT(_np)	((_np)->MediaType == NCM_LAN && (_np)->Status != NCS_DISCONNECTED)
#define INITED(_r)		{if(gInitialized)return (_r);}
#define UNINIT(_r)		{if(!gInitialized)return (_r);}
#define CALL_S(_p)		if(_p)(_p)
#define RELEASE_S(_obj)	{if(_obj){(_obj)->Release();(_obj) = NULL;}}
#define WLFREE_S(_p)	{if(_p){WlanFreeMemory(_p);(_p) = NULL;}}
#define WLCLOSE_S(_h)	{if(_h){WlanCloseHandle((_h),NULL);(_h) = NULL;}}

typedef void (WINAPI *COR_NETCONPROPS_FREE)
	(NETCON_PROPERTIES *pNetConProps);

static bool gInitialized;
static long gNotifications;
//wlan
static CRITICAL_SECTION gCriticalSection;
static HANDLE gWlanClient;
static PWLAN_HOSTED_NETWORK_CONNECTION_SETTINGS pHNConnSettings;
static PWLAN_HOSTED_NETWORK_SECURITY_SETTINGS pHNSecSettings;
static PWLAN_HOSTED_NETWORK_STATUS pHNStatus;
static PBOOL pHNEnabled;
static COR_NOTIFY_CALLBACK gNotifyCallBack[notify_count];
//ics
static bool gCoInitialized,gICSEnabled;
static INetSharingManager *pNSM;
static INetConnection *pNCPublic,*pNCPrivate;
static COR_NETCONPROPS_FREE NetconPropsFree;

static VOID WINAPI
	cor_wlan_notification
	(PWLAN_NOTIFICATION_DATA pNotifyData,PVOID pContext)
{
	PWLAN_HOSTED_NETWORK_STATE_CHANGE pStateChange;
	PWLAN_HOSTED_NETWORK_DATA_PEER_STATE_CHANGE pPeerStateChange;

	UNINIT((VOID)false);

	if(gNotifications <= 0)
		return;
	//lock
	EnterCriticalSection(&gCriticalSection);
	if(pNotifyData) //if source come from hosted network
		if(pNotifyData->NotificationSource == WLAN_NOTIFICATION_SOURCE_HNWK) {
			//notification code
			switch(pNotifyData->NotificationCode) {
			case wlan_hosted_network_state_change://hosted network state
				if(pNotifyData->pData && pNotifyData->dwDataSize == sizeof(WLAN_HOSTED_NETWORK_STATE_CHANGE)) {
					//copy pointer
					pStateChange = (PWLAN_HOSTED_NETWORK_STATE_CHANGE)pNotifyData->pData;
					switch(pStateChange->NewState) {
					case wlan_hosted_network_active:
						CALL_S(gNotifyCallBack[notify_started])(pContext);
						break;
					case wlan_hosted_network_idle:
						if(pStateChange->OldState == wlan_hosted_network_active) {
							CALL_S(gNotifyCallBack[notify_stoped])(pContext);
						} else {
							CALL_S(gNotifyCallBack[notify_available])(pContext);
						}
						break;
					case wlan_hosted_network_unavailable:
						if(pStateChange->OldState == wlan_hosted_network_active)
							CALL_S(gNotifyCallBack[notify_stoped])(pContext);
						if(pStateChange->OldState != wlan_hosted_network_unavailable)
							CALL_S(gNotifyCallBack[notify_unavailable])(pContext);
						break;
					}
				}
				break;
			case wlan_hosted_network_peer_state_change:
				if(pNotifyData->pData && pNotifyData->dwDataSize == sizeof(WLAN_HOSTED_NETWORK_DATA_PEER_STATE_CHANGE)) {
					pPeerStateChange = (PWLAN_HOSTED_NETWORK_DATA_PEER_STATE_CHANGE)pNotifyData->pData;
					switch(pPeerStateChange->NewState.PeerAuthState) {
					case wlan_hosted_network_peer_state_authenticated:
						CALL_S(gNotifyCallBack[notify_join])((PVOID)pPeerStateChange->NewState.PeerMacAddress);
						break;
					case wlan_hosted_network_peer_state_invalid:
						CALL_S(gNotifyCallBack[notify_leave])((PVOID)pPeerStateChange->NewState.PeerMacAddress);
						break;
					case wlan_hosted_network_peer_state_change:
						CALL_S(gNotifyCallBack[notify_change])((PVOID)pPeerStateChange);
					}
				}
				break;
			}
		}
	//unlock
	LeaveCriticalSection(&gCriticalSection);
	return;
}

static void
	cor_wlan_deinitialize
	(void)
{
	UNINIT((void)false);

	if(gNotifications) {
		gNotifications = 0;
		for(int i = 0;i < notify_count;i++)
			gNotifyCallBack[i] = NULL;
	}
	WLFREE_S(pHNEnabled);
	WLFREE_S(pHNConnSettings);
	WLFREE_S(pHNSecSettings);
	WLFREE_S(pHNStatus);
	WLCLOSE_S(gWlanClient);
	DeleteCriticalSection(&gCriticalSection);
	return;
}

static bool
	cor_wlan_initialize
	(void)
{
	BOOL bResult;
	DWORD dwError,dwSize,dwWlanVersion;
	WLAN_OPCODE_VALUE_TYPE tValueType;

	INITED(false);

	InitializeCriticalSection(&gCriticalSection);
	//initialize variables
	gNotifications = 0;
	gWlanClient = NULL;
	pHNConnSettings = NULL;
	pHNSecSettings = NULL;
	pHNStatus = NULL;
	pHNEnabled = NULL;
	//open handle
	dwError = WlanOpenHandle(
		WLAN_API_VERSION,
		NULL,
		&dwWlanVersion,
		&gWlanClient);
	if(ERR_SUCCESS(dwError)) {
		bResult = FALSE;
		//initialize hosted network
		dwError = WlanHostedNetworkInitSettings(
			gWlanClient,
			NULL,
			NULL);
		if(ERR_SUCCESS(dwError)) {
			pHNEnabled = NULL;
			//query hosted network enable
			dwError = WlanHostedNetworkQueryProperty(
				gWlanClient,
				wlan_hosted_network_opcode_enable,
				&dwSize,
				(PVOID *)&pHNEnabled,
				&tValueType,
				NULL);
			if(ERR_SUCCESS(dwError))
				bResult = (pHNEnabled && dwSize == sizeof(BOOL));
		}
		if(bResult == TRUE) {
			//query status
			pHNStatus = NULL;
			dwError = WlanHostedNetworkQueryStatus(
				gWlanClient,
				&pHNStatus,
				NULL);
			bResult = (pHNStatus && ERR_SUCCESS(dwError));
		}
		if(bResult == TRUE) {
			//connection settings
			pHNConnSettings = NULL;
			dwError = WlanHostedNetworkQueryProperty(
				gWlanClient,
				wlan_hosted_network_opcode_connection_settings,
				&dwSize,
				(PVOID *)&pHNConnSettings,
				&tValueType,
				NULL);
			if(!pHNConnSettings || dwSize < sizeof(WLAN_HOSTED_NETWORK_CONNECTION_SETTINGS))
				bResult = FALSE;
			else
				bResult = ERR_SUCCESS(dwError);
		}
		if(bResult == TRUE) {
			//security settings
			pHNSecSettings = NULL;
			dwError = WlanHostedNetworkQueryProperty(
				gWlanClient,
				wlan_hosted_network_opcode_security_settings,
				&dwSize,
				(PVOID *)&pHNSecSettings,
				&tValueType,
				NULL);
			if(!pHNSecSettings || dwSize < sizeof(WLAN_HOSTED_NETWORK_SECURITY_SETTINGS))
				bResult = FALSE;
			else
				bResult = ERR_SUCCESS(dwError);
		}
		if(bResult == TRUE) {
			//register notification
			 dwError = WlanRegisterNotification(
				gWlanClient,
				WLAN_NOTIFICATION_SOURCE_HNWK,
				TRUE,
				cor_wlan_notification,
				NULL,
				NULL,
				NULL);
			 bResult = ERR_SUCCESS(dwError);
		}
		if(bResult)
			return true;
		cor_wlan_deinitialize();
	}
	return false;
}

long
	cor_wlan_get_state
	(void)
{
	DWORD dwError;

	UNINIT(wlan_hosted_network_unavailable);

	dwError = WlanHostedNetworkQueryStatus(
			gWlanClient,
			&pHNStatus,
			NULL);
	if(ERR_SUCCESS(dwError))
		return pHNStatus->HostedNetworkState;
	return wlan_hosted_network_unavailable;
}

bool
	cor_wlan_is_started
	(void)
{
	UNINIT(false);

	return (cor_wlan_get_state() == wlan_hosted_network_active);
}

int
	cor_wlan_get_ssid
	(LPWSTR sSSID)
{
	WCHAR sBuffer[WLAN_MAX_NAME_LENGTH];
	DWORD dwBufferLen,dwSSIDLen;

	UNINIT(0);
	
	dwSSIDLen = pHNConnSettings->hostedNetworkSSID.uSSIDLength;
	if(dwSSIDLen > DOT11_SSID_MAX_LENGTH)
		return -1;
	dwBufferLen = 0;
	sBuffer[0] = NULL;
	if(dwSSIDLen > 0) //try ACP conversion
		dwBufferLen = MultiByteToWideChar(
				CP_ACP,
				0,
				(LPCSTR)pHNConnSettings->hostedNetworkSSID.ucSSID,
				dwSSIDLen,
				NULL,0);
	if(dwBufferLen > 0 && dwBufferLen + 1 <= DOT11_SSID_MAX_LENGTH) {
		dwBufferLen = MultiByteToWideChar(
			CP_ACP,
			0,
			(LPCSTR)pHNConnSettings->hostedNetworkSSID.ucSSID,
			dwSSIDLen,
			sBuffer,dwBufferLen);
		if(dwBufferLen > 0)
			sBuffer[dwBufferLen++] = NULL;
		else
			sBuffer[0] = NULL;
	}
	lstrcpy(sSSID,sBuffer);
	return dwBufferLen;
}

bool
	cor_wlan_set_ssid
	(LPWSTR sSSID)
{
	DWORD dwError,dwBytes;
	UCHAR bBuffer[DOT11_SSID_MAX_LENGTH];
	DOT11_SSID tSSID;

	UNINIT(false);

	dwBytes = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		sSSID,
		-1,
		(LPSTR)bBuffer,DOT11_SSID_MAX_LENGTH,
		NULL,NULL);
	if(dwBytes > 1) {
		//backup old ssid
		RtlCopyMemory(&tSSID,&pHNConnSettings->hostedNetworkSSID,sizeof(DOT11_SSID));
		//set new ssid
		pHNConnSettings->hostedNetworkSSID.uSSIDLength = dwBytes - 1;
		RtlCopyMemory(&pHNConnSettings->hostedNetworkSSID.ucSSID,bBuffer,dwBytes - 1);
		dwError = WlanHostedNetworkSetProperty(
			gWlanClient,
			wlan_hosted_network_opcode_connection_settings,
			sizeof( WLAN_HOSTED_NETWORK_CONNECTION_SETTINGS),
			(PVOID)pHNConnSettings,
			NULL,
			NULL);
		if(ERR_SUCCESS(dwError))
			return true;
		//restore ssid if failed
		RtlCopyMemory(&pHNConnSettings->hostedNetworkSSID,&tSSID,sizeof(DOT11_SSID));
	}
	return false;
}

int
	cor_wlan_get_key
	(LPWSTR sKey)
{
	DWORD dwError;
	DWORD dwKeyLen,dwBufferLen;
	BOOL bPersistent,bIsPassPhrase;
	PUCHAR pKey;
	WCHAR sBuffer[WLAN_MAX_NAME_LENGTH];

	UNINIT(0);

	dwBufferLen = -1;
	dwKeyLen = 0;
	pKey = NULL;
	bPersistent = bIsPassPhrase = FALSE;
	dwError = WlanHostedNetworkQuerySecondaryKey(
		gWlanClient,
		&dwKeyLen,
		&pKey,
		&bIsPassPhrase,
		&bPersistent,
		NULL,
		NULL);
	if(ERR_SUCCESS(dwError) && dwKeyLen > 0 && bIsPassPhrase) {
		dwBufferLen = MultiByteToWideChar(
			CP_ACP,
			MB_ERR_INVALID_CHARS,
			(LPCSTR)pKey,
			dwKeyLen,
			sBuffer,WLAN_MAX_NAME_LENGTH);
		if(dwBufferLen >= 8 && dwBufferLen <= 63)
			lstrcpy(sKey,sBuffer);
		else
			dwBufferLen = -1;
	}
	WLFREE_S(pKey);
	return dwBufferLen;
}

bool
	cor_wlan_set_key
	(LPWSTR sKey)
{
	DWORD dwError;
	DWORD dwKeyLen,dwBufferLen;
	BOOL bUnknownChar;
	UCHAR bBuffer[64];

	UNINIT(false);

	dwKeyLen = lstrlen(sKey);
	if(dwKeyLen < 8 || dwKeyLen > 63)
		return false;
	bUnknownChar = FALSE;
	dwBufferLen = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		sKey,
		dwKeyLen,
		(LPSTR)bBuffer,
		64,
		NULL,&bUnknownChar);
	if(dwBufferLen > 0 && !bUnknownChar) {
		bBuffer[dwBufferLen++] = 0;
		dwError = WlanHostedNetworkSetSecondaryKey(
			gWlanClient,
			dwBufferLen,
			bBuffer,
			TRUE,
			TRUE,
			NULL,
			NULL);
		return ERR_SUCCESS(dwError);
	}
	return false;
}

bool
	cor_register_notify
	(COR_NOTIFY_TYPE WlanState,COR_NOTIFY_CALLBACK pNotifyCallback)
{
	UNINIT(false);

	if(WlanState >= notify_count)
		return false;
	if(gNotifyCallBack[WlanState])
		return false;
	InterlockedIncrement(&gNotifications);
	gNotifyCallBack[WlanState] = pNotifyCallback;
	return true;
}

void
	cor_unregister_notify
	(COR_NOTIFY_TYPE WlanState)
{
	UNINIT((void)false);

	if(WlanState >= notify_count || gNotifications <= 0)
		return;
	if(gNotifyCallBack[WlanState]) {
		InterlockedDecrement(&gNotifications);
		gNotifyCallBack[WlanState] = NULL;
	}
	return;
}

bool
	cor_wlan_start
	(void)
{
	DWORD dwError;

	UNINIT(false);

	dwError = WlanHostedNetworkStartUsing(
		gWlanClient,
		NULL,
		NULL);
	if(ERR_SUCCESS(dwError))
		return cor_wlan_is_started();
	return false;
}

bool
	cor_wlan_stop
	(bool bForce)
{
	DWORD dwError;

	UNINIT(false);

	
	dwError = WlanHostedNetworkStopUsing(
		gWlanClient,
		NULL,
		NULL);
	if(!ERR_SUCCESS(dwError) && bForce)
		dwError = WlanHostedNetworkForceStop(
			gWlanClient,
			NULL,
			NULL);
	if(ERR_SUCCESS(dwError))
		return !cor_wlan_is_started();
	return false;
}

static void
	cor_ics_deinitialize
	(void)
{
	UNINIT((void)false);

	gICSEnabled = false;
	RELEASE_S(pNSM);
	RELEASE_S(pNCPublic);
	RELEASE_S(pNCPrivate);
	if(gCoInitialized) {
		CoUninitialize();
		gCoInitialized = false;
	}
	return;
}

static bool
	cor_ics_initialize
	(void)
{
	HMODULE hNetShell;
	
	INITED(false);

	hNetShell = LoadLibrary(LIB_NETSHELL);
	if(hNetShell)
		NetconPropsFree = (COR_NETCONPROPS_FREE)GetProcAddress(hNetShell,FP_NPFREE);
	if(!NetconPropsFree)
		return false;
	//initialize variables
	gCoInitialized = false;
	pNSM = NULL;
	pNCPublic = NULL;
	pNCPrivate = NULL;
	//initialize COM object API
	if(CoInitialize(NULL) == S_FALSE)
		gCoInitialized = true;
	CoInitializeSecurity (NULL, -1, NULL, NULL,
			RPC_C_AUTHN_LEVEL_PKT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL, EOAC_NONE, NULL);
	//create object instance
	CoCreateInstance(
		__uuidof(NetSharingManager),
		NULL,
		CLSCTX_ALL,
		__uuidof(INetSharingManager),
		(LPVOID *)&pNSM);
	if(pNSM) {
		VARIANT_BOOL bInstalled = VARIANT_FALSE;
		pNSM->get_SharingInstalled(&bInstalled);
		if(bInstalled == VARIANT_TRUE) {
			bool bNotRelease;
			INetSharingEveryConnectionCollection *pNSECC = NULL;
			pNSM->get_EnumEveryConnection(&pNSECC);
			if(pNSECC) {
				IEnumVARIANT *pEV = NULL;
				IUnknown *pUnk = NULL;
				pNSECC->get__NewEnum(&pUnk);
				if(pUnk) {
					pUnk->QueryInterface(
						__uuidof(IEnumVARIANT),
						(void **)&pEV);
					pUnk->Release();
				}
				if(pEV) {
					VARIANT v;
					VariantInit(&v);
					while(pEV->Next(1,&v,NULL) == S_OK) {
						if(V_VT(&v) == VT_UNKNOWN) {
							INetConnection *pNC = NULL;
							V_UNKNOWN(&v)->QueryInterface(
								__uuidof(INetConnection),
								(void **)&pNC);
							if(pNC) {
								bNotRelease = false;
								NETCON_PROPERTIES *pNP = NULL;
								pNC->GetProperties(&pNP);
								if(pNP) {
									if(NC_SUPPORT(pNP)) {
										INetSharingConfiguration *pNSC = NULL;
										pNSM->get_INetSharingConfigurationForINetConnection(pNC,&pNSC);
										if(pNSC) {
											VARIANT_BOOL bEnabled = VARIANT_FALSE;
											pNSC->get_SharingEnabled(&bEnabled);
											if(bEnabled == VARIANT_TRUE) {
												SHARINGCONNECTIONTYPE SharingType;
												pNSC->get_SharingConnectionType(&SharingType);
												if(SharingType == ICSSHARINGTYPE_PRIVATE)
													pNCPrivate = pNC;
												else if(SharingType == ICSSHARINGTYPE_PUBLIC)
													pNCPublic = pNC;
												bNotRelease = true;
											}
											pNSC->Release();
										}
									}
									NetconPropsFree(pNP);
								}
								if(!bNotRelease)
									pNC->Release();
							}
						}
						VariantClear(&v);
						if(pNCPrivate && pNCPublic)
							break;
					}
					pEV->Release();
				}
				pNSECC->Release();
			}
			return true;
		}
	}
	//deinitialize if failed
	cor_ics_deinitialize();
	return false;
}

void
	cor_ics_disable
	(void)
{
	INetSharingConfiguration *pNSC;

	UNINIT((void)false);
	
	if(pNCPrivate) {
		pNSC = NULL;
		pNSM->get_INetSharingConfigurationForINetConnection(pNCPrivate,&pNSC);
		if(pNSC) {
			pNSC->DisableSharing();
			pNSC->Release();
		}
		pNCPrivate->Release();
		pNCPrivate = NULL;
	}
	if(pNCPublic) {
		pNSC = NULL;
		pNSM->get_INetSharingConfigurationForINetConnection(pNCPublic,&pNSC);
		if(pNSC) {
			pNSC->DisableSharing();
			pNSC->Release();
		}
		pNCPublic->Release();
		pNCPublic = NULL;
	}
	return;
}

bool
	cor_ics_is_enabled
	(void)
{
	return (pNCPublic && pNCPrivate);
}

bool
	cor_ics_enable
	(LPWSTR sPublicConnection)
{
	INetSharingEveryConnectionCollection *pNSECC;
	INetSharingConfiguration *pNSC;
	INetConnection *pNC;
	bool bFound;

	UNINIT(false);
	
	if(cor_wlan_get_state() == wlan_hosted_network_unavailable)
		return false;
	cor_ics_disable();
	pNSECC = NULL;
	pNSM->get_EnumEveryConnection(&pNSECC);
	if(pNSECC) {
		IEnumVARIANT *pEV = NULL;
		IUnknown *pUnk = NULL;
		pNSECC->get__NewEnum(&pUnk);
		if(pUnk) {
			pUnk->QueryInterface(__uuidof(IEnumVARIANT),(void **)&pEV);
			pUnk->Release();
		}
		if(pEV) {
			VARIANT v;
			VariantInit(&v);
			while(pEV->Next(1,&v,NULL) == S_OK) {
				if(V_VT(&v) == VT_UNKNOWN) {
					pNC = NULL;
					V_UNKNOWN(&v)->QueryInterface(__uuidof(INetConnection),(void **)&pNC);
					if(pNC) {
						bFound = false;
						NETCON_PROPERTIES *pNP = NULL;
						pNC->GetProperties(&pNP);
						if(pNP) {
							if(NC_SUPPORT(pNP)) {
								if(!pNCPrivate && RtlEqualMemory(&pNP->guidId,&pHNStatus->IPDeviceID,sizeof(GUID))) {
									pNSC = NULL;
									pNSM->get_INetSharingConfigurationForINetConnection(pNC,&pNSC);
									if(pNSC) {
										bFound = SUCCEEDED(pNSC->EnableSharing(ICSSHARINGTYPE_PRIVATE));
										pNSC->Release();
									}
									if(bFound)
										pNCPrivate = pNC;
								} else if(!pNCPublic && !lstrcmp(pNP->pszwName,sPublicConnection)) {
									pNSC = NULL;
									pNSM->get_INetSharingConfigurationForINetConnection(pNC,&pNSC);
									if(pNSC) {
										bFound = SUCCEEDED(pNSC->EnableSharing(ICSSHARINGTYPE_PUBLIC));
										pNSC->Release();
									}
									if(bFound)
										pNCPublic = pNC;
								}
							}
							NetconPropsFree(pNP);
						}
						if(!bFound)
							pNC->Release();
					}
				}
				VariantClear(&v);
				if(pNCPrivate && pNCPublic)
					break;
			}
			pEV->Release();
		}
		pNSECC->Release();
	}
	bFound = cor_ics_is_enabled();
	if(!bFound)
		cor_ics_disable();
	return bFound;
}

bool
	cor_ics_enum
	(COR_ENUM_CONN_CALLBACK cor_enum_conn,LPVOID lpContext)
{
	int r;
	INetSharingEveryConnectionCollection *pNSECC;

	UNINIT(false);

	r = -1;
	pNSECC = NULL;
	pNSM->get_EnumEveryConnection(&pNSECC);
	if(pNSECC) {
		IEnumVARIANT *pEV = NULL;
		IUnknown *pUnk = NULL;
		pNSECC->get__NewEnum(&pUnk);
		if(pUnk) {
			pUnk->QueryInterface(__uuidof(IEnumVARIANT),(void **)&pEV);
			pUnk->Release();
		}
		if(pEV) {
			VARIANT v;
			VariantInit(&v);
			while(pEV->Next(1,&v,NULL) == S_OK) {
				if(V_VT(&v) == VT_UNKNOWN) {
					INetConnection *pNC = NULL;
					V_UNKNOWN(&v)->QueryInterface(__uuidof(INetConnection),(void **)&pNC);
					if(pNC) {
						NETCON_PROPERTIES *pNP = NULL;
						pNC->GetProperties(&pNP);
						if(pNP) {
							if(NC_SUPPORT(pNP) && memcmp(&pNP->guidId,&pHNStatus->IPDeviceID,sizeof(GUID)))
								r = cor_enum_conn(pNP,lpContext);
							NetconPropsFree(pNP);
						}
						pNC->Release();
					}
				}
				VariantClear(&v);
				if(r > 0)
					break;
			}
			pEV->Release();
		}
		pNSECC->Release();
	}
	return r >= 0;
}

static bool
	cor_is_administrator
	(void)
{
	BOOL bIsAdministrator;
	SID_IDENTIFIER_AUTHORITY tNtAuthority = SECURITY_NT_AUTHORITY;
	PSID adminsGroup;

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

static bool
	cor_is_ics_allowed
	(void)
{
	HKEY hRegKey;
	DWORD dwError;
	DWORD dwICSAllowed,dwValLen;

	dwICSAllowed = 1;
	dwError = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REG_NC_KEY,
		0,
		KEY_READ,
		&hRegKey);
	if(dwError == ERROR_SUCCESS && hRegKey) {
		dwValLen = sizeof(DWORD);
		RegGetValue(
			hRegKey,
			NULL,
			REG_NC_VAL,
			RRF_RT_DWORD,
			NULL,
			&dwICSAllowed,
			&dwValLen);
		RegCloseKey(hRegKey);
	}
	return (dwICSAllowed == 1);
}

int
	cor_initialize
	(void)
{
	INITED(-1);

	//initialize variables
	gInitialized = false;
	//check user privilege
	if(!cor_is_administrator())
		return 1;
	//check registry configuration
	if(!cor_is_ics_allowed())
		return 2;
	//initialize ics
	if(!cor_ics_initialize())
		return 3;
	//initialize wlan(hosted network)
	if(!cor_wlan_initialize())
		return 4;
	gInitialized = true;
	return 0;
}

void
	cor_deinitialize
	(void)
{
	cor_ics_deinitialize();
	cor_wlan_deinitialize();
	gInitialized = false;
	return;
}
