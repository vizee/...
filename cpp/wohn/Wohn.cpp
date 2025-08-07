//Wifi on Hosted Network
//entry point of source
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")
#include "WohnUI.h"

int WINAPI
	WinMain
	(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	InitCommonControls();
	return ui_build();
}
