/*
    libMDmp - main MDmp library - header file
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

#ifndef LIBMDMP_H
#define LIBMDMP_H

// C:
#include <tchar.h>
// Platform SDK:
#pragma warning(disable:4005) // avoid "macro redefinition" warning in sal.h
#include <windows.h>
#pragma warning(default:4005)

//=== macros =================================================================//
#define ALIGN_ADDR(addr, alignment) (addr % alignment) ? (addr + alignment - (addr % alignment)) : (addr)

//=== constants ==============================================================//
//--- Windows API ------------------------------------------------------------//
#define SE_DEBUG_PRIVILEGE                20

//--- MDmp API ---------------------------------------------------------------//
#define MDMP_DONT_IDENTIFY_IMAGES         0x00000001

#define MDMP_OK                           0x00000000
#define MDMP_ERR_MEM_ALLOC_FAILED         0x00010000
#define MDMP_ERR_ACCESS_DENIED            0x00010001
#define MDMP_ERR_PROC_SNAPSHOT_FAILED     0x00010003
#define MDMP_ERR_NO_PROCESS_MATCHES       0x00010004
#define MDMP_ERR_INVALID_ARGS             0x00020000
#define MDMP_ERR_WRITING_TO_DISK          0x00030000
#define MDMP_ERR_UNK_ERROR                0x000FFFFF

#define MDMP_WARN_OPEN_PROCESS_FAILED     0x00000001
#define MDMP_WARN_QUERY_INFO_FAILED       0x00000002
#define MDMP_WARN_MEM_ALLOC_FAILED        0x00000004
#define MDMP_WARN_READ_MEM_FAILED         0x00000008

#define MDMP_DUMP_REGION                  0x00000001
#define MDMP_DUMP_MAIN_IMAGE              0x00000002
#define MDMP_DUMP_ALL_IMAGES              0x00000003
#define MDMP_DUMP_IMAGE_BY_IMAGEBASE      0x00000004
#define MDMP_DUMP_IMAGE_BY_NAME           0x00000005
#define MDMP_DUMP_STACKS                  0x00000006
#define MDMP_DUMP_HEAPS                   0x00000007
#define MDMP_DUMP_ALL_MEM                 0x000000FF

#define MDMP_SEL_BY_NAME                  0x00000001
#define MDMP_SEL_BY_PID                   0x00000002
#define MDMP_SEL_ALL                      0x00000003

#define MDMP_RT_IMAGE                     0x00000001
#define MDMP_RT_UNKNOWN                   0x000000FF

//--- other ------------------------------------------------------------------//
#define MAX_PROCESSES                     256

//=== data types =============================================================//
struct MDMP_REGION {
   DWORD addr, pid, size;
   BYTE *data;
   TCHAR name[32], processName[32];
   struct MDMP_REGION *next;
   };

struct MDMP_DUMP_REQUEST {
   DWORD dumpMode, procSelMode;
   DWORD imageBase, startAddr, endAddr;
   DWORD pid;
   DWORD flags;
   TCHAR processName[32], moduleName[32];
   struct MDMP_REGION *regionList;
   DWORD warnings;
   };

//=== APIs ===================================================================//
int __stdcall initMDmp(); // libMDmp initialization function; returns 1 on success, 0 on fail

DWORD __stdcall getDumps(MDMP_DUMP_REQUEST *req);
void __stdcall releaseReqBuffers(MDMP_DUMP_REQUEST *req);
DWORD __stdcall dumpToDisk(struct MDMP_DUMP_REQUEST *req);

TCHAR __stdcall processName(DWORD pid);

#endif // LIBMDMP_H