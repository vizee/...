//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <strsafe.h>

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
	TCHAR path[MAX_PATH];
	DWORD size = GetModuleFileName((HMODULE)hModule, path, MAX_PATH);
	if (size == 0) {
		return;
	}
	for (DWORD i = size - 1; i > 0; i--) {
		if (path[i] == TEXT('\\')) {
			path[i + 1] = TEXT('\x00');
			env_init(path);
			break;
		}
	}
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Reload"), env_load, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
static bool _inited = false;
static TCHAR _config_file[MAX_PATH];

void env_init(LPTSTR path)
{
	StringCchCopy(_config_file, MAX_PATH, path);
	StringCchCat(_config_file, MAX_PATH, TEXT("envext.ini"));
	_inited = true;
	env_load();
}

void env_load()
{
	if (!_inited) {
		return;
	}
	const int sections_size = 32767;
	TCHAR *sections = new TCHAR[sections_size];
	DWORD size = GetPrivateProfileSection(TEXT("envext"), sections, sections_size, _config_file);
	if (size == 0) {
		DWORD err = GetLastError();
		if (err != 0) {
			StringCchPrintf(sections, sections_size, TEXT("GetLastError(): %d"), err);
			MessageBox(NULL, sections, NPP_PLUGIN_NAME, MB_ICONERROR);
		}
		delete sections;
		return;
	}
	const int buffer_size = 32767;
	TCHAR *buffer = new TCHAR[buffer_size];
	for (LPTSTR s = sections; *s; s = s + lstrlen(s) + 1) {
		int l = lstrlen(s);
		for (int i = 0; i < l; i++) {
			if (s[i] == TEXT('=')) {
				if (i == 0) {
					break;
				}
				ExpandEnvironmentStrings(s + i + 1, buffer, buffer_size);
				s[i] = '\x00';
				SetEnvironmentVariable(s, buffer);
				break;
			}
		}
	}
	delete sections;
	delete buffer;
}
