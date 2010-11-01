/*
    mdmp.c - example program using libmdmp
    Copyright (c) 2009-2010 Vlad-Ioan Topan

    author:           Vlad-Ioan Topan (vtopan / gmail.com)
    file version:     0.2.4 (BETA)
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
#include <libmdmp.h>
// C/C++:
#include <stdio.h>
#include <string.h>

//=== functions ==============================================================//
int getHexArg(char *arg) {
    int j = 0;

    while (arg[j] && (arg[j] == ':' || arg[j] == '=' || arg[j] == ' ')) {
        j++;
        }
    return strtol(&arg[j], NULL, 16);
    }

int getIntArg(char *arg) {
    int j = 0;

    while (arg[j] && (arg[j] == ':' || arg[j] == '=' || arg[j] == ' ')) {
        j++;
        }
    return atoi(&arg[j]);
    }

char *getStrArg(char *arg) {
    int j = 0;

    while (arg[j] && (arg[j] == ':' || arg[j] == '=' || arg[j] == ' ')) {
        j++;
        }
    return &arg[j];
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

    ZeroMemory(&req, sizeof(req));
    req.procSelMode = MDMP_SEL_ALL;
    req.dumpMode = MDMP_DUMP_MAIN_IMAGE;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (argv[i][1]) {
                case 'p':
                    req.pid = getIntArg(&argv[i][2]);
                    req.procSelMode = MDMP_SEL_BY_PID;
                    break;
                case 'a':
                    req.procSelMode = MDMP_SEL_ALL;
                    break;
                case 'n':
                    strcpy(req.processName, getStrArg(&argv[i][2]));
                    req.procSelMode = MDMP_SEL_BY_NAME;
                    break;
                case 'm':
                    req.dumpMode = MDMP_DUMP_SMART;
                    break;
                case 'M':
                    req.dumpMode = MDMP_DUMP_ALL_MEM;
                    break;
                case 'x':
                    req.dumpMode = MDMP_DUMP_ALL_IMAGES;
                    break;
                case 'e':
                    strcpy(req.moduleName, getStrArg(&argv[i][2]));
                    req.dumpMode = MDMP_DUMP_IMAGE_BY_NAME;
                    break;
                case 'b':
                    req.imageBase = getHexArg(&argv[i][2]);
                    req.dumpMode = MDMP_DUMP_IMAGE_BY_IMAGEBASE;
                    break;
                case 'r':
                    // parse args
                    // [...]
                    req.dumpMode = MDMP_DUMP_REGION;
                    break;
                case 'k':
                    req.dumpMode = MDMP_DUMP_STACKS;
                    break;
                case 'h':
                    req.dumpMode = MDMP_DUMP_HEAPS;
                    break;
                case 'X':
                    req.dumpMode = MDMP_DUMP_EXECUTABLE;
                    break;
                case 'F':
                    req.flags |= MDMP_FLAG_DONT_FIX_IMAGES;
                    break;
                case 'I':
                    req.flags |= MDMP_FLAG_FIX_IMPORTS;
                    break;
                case 'A':
                    req.flags |= MDMP_FLAG_SORT_BY_ADDR;
                    break;
                }
            }
        else {
            }
        }
    if (argc < 2) {
        printf("MDmp %s: libmdmp-based process memory dumper\n(c) Vlad-Ioan Topan (vtopan@gmail.com)\n", LIBMDMP_VER);

        printf("\nProcess selection:\n\
/a          dump from all processes (default)\n\
/p:###      by PID (dump from process with PID = ### (decimal))\n\
/n:###      by name (dump from process with image name containing \"###\")\n\n\
Dump target selection:\n\
default:    main executable image(s)\n\
/m          all the memory from the selected process(es) - smart (recognize images, stacks, heaps)\n\
/M          all the memory from the selected process(es) - direct\n\
/x          all executable images\n\
/e:###      executable image(s) containing \"###\" in the name\n\
/b:###      executable image(s) with imagebase = ### (hex)\n\
/r:###:$$$  memory region: $$$ (hex) bytes from address ### (hex)\n\
/k          all stacks\n\
/h          all heaps\n\
/X          all areas with eXecutable attribute set\n\n\
Options:\n\
/F          DON'T fix image dumps\n\
/I          fix imports\n\
/A          sort dumped file names by address\n\n\
Notes:\n\
 - at least one of target or process selection required\n\
 - \"/\" can be replaced with \"-\"\n\
");
        return 1;
        }

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
