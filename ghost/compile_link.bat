@echo off
setlocal enabledelayedexpansion

:: ============================================================
:: Ghost Agent — Compile + Link
:: Auto-detects Visual Studio installation
:: ============================================================

cd /d "%~dp0"

:: Try to find Visual Studio
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
    echo [*] Found Visual Studio 2022
) else if exist "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
    echo [*] Found Visual Studio 2019
) else (
    where cl >nul 2>nul
    if !errorlevel! neq 0 (
        echo [ERROR] Visual Studio not found.
        echo   Install VS Build Tools or open "Developer Command Prompt for VS"
        exit /b 1
    )
    echo [*] Using cl.exe from PATH
)

:: Ensure ml64 is available
where ml64 >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] ml64.exe not found — install MASM component
    exit /b 1
)

:: Assemble reflective loader
echo [*] Assembling reflective_loader.asm...
ml64 /c /Fo reflective_loader.obj reflective_loader.asm >nul
if %errorlevel% neq 0 (
    echo [FAILED] assembly error
    exit /b 1
)

:: Compile ghost with optional embedded DLL
if exist loader.h (
    set EMBED=-DCHEAT_DLL_EMBEDDED
    echo [*] loader.h found — embedding cheat DLL
) else (
    set EMBED=
    echo [*] loader.h not found — will use cheat.dll from disk
)

cl /EHsc /std:c++17 /O2 /W3 /GS- %EMBED% ghost.cpp reflective_loader.obj /Feghost.exe /link kernel32.lib user32.lib winhttp.lib advapi32.lib psapi.lib /SUBSYSTEM:WINDOWS

if exist ghost.exe (
    echo [OK] ghost.exe compiled successfully
    dir ghost.exe
) else (
    echo [FAILED]
)
