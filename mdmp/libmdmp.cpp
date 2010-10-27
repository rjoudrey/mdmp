/*
    libMDmp - main MDmp library
    Copyright (c) 2009-2010 Vlad-Ioan Topan

    author:           Vlad-Ioan Topan (vtopan / gmail.com)
    file version:     0.2.0 (ALPHA)
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
#include "libmdmp.h"

#include <stdio.h> // "debugging"; comment out for release

//===  globals ===============================================================//
typedef int (__cdecl *sprintf_template)(char *, const char *, ...);
typedef int (__cdecl *swprintf_template)(WCHAR *, const WCHAR *, ...);
sprintf_template ntdll_sprintf;
swprintf_template ntdll_swprintf;
HINSTANCE hNtDll;
SYSTEM_INFO systemInfo;

#ifdef UNICODE
#define _sprintf ntdll_swprintf
#else // UNICODE
#define _sprintf ntdll_sprintf
#endif // UNICODE

#define ALIGN_VALUE(x, alignment)	(((x) % (alignment) == 0) ? (x) : ((x) / (alignment) + 1) * (alignment))

DWORD (__stdcall * RtlAdjustPrivilege)(DWORD, DWORD, DWORD, PVOID);
DWORD (__stdcall * NtQueryInformationThread_)(HANDLE, THREAD_INFORMATION_CLASS, PVOID, DWORD, PDWORD);
DWORD (__stdcall * NtQueryInformationProcess_)(HANDLE, PROCESS_INFORMATION_CLASS, PVOID, DWORD, PDWORD);

TCHAR *UNKNOWN_TEXT = _T("[unknown]");

//=== functions ==============================================================//

//--- auxilliary functions ---------------------------------------------------//
void *getMem(size_t size) {
    return LocalAlloc(0, size);
    //return VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }

BOOL freeMem(void *ptr) {
    return LocalFree(ptr) == 0;
    //return VirtualFree(ptr, 0, MEM_RELEASE);
    }

int adjustPrivileges() {
    // aquire SeDebugPrivilege (required for access to other processes' memory)
    DWORD prevState;

    //DEBUGBREAK;
    if (RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, 1, 0, &prevState)) {
        return 0;
        }
    return 1;
    }

// TCHAR string functions (avoid using C .lib too keep code size down)
int _startsWith(const TCHAR *s1, const TCHAR *s2) {
    // checks if s1 starts with s2
    int i = 0;
    while (1) {
        if (!s2[i]) {
            return 1;
            }
        if (s1[i] != s2[i]) {
            return 0;
            }
        i++;
        }
    return 0; // should never be reached; keeps compiler from complaining
    }

void _lower(TCHAR *dest, const TCHAR *src, DWORD maxSize) {
    // put lowercase version of src in dest (dst must be preallocated of at least same size as src)
    DWORD i = 0;
    while (src[i] && (i < maxSize)) {
        dest[i] = ((TCHAR('A') <= src[i]) && (TCHAR('Z') >= src[i])) ? (src[i] - TCHAR('A') + TCHAR('a'))  : src[i];
        i++;
        }
    if (i == maxSize) {
        i--;
        }
    dest[i] = 0;
    }

void _copy(TCHAR *dest, const TCHAR *src, DWORD maxSize) {
    // strncpy for TCHAR (avoid using C .lib too keep code size down)
    DWORD i = 0;
    while (src[i] && (i < maxSize)) {
        dest[i] = src[i];
        i++;
        }
    if (i == maxSize) {
        i--;
        }
    dest[i] = 0;
    }

int _length(const TCHAR *string) {
    // counts TCHARs in string up to and including the final \0
    int i = 0;
    while (string[i]) {
        i++;
        }
    return i + 1;
    }

int _isSubString(const TCHAR *substring, const TCHAR *string) {
    //
    int i, result;
    TCHAR *substrLow, *strLow;
    if (!substring || !string) {
        return 0;
        }
    substrLow = (TCHAR *)getMem(sizeof(TCHAR) * _length(substring));
    if (!substrLow) {
        return 0;
        }
    strLow = (TCHAR *)getMem(sizeof(TCHAR) * _length(string));
    if (!strLow) {
        freeMem(substrLow);
        return 0;
        }
    _lower(substrLow, substring, 0xFFFFFFFF);
    _lower(strLow, string, 0xFFFFFFFF);
    result = 0;
    for (i = 0; 1; i++) {
        if (!strLow[i]) {
            break;
            }
        if (_startsWith(&strLow[i], substrLow)) {
            result = 1;
            break;
            }
        }
    freeMem(substrLow);
    freeMem(strLow);
    return result;
    }

//--- internal core functions ------------------------------------------------//
size_t toggleProcessState(DWORD pid, BOOL suspend) {
    // if (suspend) suspendprocess() else resumeprocess()
    HANDLE snap, hThread;
    BOOL ok;
    THREADENTRY32 thread;

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); // the PID argument is ignored; see below for fun
    if (snap == INVALID_HANDLE_VALUE) {
        return MDMP_ERR_PROC_SNAPSHOT_FAILED;
        }

    thread.dwSize = sizeof(thread);
    ok = Thread32First(snap, &thread);
    while (ok) {
        if (thread.th32OwnerProcessID == pid) {
            // Funny gotcha: the PID argument in CreateToolhelp32Snapshot is ignored for TH32CS_SNAPTHREAD,
            // so suspending threads for some PID ended up suspending winlogon & co. on the first try...
            // The ensuing reboot convinced me to go through the documentation for CreateToolhelp32Snapshot
            // with greater interest & notice that.
            hThread = OpenThread(THREAD_SUSPEND_RESUME, 0, thread.th32ThreadID);
            if (hThread != INVALID_HANDLE_VALUE) {
                if (suspend) {
                    SuspendThread(hThread);
                    }
                else {
                    ResumeThread(hThread);
                    }
                CloseHandle(hThread);
                }
            }
        ok = Thread32Next(snap, &thread);
        }
    CloseHandle(snap);
    }

void _addDump(struct MDMP_DUMP_REQUEST *req, DWORD addr, DWORD size, HANDLE hProcess, TCHAR *processName, TCHAR *name, BYTE *data) {
    MDMP_REGION *region, *crt;
    TCHAR processName_[MAX_PATH];

    processName_[0] = 0;
    region = (MDMP_REGION *)getMem(sizeof(MDMP_REGION));
    if (!region) {
        req->warnings |= MDMP_WARN_MEM_ALLOC_FAILED;
        return;
        }
    region->next = 0;
    crt = req->regionList;
    if (crt) {
        while (crt->next) {
            crt = crt->next;
            }
        crt->next = region;
        }
    else {
        req->regionList = region;
        }
    region->addr = addr;
    region->size = size;
    region->pid = GetProcessId(hProcess);

    if (processName) {
        _lower(region->processName, processName, 32);
        }
    else {
        GetModuleBaseName(hProcess, 0, processName_, MAX_PATH - 1);
        _lower(region->processName, processName_, 32);
        }
    if (name) {
        _copy(region->name, name, 32);
        }
    else {
        region->name[0] = 0;
        }
    region->data = data;
    }

void _addGenericDump(MDMP_DUMP_REQUEST *req, HANDLE hProcess, size_t addr, TCHAR *processName, TCHAR *dumpType, MEMORY_BASIC_INFORMATION *pmbi) {
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T size;
    void *buf;
    BYTE *bBuf;
    TCHAR name[MAX_PATH], *prot;
    DWORD oldProt;

    if (!pmbi) {
        size = VirtualQueryEx(hProcess, (LPVOID)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
        pmbi = &mbi;
        }
    else {
        size = 1;
        }
    if (size) {
        //DEBUGBREAK;
        if (PAGE_GUARD & pmbi->Protect || PAGE_GUARD & pmbi->AllocationProtect) {
            req->warnings |= MDMP_WARN_PAGE_GUARD_NOT_DUMPING;
            }
        else {
            size = pmbi->RegionSize-(addr-(SIZE_T)pmbi->BaseAddress);
            buf = getMem(size);
            if (buf) {
                //RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, 1, 0, &oldProt);
                //VirtualProtectEx(hProcess, pmbi->BaseAddress, pmbi->RegionSize, PAGE_EXECUTE_READWRITE, &oldProt);
                if (ReadProcessMemory(hProcess, (LPVOID)addr, buf, size, &size)) {
                //if (ReadProcessMemory(hProcess, pmbi->BaseAddress, buf, pmbi->RegionSize-(addr-(SIZE_T)pmbi->BaseAddress), &size)) {
                    // [fixme]: this fails
                    if (size) {
                        if (req->dumpMode == MDMP_DUMP_SMART && !((DWORD*)buf)[0]) {
                            bBuf = (BYTE*)buf;
                            while ((bBuf - buf < size) && (!bBuf[0])) {
                                bBuf++;
                                }
                            if (bBuf - buf == size) {
                                // buffer only contains zeros
                                return;
                                }
                            }
                        switch (pmbi->Protect) {
                            case PAGE_EXECUTE: prot = _T("X"); break;
                            case PAGE_EXECUTE_READ: prot = _T("RX"); break;
                            case PAGE_EXECUTE_READWRITE: prot = _T("RWX"); break;
                            case PAGE_EXECUTE_WRITECOPY: prot = _T("WcX"); break;
                            case PAGE_NOACCESS: prot = _T("NoAcc"); break;
                            case PAGE_READONLY: prot = _T("RO"); break;
                            case PAGE_READWRITE: prot = _T("RW"); break;
                            case PAGE_WRITECOPY: prot = _T("Wc"); break;
                            default: prot = _T("unk");
                            }
                        _sprintf(name, _T("%s[%s]"), dumpType, prot);
                        _addDump(req, addr, size, hProcess, processName, name, (BYTE*)buf);
                        }
                    else {
                        req->warnings |= MDMP_WARN_READ_MEM_FAILED;
                        }
                    }
                else {
                    req->warnings |= MDMP_WARN_READ_MEM_FAILED;
                    }
                //VirtualProtectEx(hProcess, pmbi->BaseAddress, pmbi->RegionSize, oldProt, &oldProt);
                // DO NOT RELEASE buf! // freeMem(buf);
                }
            else {
                req->warnings |= MDMP_WARN_MEM_ALLOC_FAILED;
                }
            }
        }
    }

BOOL _addrIsDumped(MDMP_DUMP_REQUEST *req, size_t addr) {
    MDMP_REGION *crt;

    crt = req->regionList;
    if (crt) {
        do {
            if ((crt->addr <= addr) && (crt->addr + crt->size > addr))
                return TRUE;
            crt = crt->next;
            }
        while (crt);
        }
    return FALSE;
    }

void fixDumpImports(BYTE *imageData, size_t *size) {
    }

void fixDumpImage(BYTE *imageData, size_t *size) {
    PIMAGE_DOS_HEADER pDOSHeader;
    PIMAGE_NT_HEADERS pNTHeader;
    PIMAGE_SECTION_HEADER sections;
    size_t i, lastSec;

    if (*size < sizeof(IMAGE_DOS_HEADER)) {
        return;
        }
    pDOSHeader = (PIMAGE_DOS_HEADER)imageData;
    if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE || pDOSHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) > *size) {
        return;
        }
    pNTHeader = (PIMAGE_NT_HEADERS)&imageData[pDOSHeader->e_lfanew];
    //pNTHeader->OptionalHeader.FileAlignment = min(pNTHeader->OptionalHeader.FileAlignment, 0x200);
    sections = IMAGE_FIRST_SECTION(pNTHeader);
    if ((BYTE*)sections-imageData+pNTHeader->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER) > *size) {
        return;
        }
    lastSec = 0;
    for (i=0; i < pNTHeader->FileHeader.NumberOfSections; i++) {
        sections[i].PointerToRawData = sections[i].VirtualAddress;
        sections[i].SizeOfRawData = ALIGN_VALUE(sections[i].Misc.VirtualSize, pNTHeader->OptionalHeader.FileAlignment);
        if (sections[i].PointerToRawData > sections[lastSec].PointerToRawData) {
            lastSec = i;
            }
        }
    *size = sections[lastSec].PointerToRawData + sections[lastSec].SizeOfRawData;
    }

SIZE_T _dumpImage(MDMP_DUMP_REQUEST *req, HANDLE hProcess, HMODULE hModule) {
    // return 0 on fail, SizeOfImage on success
    MODULEINFO modInfo;
    TCHAR name[MAX_PATH], moduleName[MAX_PATH];
    SIZE_T size, size2;
    BYTE *buf;

    moduleName[0] = 0;
    if (GetModuleInformation(hProcess, hModule, &modInfo, sizeof(modInfo))) {
        size = modInfo.SizeOfImage;
        buf = (BYTE *)getMem(size);
        if (!buf) {
            req->warnings |= MDMP_WARN_MEM_ALLOC_FAILED;
            return 0;
            }
        if (ReadProcessMemory(hProcess, modInfo.lpBaseOfDll, buf, size, &size)) {
            if (!(req->flags & MDMP_FLAG_DONT_FIX_IMAGES)) {
                fixDumpImage(buf, (size_t*)&size);
                }
            if (req->flags & MDMP_FLAG_FIX_IMPORTS) {
                fixDumpImports(buf, (size_t*)&size);
                }
            GetModuleBaseName(hProcess, hModule, moduleName, MAX_PATH - 1);
            _sprintf(name, "pe[%s]", moduleName);
            _addDump(req, (size_t)modInfo.lpBaseOfDll, size, hProcess, 0, name, buf);
            return size;
            }
        else {
            freeMem(buf);
            }
        }
    else {
        req->warnings |= MDMP_WARN_QUERY_INFO_FAILED;
        }
    return 0;
    }

//--- API --------------------------------------------------------------------//
int __stdcall initMDmp() {
    // libMDmp initialization function; must be called before any other API
    // returns 1 on success, 0 on fail
    hNtDll = GetModuleHandle("ntdll.dll"); // always loaded
    if (!hNtDll) {
        return 0;
        }
    *(FARPROC *)&RtlAdjustPrivilege = GetProcAddress(hNtDll, "RtlAdjustPrivilege");
    *(FARPROC *)&NtQueryInformationThread_ = GetProcAddress(hNtDll, "NtQueryInformationThread");
    *(FARPROC *)&NtQueryInformationProcess_ = GetProcAddress(hNtDll, "NtQueryInformationProcess");
#ifdef UNICODE
    *(FARPROC *)&ntdll_swprintf = GetProcAddress(hNtDll, "swprintf");
    if (!ntdll_swprintf) {
        return 0;
        }
#else // UNICODE
    *(FARPROC *)&ntdll_sprintf = GetProcAddress(hNtDll, "sprintf");
    if (!ntdll_sprintf) {
        return 0;
        }
#endif // UNICODE

    GetSystemInfo(&systemInfo);

    if (!adjustPrivileges()) {
        return 0;
        }

    return 1;
    }

DWORD __stdcall getDumps(struct MDMP_DUMP_REQUEST *req) {
    // main workhorse; see help for details
    size_t i, j, count, npids, addr, maxAddr, remotePEBAddr;
    SIZE_T size, size2, readBytes;
    DWORD pids[MAX_PROCESSES], sizeD;
    HMODULE *hModules;
    HANDLE snap, hProcess, *handles, hThread;
    MODULEINFO modInf;
    TCHAR name[MAX_PATH], crtProcess[MAX_PATH];
    MEMORY_BASIC_INFORMATION mbi;
    BYTE *buf;
    PROCESSENTRY32 pe32;
    HEAPLIST32 heapList;
    HEAPENTRY32 heapEntry;
    THREADENTRY32 thread;
    BOOL ok, ok2;
    PIMAGE_DOS_HEADER pDOSHeader;
    THREAD_BASIC_INFORMATION threadInfo;
    PTEB_UNDOC pRemoteTEB;
    PEB_UNDOC remotePEB;
    PROCESS_BASIC_INFORMATION_UNDOC procInfo;
    // CONTEXT context;

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
        if (snap == INVALID_HANDLE_VALUE) {
            return MDMP_ERR_PROC_SNAPSHOT_FAILED;
            }
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(snap, &pe32)) {
            CloseHandle(snap);
            return MDMP_ERR_PROC_SNAPSHOT_FAILED;
            }
        do {
            if (req->procSelMode == MDMP_SEL_ALL || (req->procSelMode == MDMP_SEL_BY_NAME && _isSubString(req->processName, pe32.szExeFile))) {
                pids[npids++] = pe32.th32ProcessID;
                }
            if (npids == MAX_PROCESSES) {
                break;
                }
            }
        while (Process32Next(snap, &pe32));
        CloseHandle(snap);
        }
    if (!npids) {
        return MDMP_ERR_NO_PROCESS_MATCHES;
        }

    // walk selected processes
    for (i = 0; i < npids; i++) {
        //DEBUGBREAK;
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_OPERATION, 0, pids[i]);
        //hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pids[i]);
        if (!hProcess) {
            req->warnings |= MDMP_WARN_OPEN_PROCESS_FAILED;
            continue;
            }

        if (!GetModuleBaseName(hProcess, 0, crtProcess, sizeof(crtProcess)/sizeof(crtProcess[0]))) {
            req->warnings |= MDMP_WARN_GET_PROCESS_INFO_FAILED;
            continue;
            }

        // suspend process threads
        toggleProcessState(pids[i], TRUE);

        // read remote PEB
        if (NtQueryInformationProcess_(hProcess, ProcessBasicInformation_, &procInfo, sizeof(procInfo), 0) < 0xC0000000) {
            remotePEBAddr = (size_t)procInfo.PebBaseAddress;
            ReadProcessMemory(hProcess, procInfo.PebBaseAddress, &remotePEB, sizeof(PEB_UNDOC), &readBytes);
            }
        else {
            remotePEBAddr = 0;
            }

        pRemoteTEB = (PTEB_UNDOC)getMem(sizeof(TEB_UNDOC));

        // get dump
        switch (req->dumpMode) {
            case MDMP_DUMP_MAIN_IMAGE:
            case MDMP_DUMP_ALL_IMAGES:
            case MDMP_DUMP_IMAGE_BY_IMAGEBASE:
            case MDMP_DUMP_IMAGE_BY_NAME:
                // dump image(s)
                EnumProcessModules(hProcess, 0, 0, &sizeD);
                if ((sizeD > 0) && (sizeD <= 8192)) {
                    hModules = (HMODULE *)getMem(sizeD);
                    if (hModules) {
                        if (EnumProcessModules(hProcess, hModules, sizeD, &sizeD)) {
                            if (req->dumpMode == MDMP_DUMP_MAIN_IMAGE) {
                                _dumpImage(req, hProcess, hModules[0]);
                                }
                            else {
                                count = sizeD/sizeof(HMODULE*);
                                for (j = 0; j < count; j++) {
                                    if (req->dumpMode == MDMP_DUMP_IMAGE_BY_NAME) {
                                        if (!GetModuleFileNameEx(hProcess, hModules[j], name, sizeof(name))) {
                                            req->warnings |= MDMP_WARN_GET_MODULE_INFO_FAILED;
                                            name[0] = _T('\0');
                                            }
                                        }
                                    if (
                                        (req->dumpMode == MDMP_DUMP_ALL_IMAGES)
                                        ||
                                        ((req->dumpMode == MDMP_DUMP_IMAGE_BY_IMAGEBASE) && (req->imageBase == (size_t)hModules[j]))
                                        ||
                                        ((req->dumpMode == MDMP_DUMP_IMAGE_BY_NAME) && (_isSubString(req->moduleName, name)))
                                        ) {
                                        _dumpImage(req, hProcess, hModules[j]);
                                        }
                                    }
                                }
                            }
                        else {
                            req->warnings |= MDMP_WARN_ENUM_MODULES_FAILED;
                            }
                        freeMem(hModules);
                        }
                    else {
                        req->warnings |= MDMP_WARN_MEM_ALLOC_FAILED;
                        }
                    }
                break;
            case MDMP_DUMP_STACKS:
            case MDMP_DUMP_HEAPS:
            case MDMP_DUMP_SMART:
            case MDMP_DUMP_ALL_MEM:
            case MDMP_DUMP_REGION:
            case MDMP_DUMP_EXECUTABLE:
                // dump heaps
                if (req->dumpMode == MDMP_DUMP_HEAPS || req->dumpMode == MDMP_DUMP_SMART) {
                    snap = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, pids[i]);
                    if (snap == INVALID_HANDLE_VALUE) {
                        req->warnings |= MDMP_WARN_ENUM_HEAPS_FAILED;
                        }
                    else {
                        heapList.dwSize = sizeof(heapList);
                        ok = Heap32ListFirst(snap, &heapList);
                        while (ok) {
                            heapEntry.dwSize = sizeof(heapEntry);
                            heapEntry.dwFlags = 0;
                            ok2 = Heap32First(&heapEntry, pids[i], heapList.th32HeapID);
                            addr = heapEntry.dwAddress & 0xFFFFF000;
                            /*while (ok2 && (heapEntry.dwFlags != LF32_FREE)) {
                                ok2 = Heap32Next(&heapEntry);
                                }*/
                            _addGenericDump(req, hProcess, addr, crtProcess, _T("heap"), 0);
                            ok = Heap32ListNext(snap, &heapList);
                            }
                        CloseHandle(snap);
                        }
                    if (req->dumpMode == MDMP_DUMP_HEAPS) {
                        break;
                        }
                    }

                // dump stacks
                if (req->dumpMode == MDMP_DUMP_STACKS || req->dumpMode == MDMP_DUMP_SMART) {
                    snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
                    if (snap == INVALID_HANDLE_VALUE) {
                        req->warnings |= MDMP_WARN_ENUM_THREADS_FAILED;
                        }
                    else {
                        thread.dwSize = sizeof(thread);
                        ok = Thread32First(snap, &thread);
                        while (ok) {
                            if (thread.th32OwnerProcessID == pids[i]) {
                                hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, 0, thread.th32ThreadID);
                                if (hThread != INVALID_HANDLE_VALUE) {
                                    // GetThreadContext(hThread, &context) // not working. :|
                                    if (!NtQueryInformationThread_(hThread, ThreadBasicInformation, &threadInfo, sizeof(threadInfo), 0)) {
                                        if (ReadProcessMemory(hProcess, threadInfo.TebBaseAddress, pRemoteTEB, sizeof(TEB_UNDOC), &readBytes)) {
                                            _sprintf(name, _T("stack-T%d"), thread.th32ThreadID);
                                            _addGenericDump(req, hProcess, (size_t)pRemoteTEB->Tib.StackLimit, crtProcess, name, 0);
                                            }
                                        }
                                    CloseHandle(hThread);
                                    }
                                }
                            ok = Thread32Next(snap, &thread);
                            }
                        CloseHandle(snap);
                        }
                    if (req->dumpMode == MDMP_DUMP_STACKS) {
                        break;
                        }
                    }

                // dump rest
                //DEBUGBREAK;
                if (req->dumpMode == MDMP_DUMP_REGION) {
                    addr = req->startAddr;
                    maxAddr = req->endAddr;
                    }
                else {
                    addr = (size_t)systemInfo.lpMinimumApplicationAddress;
                    maxAddr = (size_t)systemInfo.lpMaximumApplicationAddress; //(size_t)-1;
                    }
                while (addr < maxAddr) {
                    size = VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                    if (!(size && mbi.RegionSize)) {
                        break;
                        }
                    size = mbi.RegionSize;
                    if (
                        _addrIsDumped(req, addr)
                        || (mbi.Protect & PAGE_GUARD)
                        || (mbi.AllocationProtect & PAGE_GUARD)
                        || (mbi.State != MEM_COMMIT)
                        || ((req->dumpMode == MDMP_DUMP_EXECUTABLE) && !(mbi.Protect & (0x10 | 0x20 | 0x40 | 0x80)))
                        ) {
                        addr += size;
                        continue;
                        }
                    if ((req->dumpMode == MDMP_DUMP_SMART) && (size >= sizeof(IMAGE_DOS_HEADER))) {
                        buf = (BYTE*)getMem(min(size, sizeof(IMAGE_DOS_HEADER)));
                        if (ReadProcessMemory(hProcess, (LPCVOID)addr, buf, min(size, sizeof(IMAGE_DOS_HEADER)), &readBytes)) {
                            pDOSHeader = (PIMAGE_DOS_HEADER)buf;
                            if (
                                (pDOSHeader->e_magic == IMAGE_DOS_SIGNATURE)
                                && (pDOSHeader->e_lfanew < 0x2000)
                                && (size2 = _dumpImage(req, hProcess, (HMODULE)addr))
                                ) {
                                addr += size2;
                                }
                            else {
                                _addGenericDump(req, hProcess, addr, crtProcess, addr == remotePEBAddr ? _T("struc-peb") : _T("other"), &mbi);
                                addr = (size_t)mbi.BaseAddress + mbi.RegionSize;
                                }
                            }
                        else {
                            req->warnings |= MDMP_WARN_READ_MEM_FAILED;
                            addr = (size_t)mbi.BaseAddress + mbi.RegionSize;
                            }
                        freeMem(buf);
                        }
                    else {
                        _addGenericDump(req, hProcess, addr, crtProcess, _T("other"), &mbi);
                        addr = (size_t)mbi.BaseAddress + mbi.RegionSize;
                        }
                    //addr += size;
                    }
                break;
            } // switch

        // resume suspended threads
        toggleProcessState(pids[i], FALSE);

        freeMem(pRemoteTEB);
        CloseHandle(hProcess);
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

TCHAR __stdcall processName(DWORD pid, TCHAR *buffer, size_t bufferSize) {
    HANDLE ph;
    TCHAR result[MAX_PATH];
    size_t size;

    ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid);
    if (ph) {
        size = GetModuleFileNameEx(ph, 0, buffer, bufferSize);
        CloseHandle(ph);
        return size;
        }
    else {
        return 0;
        }
    }

DWORD __stdcall dumpToDisk(struct MDMP_DUMP_REQUEST *req) {
    TCHAR fileName[MAX_PATH];
    MDMP_REGION *crt;
    HANDLE fh;
    DWORD written;

    if (!req) {
        return MDMP_ERR_INVALID_ARGS;
        }
    crt = req->regionList;
    while (crt) {
        if (req->flags & MDMP_FLAG_SORT_BY_ADDR) {
            _sprintf(fileName, _T("%s(%d)-%p-%p-%s.dump"), crt->processName, crt->pid, crt->addr, crt->size, crt->name ? crt->name : UNKNOWN_TEXT);
            }
        else {
            _sprintf(fileName, _T("%s(%d)-%s-%p-%p.dump"), crt->processName, crt->pid, crt->name ? crt->name : UNKNOWN_TEXT, crt->addr, crt->size);
            }
        fh = CreateFile(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (fh != INVALID_HANDLE_VALUE) {
            WriteFile(fh, crt->data, crt->size, &written, 0);
            CloseHandle(fh);
            }
        else {
            return MDMP_ERR_WRITING_TO_DISK;
            }
        crt = crt->next;
        }
    return MDMP_OK;
    }

DWORD __stdcall dumpPIDToFile(DWORD pid, struct MDMP_DUMP_REQUEST *req) {
    if (!req) {
        return MDMP_ERR_INVALID_ARGS;
        }

    // [todo]: code...
    return MDMP_OK;
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
