#ifndef WOHN_DEF_H
#define WOHN_DEF_H

//text
#define SPASSWORD_CHAR	L'¡ñ'
//log
#define SLOG_FORMAT		L"[%04d/%02d/%02d %02d:%02d:%02d]%ws"
#define SLOG_UNKNOWN	L"Unknown error!"
#define SLOG_ICSENABED	L"ICS has already been enabled."
#define SLOG_HNSTARTED	L"Wireless hosted network has already started"
#define SLOG_INITSUCC	L"Initialize successfully"
#define SLOG_ENABLICS	L"Enable ICS successfully"
#define SLOG_ENABLICSF	L"Failed to enable ICS"
#define SLOG_DISABLICS	L"ICS is disabled"
#define SLOG_STARTWLAN	L"Started using hosted Network."
#define SLOG_STOPWLAN	L"Stopped using hosted network."
#define SLOG_WLANUNAVL	L"Hosted network is not available on current NIC(s)."
#define SLOG_WLANACTED	L"Hosted network is available on current NIC(s)."
#define SLOG_WLANSTRAD	L"Hosted network is now started."
#define SLOG_WLANSTOPD	L"Hosted network is now stopped."
#define SLOG_WLANJOIN	L"New device join:\"%02X:%02X:%02X:%02X:%02X:%02X\""
//menu item
#define SMENU_ABOUT		L"About Wohn"
//about box of menu
#define SABOUT_TEXT		L"WiFi on Hosted Network(build:1211)\n\n"\
						L"OS Request() {\n"\
						L"\tVersion\t\t= Windows 7 or later;\n"\
						L"\tWirelessNIC\t= exist;\n"\
						L"\tICS\t\t= enable;\n"\
						L"\tHostedNetwork\t= enable;\n"\
						L"\treturn;\n"\
						L"}"
#define SABOUT_TITLE	L"Wohn - vizee"

//messagebox text
#define SMB_INITFAIL	L"Initialize failed"
#define SMB_REQADMIN	L"Missing administrator privilege"
#define SMB_ICSDALLW	L"ICS is disabled"
#define SMB_ICSINITF	L"Failed to initialize ICS"
#define SMB_WLANFAIL	L"Failed to initialize Wireless Hosted Network"
#define SMB_SSID_LEN	L"SSID contains 1 ~ 32 case-sensitive characters."
#define SMB_PWD_LEN		L"Password contains 8 ~ 63 case-sensitive characters."
#define SMB_CMB_INV		L"Choose a connection which connects network"
#define SMB_SSID_NOUPD	L"SSID cannot be updates"
#define SMB_PWD_NOUPD	L"Password cannot be updates"
#define SMB_HN_STARTF	L"Failed to start wlan"
#define SMB_HN_UNAVL	L"Wireless hosted network has been unavailabled\n"\
						L"Run \"netsh\" to enable it if needed, Command:\n"\
						L"\t\"netsh wlan set hostednetwork mode=allow\""
//messagebox title
#define SMB_DEFAULT		L"Wohn"
#define SMB_ERROR		L"Error"
#define SMB_WARNING		L"Warning"
#define SMB_INFORMATION	L"Information"
#define SMB_PASSWORD	L"Password"

//UI controls text
#define SCB_FIRST		L"Choose a connection"
#define SBTN_START		L"Start"
#define SBTN_STOP		L"Stop"
#define SBTN_ENABLE		L"Enable"
#define SBTN_DISABLE	L"Disable"
#endif
