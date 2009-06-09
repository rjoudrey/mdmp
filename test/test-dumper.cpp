#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <math.h>

#define ALIGN_ADDR(addr, alignment) (addr % alignment) ? (addr + alignment - (addr % alignment)) : (addr)

DWORD (__stdcall *RtlAdjustPrivilege)(DWORD, DWORD, DWORD, PVOID);

int main(int argc, char **argv) {

   HANDLE h, hf;
   MEMORY_BASIC_INFORMATION mbi;
   DWORD i, pid, size, addr = 0, tmp;
   PIMAGE_DOS_HEADER pdos;
   PIMAGE_NT_HEADERS pnt;
   PIMAGE_SECTION_HEADER ps;
   MODULEINFO mi;
   HINSTANCE hNtDll;
   char *buf, *sec, fileName[MAX_PATH], baseName[MAX_PATH];
   BOOL dumped;
   
   for (i=1; i<argc; i++) {
       if (argv[i][0] != '/')
          pid = atoi(argv[i]);
       }

   hNtDll = GetModuleHandle("ntdll.dll");
   *(FARPROC *)&RtlAdjustPrivilege = GetProcAddress(hNtDll, "RtlAdjustPrivilege");
   RtlAdjustPrivilege(20, 1, 0, &tmp);

   h = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_OPERATION, 0, pid);
   if (h < 0) return 1;

   while (1) {
      size = VirtualQueryEx(h, (LPCVOID)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
      if (!size) break;
      printf("Base addr: %p; AllocationBase: %p; AllocationProtect:%x; RegionSize: %d; State: %x; Protect: %x; Type: %x\n", mbi.BaseAddress, mbi.AllocationBase, mbi.AllocationProtect, mbi.RegionSize, mbi.State, mbi.Protect, mbi.Type);
      size = mbi.RegionSize;
      buf = (char*)malloc(size);
      dumped = 0;
      if (ReadProcessMemory(h, (LPCVOID)addr, buf, size, &size)) {
         if ((((WORD*)buf)[0] == 0x5A4D) && GetModuleInformation(h, (HMODULE)addr, &mi, sizeof(mi))) {// MZ
            if (!GetModuleBaseName(h, (HMODULE)addr, baseName, MAX_PATH))
               strcpy(baseName, "unknown");
            sprintf(fileName, "PID-%d-%p-%s.mdmp", pid, addr, baseName);
            hf = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            pdos = (PIMAGE_DOS_HEADER)buf;
            pnt = (PIMAGE_NT_HEADERS)&(buf[pdos->e_lfanew]);
            pnt->OptionalHeader.FileAlignment = min(pnt->OptionalHeader.FileAlignment, 0x200);
            ps = IMAGE_FIRST_SECTION(pnt);
            SetFilePointer(hf, pnt->OptionalHeader.SizeOfHeaders, 0, 0);
            for (i=0; i < pnt->FileHeader.NumberOfSections; i++) {
                printf("addr: %p; va: %p, vs:%x\n", addr, ps[i].VirtualAddress, ps[i].Misc.VirtualSize);
                //VirtualProtectEx(h, (LPVOID)(addr + ps[i].VirtualAddress), ps[i].Misc.VirtualSize, PAGE_EXECUTE_READWRITE, &tmp);
                sec = (char*)malloc(ALIGN_ADDR(ps[i].Misc.VirtualSize, pnt->OptionalHeader.FileAlignment));
                if (sec && ReadProcessMemory(h, (LPCVOID)(addr + ps[i].VirtualAddress), sec, ps[i].Misc.VirtualSize, &size)) {
                   ps[i].PointerToRawData = SetFilePointer(hf, 0, 0, FILE_CURRENT);
                   while ((!sec[size]) && (size > 0)) size--;
                   size = ALIGN_ADDR(size, pnt->OptionalHeader.FileAlignment);
                   ps[i].SizeOfRawData = size;
                   WriteFile(hf, sec, size, &size, 0);
                   }
                if (sec) free(sec);
                //VirtualProtectEx(h, (LPVOID)(addr + ps[i].VirtualAddress), ps[i].Misc.VirtualSize, tmp, 0);
                }
            //__asm{int 3}
            SetFilePointer(hf, 0, 0, 0);
            WriteFile(hf, buf, pnt->OptionalHeader.SizeOfHeaders, &size, 0);
            CloseHandle(hf);
            addr += pnt->OptionalHeader.SizeOfImage;
            dumped = 1;
            }
         if (!dumped) {
            sprintf(fileName, "PID-%d-%p.mdmp", pid, addr);
            hf = CreateFile(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            WriteFile(hf, buf, mbi.RegionSize, &size, 0);
            printf(" - written %d bytes out of %d to file %s\n", size, mbi.RegionSize, fileName);
            CloseHandle(hf);
            }
         }
      else printf(" - read failed!\n");
      free(buf);
      if (!dumped) addr += mbi.RegionSize;
      }
   }
