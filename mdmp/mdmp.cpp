/*
    mdmp.c - example program using libmdmp
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
#include <stdio.h>
// Platform SDK:
#include <windows.h>
#include <stdio.h>
#include <string.h>

//=== constants ==============================================================//
#define MODE_PID                1
#define MODE_PNAME              2

//=== functions ==============================================================//
int getIntArg(char *arg) {
   int j = 0;

   if (!arg[0]) return 0;
   while (arg[j] == ':' || arg[j] == '=' || arg[j] == ' ') j++;
   return atoi(&arg[j]);
   }

int main(int argc, char *argv[], char *envp[]) {
   DWORD i, j, res;
   MDMP_DUMP_REQUEST req;
   int pid;
   char *processName, *moduleName;
   
   if (!initMDmp()) {
      printf("Failed initializing MDmp!");
      return 1;
      }

   req.procSelMode = 0;
   req.dumpMode = MDMP_DUMP_MAIN_IMAGE;
   
   for (i=1; i<argc; i++) {
      if (argv[i][0] == '-' || argv[i][0] == '/') {
         switch (argv[i][1]) {
            case 'p':
               req.pid = getIntArg(&argv[i][2]);
               req.procSelMode = MDMP_SEL_BY_PID;
               break;
            }
         }
      else {
         }
      }
   if (!req.procSelMode) {
      // add description/basic help
      // [...]
      printf("No op selected!");
      return 1;
      }
   
   __debugbreak;
   res = getDumps(&req);
   if (res == MDMP_OK) {
      res = dumpToDisk(&req);
      if (res != MDMP_OK) {
         printf("dumpToDisk() failed with code [%p]!", res);
         }
      releaseReqBuffers(&req);
      }
   else {
      printf("getDumps() failed with code [%p]!", res);
      }
   
   return 0;
   }