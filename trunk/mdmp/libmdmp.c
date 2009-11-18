/*
    libMDMP - main MDmp library
    Copyright (c) 2009 Vlad-Ioan Topan

    author:           Vlad-Ioan Topan (vtopan / gmail.com)
    file version:     0.1 (ALPHA)
    web:              http://code.google.com/p/mdmp/

    This file is part of MDmp.

    MDmp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

//=== include ================================================================//
// local:
#include "libmdmp.h"
// C/C++:
// Platform SDK:
#include <windows.h>
//#include <winnt.h>
//#include <psapi.h>

//=== functions ==============================================================//
int adjustPrivileges() {
   // aquire SeDebugPrivilege (required for access to other processes' memory)
   HINSTANCE hNtDll;
   DWORD ignore;
   DWORD (__stdcall *RtlAdjustPrivilege)(DWORD, DWORD, DWORD, PVOID);

   hNtDll = GetModuleHandle("ntdll.dll"); // always loaded
   *(FARPROC *)&RtlAdjustPrivilege = GetProcAddress(hNtDll, "RtlAdjustPrivilege");
   if (RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, 1, 0, &ignore)) return 0;
   }

int __stdcall initMDMP() {
   // libMDmp initialization function; must be called before any other API
   // returns 1 on success, 0 on fail
   if (!adjustPrivileges()) return 0;
   }
   
//=== entrypoint =============================================================//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
         if (!initMDMP()) return 0;
         break;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
         break;
      }
   return 1;
   }
