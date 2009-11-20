/*
    libMDmp - main MDmp library
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
// C:
#include <tchar.h>
// Platform SDK:
#include <tlhelp32.h>
#include <psapi.h>
//#include <winnt.h>

//===  globals ===============================================================//
typedef int (__cdecl *sprintf_template)(char *, const char *, ...);
typedef int (__cdecl *swprintf_template)(WCHAR *, const WCHAR *, ...);
sprintf_template ntdll_sprintf;
swprintf_template ntdll_swprintf;
HINSTANCE hNtDll;

#ifdef UNICODE
#define _sprintf ntdll_swprintf
#else // UNICODE
#define _sprintf ntdll_sprintf
#endif // UNICODE

//=== functions ==============================================================//

//--- auxilliary functions ---------------------------------------------------//
void *getMem(int size) {
   return LocalAlloc(0, size);
   }

BOOL freeMem(void *ptr) {
   return LocalFree(ptr) == 0;
   }

int adjustPrivileges() {
   // aquire SeDebugPrivilege (required for access to other processes' memory)
   DWORD ignore;
   DWORD (__stdcall *RtlAdjustPrivilege)(DWORD, DWORD, DWORD, PVOID);

   *(FARPROC *)&RtlAdjustPrivilege = GetProcAddress(hNtDll, "RtlAdjustPrivilege");
   if (RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, 1, 0, &ignore)) return 0;
   return 1;
   }

// TCHAR string functions (avoid using C .lib too keep code size down)
int _startsWith(const TCHAR* s1, const TCHAR* s2) {
   // checks if s1 starts with s2
   int i = 0;
   while (1) {
       if (!s2[i]) return 1;
       if (s1[i] != s2[i]) return 0;
       i++;
       }
   return 0; // should never be reached; keeps compiler from complaining
   }

void _lower(TCHAR* dest, const TCHAR* src, DWORD maxSize) {
   // put lowercase version of src in dest (dst must be preallocated of at least same size as src)
   DWORD i = 0;
   while (src[i] && (i < maxSize)) {
       dest[i] = ((TCHAR('A') <= src[i]) && (TCHAR('Z') >= src[i])) ? (src[i] - TCHAR('A') + TCHAR('a'))  : src[i];
       i++;
       }
   if (i == maxSize) i--;
   dest[i] = 0;
   }

void _copy(TCHAR* dest, const TCHAR* src, DWORD maxSize) {
   // strncpy for TCHAR (avoid using C .lib too keep code size down)
   DWORD i = 0;
   while (src[i] && (i < maxSize)) {
       dest[i] = src[i];
       i++;
       }
   if (i == maxSize) i--;
   dest[i] = 0;
   }

int _length(const TCHAR* string) {
   // counts TCHARs in string up to and including the final \0
   int i = 0;
   while (string[i]) i++;
   return i+1;
   }
   
int _isSubString(const TCHAR* substring, const TCHAR* string) {
   //
   int i, result;
   TCHAR *substrLow, *strLow;
   if (!substring || !string) return 0;
   substrLow = (TCHAR*)getMem(sizeof(TCHAR)*_length(substring));
   if (!substrLow) return 0;
   strLow = (TCHAR*)getMem(sizeof(TCHAR)*_length(string));
   if (!strLow) {
      LocalFree(substrLow);
      return 0;
      }
   _lower(substrLow, substring, 0xFFFFFFFF);
   _lower(strLow, string, 0xFFFFFFFF);
   result = 0;
   for (i=0; 1; i++) {
      if (!strLow[i]) break;
      if (_startsWith(&strLow[i], substrLow)) {
         result = 1;
         break;
         }
      }
   LocalFree(substrLow);
   LocalFree(strLow);
   return result;
   }
   
void _addDump(struct MDMP_DUMP_REQUEST *req, DWORD addr, DWORD size, HANDLE hProcess, TCHAR *name, BYTE *data) {
   MDMP_REGION *region, *crt;
   TCHAR processName[MAX_PATH];
   
   processName[0] = 0;
   region = (MDMP_REGION*)getMem(sizeof(MDMP_REGION));
   if (!region) {
      req->warnings |= MDMP_WARN_MEM_ALLOC_FAILED;
      return;
      }
   region->next = 0;
   crt = req->regionList;
   if (crt) {
      while (crt->next) crt = crt->next;
      crt->next = region;
      }
   else req->regionList = region;
   region->addr = addr;
   region->size = size;

   GetModuleBaseName(hProcess, 0, processName, MAX_PATH-1);
   _lower(region->processName, processName, 32);
   if (name) _copy(region->name, name, 32);
   else region->name[0] = 0;
   region->data = data;
   }
   
void _dumpImage(struct MDMP_DUMP_REQUEST *req, HANDLE hProcess, HMODULE hModule) {
   MODULEINFO modInfo;
   TCHAR moduleName[MAX_PATH];
   DWORD size;
   BYTE *buf;
   
   moduleName[0] = 0;
   if (GetModuleInformation(hProcess, hModule, &modInfo, sizeof(modInfo))) {
      size = modInfo.SizeOfImage;
      buf = (BYTE*)getMem(size);
      if (!buf) {
         req->warnings |= MDMP_WARN_MEM_ALLOC_FAILED;
         return;
         }
      if (ReadProcessMemory(hProcess, modInfo.lpBaseOfDll, buf, size, &size)) {
         GetModuleBaseName(hProcess, hModule, moduleName, MAX_PATH-1);
         _addDump(req, (DWORD)modInfo.lpBaseOfDll, size, hProcess, moduleName, buf);
         }
      else freeMem(buf);
      }
   else req->warnings |= MDMP_WARN_QUERY_INFO_FAILED;
   }

//--- API --------------------------------------------------------------------//
int __stdcall initMDmp() {
   // libMDmp initialization function; must be called before any other API
   // returns 1 on success, 0 on fail
   hNtDll = GetModuleHandle("ntdll.dll"); // always loaded
   if (!hNtDll) return 0;
#ifdef UNICODE
   *(FARPROC*)&ntdll_swprintf = GetProcAddress(hNtDll, "swprintf");
   if (!ntdll_swprintf) return 0;
#else // UNICODE
   *(FARPROC*)&ntdll_sprintf = GetProcAddress(hNtDll, "sprintf");
   if (!ntdll_sprintf) return 0;
#endif // UNICODE

   if (!adjustPrivileges()) return 0;
   return 1;
   }

DWORD __stdcall getDumps(struct MDMP_DUMP_REQUEST *req) {
   // main workhorse; see help for details
   DWORD i, size, npids, pids[MAX_PROCESSES];
   HMODULE *handles;
   HANDLE snap, ph;
   PROCESSENTRY32 pe32;
   MODULEINFO modInf;
   
   // init
   req->regionList = 0;
   req->warnings = 0;
   
   // select processes
   if (req->procSelMode == MDMP_SEL_BY_PID) { // single PID
      pids[0] = req->pid;
      npids = 1;
      }
   else { // enumerate processes to select PIDs
      npids = 0;
      snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
      if (snap == INVALID_HANDLE_VALUE) return MDMP_ERR_PROC_SNAPSHOT_FAILED;
      pe32.dwSize = sizeof(PROCESSENTRY32);
      if (!Process32First(snap, &pe32)) {
         CloseHandle(snap);
         return MDMP_ERR_PROC_SNAPSHOT_FAILED;
         }
      do {
         if (req->procSelMode == MDMP_SEL_ALL || (req->procSelMode == MDMP_SEL_BY_NAME && _isSubString(req->processName, pe32.szExeFile)))
            pids[npids++] = pe32.th32ProcessID;
            if (npids == MAX_PROCESSES) break;
         }
      while (Process32Next(snap, &pe32));
      CloseHandle(snap);
      }
   if (!npids) return MDMP_ERR_NO_PROCESS_MATCHES;

   // walk selected processes
   for (i=0; i<npids; i++) {
      ph = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_OPERATION, 0, pids[i]);
      if (!ph) {
         req->warnings |= MDMP_WARN_OPEN_PROCESS_FAILED;
         continue;
         }

      // suspend non-suspended process threads
      // [...]

      // get dump
      switch (req->dumpMode) {
         case MDMP_DUMP_MAIN_IMAGE:
         case MDMP_DUMP_ALL_IMAGES:
         case MDMP_DUMP_IMAGE_BY_IMAGEBASE:
         case MDMP_DUMP_IMAGE_BY_NAME:
            // dump image(s)
            EnumProcessModules(ph, 0, 0, &size);
            if ((size > 0) && (size <= 8192)) {
               handles = (HMODULE*)getMem(size);
               if (handles) {
                  if (EnumProcessModules(ph, handles, size, &size)) {
                     switch (req->dumpMode) {
                        case MDMP_DUMP_MAIN_IMAGE:
                           _dumpImage(req, ph, handles[0]);
                        }
                     }
                  else req->warnings |= MDMP_WARN_QUERY_INFO_FAILED;
                  freeMem(handles);
                  }
               else req->warnings |= MDMP_WARN_MEM_ALLOC_FAILED;
               }
            break;
         }
         
      // resume suspended threads
      // [...]
      CloseHandle(ph);
      }
   return MDMP_OK;
   }
   
void __stdcall releaseReqBuffers(struct MDMP_DUMP_REQUEST *req) {
   MDMP_REGION *crt, *next;
   crt = req->regionList;
   while (crt) {
      next = crt->next;
      freeMem(crt->data);
      freeMem(crt);
      crt = next;
      }
   }

TCHAR __stdcall processName(DWORD pid) {
   return NULL;
   }
   
DWORD __stdcall dumpToDisk(struct MDMP_DUMP_REQUEST *req) {
   TCHAR fileName[MAX_PATH];
   MDMP_REGION *crt;
   HANDLE fh;
   DWORD written;

   if (!req) return MDMP_ERR_INVALID_ARGS;
   crt = req->regionList;
   while (crt) {
      _sprintf(fileName, _T("%s(%d)-%s-%p-%p.dump"), crt->processName, crt->pid, crt->name ? crt->name : _T("main-module"), crt->addr, crt->size);
      fh = CreateFile(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (fh != INVALID_HANDLE_VALUE) {
         WriteFile(fh, crt->data, crt->size, &written, 0);
         CloseHandle(fh);
         }
      else return MDMP_ERR_WRITING_TO_DISK;
      crt = crt->next;
      }
   return MDMP_OK;
   }

DWORD __stdcall dumpPIDToFile(DWORD pid, struct MDMP_DUMP_REQUEST *req) {
   if (!req) return MDMP_ERR_INVALID_ARGS;
   return MDMP_OK;
   }

//=== entrypoint =============================================================//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
   switch (ul_reason_for_call) {
      case DLL_PROCESS_ATTACH:
         if (!initMDmp()) return 0;
         break;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
         break;
      }
   return 1;
   }