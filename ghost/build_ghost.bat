@echo off
title building ghost agent
echo.
echo  === building ghost.exe ===
echo.

where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Visual Studio Build Tools not found.
    echo   Open "Developer Command Prompt for VS" and run this again.
    pause
    exit /b 1
)

:: Check for MASM (ml64)
where ml64 >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] ml64.exe not found — need Visual Studio with MASM.
    pause
    exit /b 1
)

:: Compile reflective loader assembly
echo [*] assembling reflective_loader.asm...
ml64 /c /Fo reflective_loader.obj reflective_loader.asm
if %errorlevel% neq 0 (
    echo [FAILED] assembly error
    pause
    exit /b 1
)

:: Check if loader.h exists for embedded bytes
if exist loader.h (
    set EMBED=-DCHEAT_DLL_EMBEDDED
    echo [*] loader.h found — embedding cheat DLL bytes (no disk touch)
) else (
    set EMBED=
    echo [*] loader.h not found — will use cheat.dll from disk (fallback)
)

echo [*] compiling ghost.cpp...
cl /EHsc /std:c++17 /O2 /W3 /GS- %EMBED% ghost.cpp reflective_loader.obj /Fe:ghost.exe /link kernel32.lib user32.lib winhttp.lib advapi32.lib psapi.lib /SUBSYSTEM:WINDOWS

if %errorlevel% equ 0 (
    echo.
    echo  [OK] ghost.exe created
    echo.
    echo  to use embedded DLL: copy loader.h to this folder and rebuild
    echo  to start: ghost.exe --server=https://render-cheat-panel.onrender.com
    echo.
) else (
    echo.
    echo  [FAILED] build error
    echo.
)
pause
