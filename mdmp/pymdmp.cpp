/*
    pymdmp - Python wrapper for the libmdmp library
    Copyright (c) 2010 Vlad-Ioan Topan

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

//#define VIR_DEBUG

#include <libmdmp.h>

#include <python.h>
#include <stdio.h>

#ifdef UNICODE
#define DUMP_OUTPUT_TUPLE "ssIIs#"
#else
#define DUMP_OUTPUT_TUPLE "ssIIs#"
//#define DUMP_OUTPUT_TUPLE "uuIIs#"
#endif
//==============================================================================
PyObject * pyDump(PyObject *self, PyObject *args, PyObject *kargs);
PyObject * pyVer(PyObject *self);

PyMethodDef methods[] = {
    {"dump", (PyCFunction)pyDump, METH_VARARGS|METH_KEYWORDS},
    {"ver", (PyCFunction)pyVer, METH_NOARGS},
    {NULL, NULL},
    };

//------------------------------------------------------------------------------
extern "C"
void initpymdmp() {
    PyObject *m = Py_InitModule("pymdmp", methods);
    PyObject *d = PyModule_GetDict(m);

    PyDict_SetItemString(d, "DUMP_REGION", Py_BuildValue("I", MDMP_DUMP_REGION));
    PyDict_SetItemString(d, "DUMP_MAIN_IMAGE", Py_BuildValue("I", MDMP_DUMP_MAIN_IMAGE));
    PyDict_SetItemString(d, "DUMP_ALL_IMAGES", Py_BuildValue("I", MDMP_DUMP_ALL_IMAGES));
    PyDict_SetItemString(d, "DUMP_IMAGE_BY_IMAGEBASE", Py_BuildValue("I", MDMP_DUMP_IMAGE_BY_IMAGEBASE));
    PyDict_SetItemString(d, "DUMP_IMAGE_BY_NAME", Py_BuildValue("I", MDMP_DUMP_IMAGE_BY_NAME));
    PyDict_SetItemString(d, "DUMP_STACKS", Py_BuildValue("I", MDMP_DUMP_STACKS));
    PyDict_SetItemString(d, "DUMP_HEAPS", Py_BuildValue("I", MDMP_DUMP_HEAPS));
    PyDict_SetItemString(d, "DUMP_SMART", Py_BuildValue("I", MDMP_DUMP_SMART));
    PyDict_SetItemString(d, "DUMP_EXECUTABLE", Py_BuildValue("I", MDMP_DUMP_EXECUTABLE));
    PyDict_SetItemString(d, "DUMP_ALL_MEM", Py_BuildValue("I", MDMP_DUMP_ALL_MEM));

    PyDict_SetItemString(d, "SEL_BY_NAME", Py_BuildValue("I", MDMP_SEL_BY_NAME));
    PyDict_SetItemString(d, "SEL_BY_PID", Py_BuildValue("I", MDMP_SEL_BY_PID));
    PyDict_SetItemString(d, "SEL_ALL", Py_BuildValue("I", MDMP_SEL_ALL));

    PyDict_SetItemString(d, "FLAG_DONT_FIX_IMAGES", Py_BuildValue("I", MDMP_FLAG_DONT_FIX_IMAGES));
    PyDict_SetItemString(d, "FLAG_FIX_IMPORTS", Py_BuildValue("I", MDMP_FLAG_FIX_IMPORTS));
    PyDict_SetItemString(d, "FLAG_SORT_BY_ADDR", Py_BuildValue("I", MDMP_FLAG_SORT_BY_ADDR));

    PyDict_SetItemString(d, "MDMP_OK", Py_BuildValue("I", MDMP_OK));
    PyDict_SetItemString(d, "MDMP_ERR_MEM_ALLOC_FAILED", Py_BuildValue("I", MDMP_ERR_MEM_ALLOC_FAILED));
    PyDict_SetItemString(d, "MDMP_ERR_ACCESS_DENIED", Py_BuildValue("I", MDMP_ERR_ACCESS_DENIED));
    PyDict_SetItemString(d, "MDMP_ERR_PROC_SNAPSHOT_FAILED", Py_BuildValue("I", MDMP_ERR_PROC_SNAPSHOT_FAILED));
    PyDict_SetItemString(d, "MDMP_ERR_NO_PROCESS_MATCHES", Py_BuildValue("I", MDMP_ERR_NO_PROCESS_MATCHES));
    PyDict_SetItemString(d, "MDMP_ERR_READ_MEM_FAILED", Py_BuildValue("I", MDMP_ERR_READ_MEM_FAILED));
    PyDict_SetItemString(d, "MDMP_ERR_INVALID_ARGS", Py_BuildValue("I", MDMP_ERR_INVALID_ARGS));
    PyDict_SetItemString(d, "MDMP_ERR_WRITING_TO_DISK", Py_BuildValue("I", MDMP_ERR_WRITING_TO_DISK));
    PyDict_SetItemString(d, "MDMP_ERR_UNK_ERROR", Py_BuildValue("I", MDMP_ERR_UNK_ERROR));
    }

//==============================================================================
PyObject * pyVer(PyObject *self) {
    return Py_BuildValue("s", LIBMDMP_VER);
    }

PyObject * pyDump(PyObject *self, PyObject *args, PyObject *kargs) {
    // expected arguments: dump(processSelectionMode, dumpMode, flags, [optionalArgs])
    //  * processSelectionMode(integer): SEL_* constant
    //  * dumpMode(integer): DUMP_* constant
    //  * flags(integer): OR-combination of FLAG_* constants
    //  * optionalArgs:
    //    * processName(string) if processSelectionMode == SEL_BY_NAME
    //    * processPID(string) if processSelectionMode == SEL_BY_PID
    //    * moduleName(string) if dumpMode == DUMP_IMAGE_BY_NAME
    //    * moduleAddr(string) if dumpMode == DUMP_IMAGE_BY_IMAGEBASE
    //    * regStart(integer), regEnd(integer) if dumpMode == DUMP_REGION
    // Return value: if successful, list of tuples (processName, dumpName, pid, dumpAddr, dumpData);
    //               ERR_* error code otherwise
    static char *pydump_kargs[] = {"selMode", "dumpMode", "flags", "processName", "processPID", "moduleName",
                                    "moduleAddr", "regStart", "regEnd", NULL};
    MDMP_DUMP_REQUEST req;
    MDMP_REGION *crt;
    DWORD res;
    char *processName, *moduleName;
    PyObject *result = PyList_New(0);;

    ZeroMemory(&req, sizeof(req));
    PyArg_ParseTupleAndKeywords(args, kargs, "III|sIsIII", pydump_kargs, &req.procSelMode, &req.dumpMode, &req.flags, &processName,
                                &req.pid, &moduleName, &req.imageBase, &req.startAddr, &req.endAddr);

    if (req.procSelMode == MDMP_SEL_BY_NAME) {
        strcpy(req.processName, processName);
        }
    if (req.dumpMode == MDMP_DUMP_IMAGE_BY_NAME) {
        strcpy(req.moduleName, moduleName);
        }

    res = getDumps(&req);

    if (res == MDMP_OK) {
        crt = req.regionList;
        while (crt) {
            //printf("%d\n", crt->pid); // ssIIIs#
            PyList_Append(result, Py_BuildValue(DUMP_OUTPUT_TUPLE, crt->processName, crt->name ? crt->name : _T("unknown-dump"), crt->pid, crt->addr, crt->data, crt->size));
            crt = crt->next;
            }
        releaseReqBuffers(&req);
        }
    else {

        }

    return result;
    }

//=== entrypoint =============================================================//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            if (!initMDmp()) {
                return 0;
                }
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        }
    return 1;
    }
