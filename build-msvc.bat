@echo off
:
: Build batch file for the MDMP project
:   ver: 0.2.5 (15:43:53 18.05.2011)
:   author: Vlad-Ioan Topan (vtopan / gmail.com)
:
: Notes:
:  * tested with MS Visual C++ Express 2008; requires the Platform SDK; see help
:    for more info
:  * do NOT force __stdcall; Python requires __cdecl (default)

: Used CL args:
:   /O2     maximize speed

SET VS_PATH=C:\VS.LATEST
SET PSDK_PATH=C:\SDK.LATEST
SET PY_PATH=C:\PY.LATEST
SET ORIG_PATH=%PATH%
: comment out the above lines or adjust them to your setup

SET INCLUDE=%VS_PATH%\vc\include;%PSDK_PATH%\include;%PY_PATH%\include;include
SET LIB=%VS_PATH%\vc\lib;%PSDK_PATH%\lib;%PY_PATH%\libs
SET PATH=%ORIG_PATH%;%VS_PATH%\Common7\IDE

SET reqd_libs=advapi32.lib kernel32.lib psapi.lib
SET mdmp_link_opts=/ALIGN:512 /SUBSYSTEM:CONSOLE /IGNORE:4108
SET cl_opts=/nologo
: /O2

ECHO .>build.log
ECHO .>build-errors.log
DEL out\*.* /S /Q >nul 2>nul
DEL *.log /S /Q >nul 2>nul

:build_mdmp_exe
SET crtfile=mdmp
echo Building mdmp.exe (static code linking)...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /MT /GS- mdmp\%crtfile%.cpp mdmp\libmdmp.cpp %cl_opts% /link %reqd_libs% /RELEASE %mdmp_link_opts% /OUT:out\%crtfile%.exe >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed
mt.exe /manifest out\%crtfile%.exe.manifest /outputresource:out\%crtfile%.exe;#1 >nul 2>nul
DEL out\%crtfile%.exe.manifest >nul 2>nul

:build_pymdmp_pyd
SET crtfile=pymdmp
echo Building pymdmp.pyd...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /MT /GS- mdmp\%crtfile%.cpp mdmp\libmdmp.cpp %cl_opts% /link /DLL %reqd_libs% /RELEASE /ALIGN:512 /DEF:mdmp\%crtfile%.def %mdmp_link_opts% /BASE:0x45680000 /SUBSYSTEM:WINDOWS /ENTRY:DllMain /OUT:out\%crtfile%.pyd >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed
mt.exe /manifest out\%crtfile%.pyd.manifest /outputresource:out\%crtfile%.pyd;#1 >nul 2>nul
DEL out\%crtfile%.pyd.manifest >nul 2>nul

:build_libmdmp_dll
SET crtfile=libmdmp
echo Building libmdmp.dll...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /MT /GS- mdmp\%crtfile%.cpp %cl_opts% /link /DLL %reqd_libs% /RELEASE /ALIGN:512 /DEF:mdmp\%crtfile%.def /BASE:0x45670000 /SUBSYSTEM:WINDOWS /NODEFAULTLIB /IGNORE:4108 /ENTRY:DllMainLibMDMP /OUT:out\%crtfile%.dll >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed

:build_debug_and_testing_versions
SET crtfile=mdmp
echo Building mdmp_dll.exe (depends on libmdmp.dll; for testing purposes only)...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /MT /GS- mdmp\%crtfile%.cpp %cl_opts% /link %reqd_libs% out\libmdmp.lib /RELEASE %mdmp_link_opts% /OUT:out\%crtfile%_dll.exe >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed
echo Building mdmp_dbg.exe (debug version; for testing purposes only)...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /Zi /MT /GS- mdmp\%crtfile%.cpp libmdmp.obj %cl_opts% /link %reqd_libs% /DEBUG %mdmp_link_opts% /OUT:out\%crtfile%_dbg.exe >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed

:build_mdmp64
echo Building mdmp.exe/64 (static code linking)...
SET INCLUDE=%PSDK_PATH%\include;%VS_PATH%\VC\ATLMFC\INCLUDE;%VS_PATH%\VC\INCLUDE;include
SET LIB=%PSDK_PATH%\LIB\x64;%VS_PATH%\VC\ATLMFC\LIB\amd64;%VS_PATH%\VC\LIB\amd64;
:::SET PATH=%ORIG_PATH%;%VS_PATH%\VC\bin\amd64

DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\x86_amd64\cl.exe" /MT /GS- mdmp\%crtfile%.cpp mdmp\libmdmp.cpp %cl_opts% /link %reqd_libs% /RELEASE %mdmp_link_opts% /OUT:out\%crtfile%64.exe >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed
mt.exe /manifest out\%crtfile%64.exe.manifest /outputresource:out\%crtfile%64.exe;#1 >nul 2>nul
DEL out\%crtfile%64.exe.manifest >nul 2>nul

GOTO done

:build_errors
GOTO build_failed

:build_failed
ECHO From build.log:>>build-errors.log
TYPE build.log|FIND "error">>build-errors.log

:done
ECHO.
DEL out\*.exp >nul 2>nul
DEL out\*.ilk >nul 2>nul
DEL /Q *.obj >nul 2>nul
DEL /Q *.pdb >nul 2>nul
FOR %%x in (build-errors.log) DO SET size=%%~zx
IF %size% EQU 0 (
   DEL build-errors.log >nul 2>nul
   ECHO Build successful!
) ELSE (
   ECHO Build errors/warnings:
   TYPE build-errors.log
   )
ECHO.
