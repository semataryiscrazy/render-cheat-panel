@echo off
title building injector_cli
echo.
echo  === building injector_cli.exe ===
echo.

where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Visual Studio Build Tools not found.
    echo   Open "Developer Command Prompt for VS" and run this again.
    pause
    exit /b 1
)

cl /EHsc /std:c++17 /O2 /W3 injector_cli.cpp /Fe:injector_cli.exe /link kernel32.lib user32.lib advapi32.lib
if %errorlevel% equ 0 (
    echo  [OK] injector_cli.exe created
) else (
    echo  [FAILED] injector_cli.exe
)
echo.

:: Build injector simples (compatibilidade)
cl /EHsc /std:c++17 /O2 /W3 injector.cpp /Fe:injector.exe /link kernel32.lib user32.lib
if %errorlevel% equ 0 (
    echo  [OK] injector.exe created
) else (
    echo  [FAILED] injector.exe
)
echo.

:: Build hd_injector (BlueStacks)
cl /EHsc /std:c++17 /O2 /W3 hd_injector.cpp /Fe:hd_injector.exe /link kernel32.lib user32.lib advapi32.lib shlwapi.lib
if %errorlevel% equ 0 (
    echo  [OK] hd_injector.exe created
) else (
    echo  [FAILED] hd_injector.exe
)
echo.

:: Build unloader
cl /EHsc /std:c++17 /O2 /W3 unloader.cpp /Fe:unloader.exe /link ws2_32.lib
if %errorlevel% equ 0 (
    echo  [OK] unloader.exe created
) else (
    echo  [FAILED] unloader.exe
)
echo.

pause
