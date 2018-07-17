/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef WIN32
#ifdef USE_XFIRE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX // required to stop windows.h messing up std::min
#endif //NOMINMAX
#include <windows.h>
#include <tlhelp32.h>

#include "xfiregameclient.h"

static HMODULE g_toucan_dll = NULL;
static void HelperInit();
static HMODULE HelperGetToucanDLL();

typedef int (*XfireSetCustomGameDataAFunction)(int , const char **, const char **);
typedef int (*XfireSetCustomGameDataWFunction)(int , const wchar_t **, const wchar_t **);
typedef int (*XfireSetCustomGameDataUTF8Function)(int , const char **, const char **);

static XfireSetCustomGameDataAFunction ptr_XfireSetCustomGameDataA = NULL;
static XfireSetCustomGameDataWFunction ptr_XfireSetCustomGameDataW = NULL;
static XfireSetCustomGameDataUTF8Function ptr_XfireSetCustomGameDataUTF8 = NULL;

/* make sure we are going to call the ANSI version */
#ifdef MODULEENTRY32
#undef MODULEENTRY32
#endif

#ifdef Module32First
#undef Module32First
#endif

#ifdef Module32Next
#undef Module32Next
#endif 


int XfireIsLoaded()
{
	HelperInit();
	if (ptr_XfireSetCustomGameDataA &&
		ptr_XfireSetCustomGameDataW &&
		ptr_XfireSetCustomGameDataUTF8)
		return 1;
	return 0;
}

int XfireSetCustomGameDataA(int num_keys, const char **keys, const char **values)
{
	HelperInit();
	if (ptr_XfireSetCustomGameDataA)
		return ptr_XfireSetCustomGameDataA(num_keys, keys, values);
	return 1;
}

int XfireSetCustomGameDataW(int num_keys, const wchar_t **keys, const wchar_t **values)
{
	HelperInit();
	if (ptr_XfireSetCustomGameDataW)
		return ptr_XfireSetCustomGameDataW(num_keys, keys, values);
	return 1;
}

int XfireSetCustomGameDataUTF8(int num_keys, const char **keys, const char **values)
{
	HelperInit();
	if (ptr_XfireSetCustomGameDataUTF8)
		return ptr_XfireSetCustomGameDataUTF8(num_keys, keys, values);
	return 1;
}

/* ------------------------------------------------------------------------- */
static void HelperInit()
{
	if (!ptr_XfireSetCustomGameDataA ||
		!ptr_XfireSetCustomGameDataW ||
		!ptr_XfireSetCustomGameDataUTF8)
	{
		HMODULE toucan_dll = HelperGetToucanDLL();
		if (toucan_dll)
		{
			ptr_XfireSetCustomGameDataA = (XfireSetCustomGameDataAFunction)::GetProcAddress(toucan_dll, "ToucanSendGameClientDataA_V1");
			ptr_XfireSetCustomGameDataW = (XfireSetCustomGameDataWFunction)::GetProcAddress(toucan_dll, "ToucanSendGameClientDataW_V1");
			ptr_XfireSetCustomGameDataUTF8 = (XfireSetCustomGameDataUTF8Function)::GetProcAddress(toucan_dll, "ToucanSendGameClientDataUTF8_V1");
		}
	}
}


static HMODULE HelperGetToucanDLL()
{
	if (g_toucan_dll)
		return g_toucan_dll;

	/*
	** We need to enumerate the DLLs loaded to find toucan dll.
	** This is done because the toucan dll changes with each update.
	** The toucan dll has the following format. "xfire_toucan_{BUILD_NUMBER}.dll"
	** We simply try to find a dll w/ the prefix "xfire_toucan"
	*/
	HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (snapshot_handle != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module_entry;
		module_entry.dwSize = sizeof(MODULEENTRY32); 

		BOOL result = Module32First(snapshot_handle, &module_entry);
		char module_name[] = "xfire_toucan";
		DWORD module_name_len = sizeof(module_name)-1;
		while (result)
		{
			if (CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE, module_entry.szModule, module_name_len, module_name, module_name_len) == CSTR_EQUAL)
			{
				g_toucan_dll = module_entry.hModule;
				break;
			}
			result = Module32Next(snapshot_handle, &module_entry);
		}

		CloseHandle(snapshot_handle);
	}

	return g_toucan_dll;
}

#endif //XFIRE
#endif // WIN32
