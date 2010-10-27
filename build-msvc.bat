@echo off
:
: Build batch file for the MDMP project
:   author: Vlad-Ioan Topan (vtopan / gmail.com)
:
: Notes:
:  * tested with MS Visual C++ Express 2008; requires the Platform SDK; see help
:    for more info

SET VS_PATH=C:\VS.LATEST
SET INCLUDE=C:\VS.LATEST\vc\include;C:\SDK.LATEST\include
SET LIB=C:\VS.LATEST\vc\lib;C:\SDK.LATEST\lib
: comment the above lines or adjust them to your setup

SET reqd_libs=advapi32.lib kernel32.lib psapi.lib
SET mdmp_link_opts=/ALIGN:512 /SUBSYSTEM:CONSOLE /IGNORE:4108
SET cl_opts=/nologo /Gz
: /O2

ECHO .>build.log
ECHO .>build-errors.log
DEL out\*.* /S /Q >nul 2>nul
DEL *.log /S /Q >nul 2>nul

SET crtfile=mdmp
echo Building mdmp.exe (static code linking)...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /MD /GS- mdmp\%crtfile%.cpp mdmp\libmdmp.cpp %cl_opts% /link %reqd_libs% /RELEASE %mdmp_link_opts% /OUT:out\%crtfile%.exe >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed
mt.exe /manifest out\mdmp.exe.manifest /outputresource:out\mdmp.exe;#1 >nul 2>nul
DEL out\mdmp.exe.manifest >nul 2>nul

SET crtfile=libmdmp
echo Building libmdmp.dll...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" mdmp\%crtfile%.cpp %cl_opts% /GS- /link /DLL %reqd_libs% /RELEASE /ALIGN:512 /DEF:mdmp\%crtfile%.def /BASE:0x45670000 /SUBSYSTEM:WINDOWS /NODEFAULTLIB /IGNORE:4108 /ENTRY:DllMain /OUT:out\%crtfile%.dll >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed

SET crtfile=mdmp
echo Building mdmp_dll.exe (depends on libmdmp.dll; for testing purposes only)...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /MD /GS- mdmp\%crtfile%.cpp %cl_opts% /link %reqd_libs% out\libmdmp.lib /RELEASE %mdmp_link_opts% /OUT:out\%crtfile%_dll.exe >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed

echo Building mdmp_dbg.exe (debug version; for testing purposes only)...
DEL %crtfile%.* >nul 2>nul
"%VS_PATH%\vc\bin\cl.exe" /Zi /MD /GS- mdmp\%crtfile%.cpp libmdmp.obj %cl_opts% /link %reqd_libs% /DEBUG %mdmp_link_opts% /OUT:out\%crtfile%_dbg.exe >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed

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
