@echo off
:
: Build batch file for the MDMP project
:   author: Vlad-Ioan Topan (vtopan / gmail.com)
:
: Notes:
:  * tested with MS Visual C++ Express 2008; requires the Platform SDK; see help
:    for more info
:  * assumes:
:     * the INCLUDE env. var. contains the <MS-VS>\VC\include and
:       <MS-PSDK>\include folders; either set/update them from the "System
:       Properties" applet (Win+Break, Advanced tab, "Environment Variables"
:       button, or call this batch from another one which sets them using
:       SET INCLUDE=<MS-VC>\include;<MS-PSDK>\include
:     * the LIB env. var. contains <MS-VS>\VC\lib and <MS-PSDK>\lib; see above
:     * cl.exe and mspdb80.dll's folder (usually <MS-VS>\VC\bin and
:       <MS-VS>\Common*\IDE respectively) is in the PATH env. var.
:

ECHO Building libmdmp.dll...>build.log
ECHO .>build-errors.log
DEL out\*.* /S /Q >nul 2>nul
DEL *.log /S /Q >nul 2>nul
SET crtfile=libmdmp
DEL %crtfile%.* >nul 2>nul
cl.exe /O2 /GS /Gz /nologo mdmp\%crtfile%.c /link /DLL advapi32.lib kernel32.lib /DEF:mdmp\%crtfile%.def /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:DllMain /OUT:out\%crtfile%.dll >>build.log 2>>build-errors.log
FOR %%x IN (1 2) DO IF ERRORLEVEL %%x GOTO build_failed
echo Build successful!
GOTO done

:build_errors
GOTO build_failed

:build_failed
ECHO From build.log:>>build-errors.log
TYPE build.log|FIND "error">>build-errors.log
ECHO Build failed! See build-errors.log for details.

:done
DEL out\*.exp >nul 2>nul
DEL *.obj >nul 2>nul
FOR %%x in (build-errors.log) DO SET size=%%~zx
IF %size% EQU 0 DEL build-errors.log >nul 2>nul
PAUSE
