@echo off
title Nyx Project — Build All
setlocal enabledelayedexpansion

echo.
echo  ╔══════════════════════════════════════╗
echo  ║     Nyx Project — Build All          ║
echo  ║     (c) He + Nyx                     ║
echo  ╚══════════════════════════════════════╝
echo.

:: ============================================================
:: Check for Visual Studio Build Tools
:: ============================================================
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] Visual Studio Build Tools not found in PATH.
    echo.
    echo     Please open a "Developer Command Prompt for VS" and run this script from there.
    echo     Or install Build Tools from:
    echo     https://visualstudio.microsoft.com/visual-cpp-build-tools/
    echo.
    pause
    exit /b 1
)

echo [*] Visual Studio found: 
where cl
echo.

:: ============================================================
:: 1. Build Injector CLI
:: ============================================================
echo  === [1/5] Building injector_cli.exe ===
echo.
pushd "%~dp0cheat"
if exist injector_cli.cpp (
    cl /EHsc /std:c++17 /O2 /W3 injector_cli.cpp /Fe:injector_cli.exe /link kernel32.lib user32.lib advapi32.lib >build_log.txt 2>&1
    if !errorlevel! equ 0 (
        echo [OK] injector_cli.exe created
    ) else (
        echo [FAILED] injector_cli — check cheat\build_log.txt
    )
) else (
    echo [SKIP] injector_cli.cpp not found
)
echo.
popd

:: ============================================================
:: 2. Build Cheat DLL
:: ============================================================
echo  === [2/5] Building cheat.dll ===
echo.
pushd "%~dp0cheat"
if exist cheat.cpp (
    cl /EHsc /std:c++17 /O2 /W3 /LD /Fe:cheat.dll cheat.cpp server.cpp config.cpp /link kernel32.lib user32.lib gdi32.lib winmm.lib ws2_32.lib /DEF:cheat.def /DLL >build_dll_log.txt 2>&1
    if !errorlevel! equ 0 (
        echo [OK] cheat.dll created
        if exist "%~dp0render-cheat-panel\public\" (
            copy /Y cheat.dll "%~dp0render-cheat-panel\public\" >nul 2>nul
            echo [*] copied cheat.dll to render-cheat-panel\public\
        ) else (
            echo [*] skip copy — render-cheat-panel\public not found
        )
    ) else (
        echo [FAILED] cheat.dll — check cheat\build_dll_log.txt
    )
) else (
    echo [SKIP] cheat.cpp not found
)
echo.
popd

:: ============================================================
:: 3. Build Ghost Agent
:: ============================================================
echo  === [3/5] Building ghost.exe ===
echo.
where ml64 >nul 2>nul
if !errorlevel! equ 0 (
    pushd "%~dp0ghost"
    ml64 /c /Fo reflective_loader.obj reflective_loader.asm >asm_log.txt 2>&1
    if !errorlevel! equ 0 (
        echo [OK] reflective_loader.asm assembled
        if exist loader.h (
            set EMBED_FLAG=-DCHEAT_DLL_EMBEDDED
        ) else (
            set EMBED_FLAG=
        )
        cl /EHsc /std:c++17 /O2 /W3 /GS- !EMBED_FLAG! ghost.cpp reflective_loader.obj /Fe:ghost.exe /link kernel32.lib user32.lib winhttp.lib advapi32.lib psapi.lib /SUBSYSTEM:WINDOWS >ghost_build_log.txt 2>&1
        if !errorlevel! equ 0 (
            echo [OK] ghost.exe created
        ) else (
            echo [FAILED] ghost.exe — check ghost\ghost_build_log.txt
        )
    ) else (
        echo [FAILED] assembly error — check ghost\asm_log.txt
    )
    popd
) else (
    echo [SKIP] ml64 not found — install MASM component of VS
)
echo.

:: ============================================================
:: 4. Build Web Panel
:: ============================================================
echo  === [4/5] Building web_panel.exe ===
echo.
pushd "%~dp0web_panel"
if exist main.cpp (
    cl /EHsc /std:c++17 /O2 /W3 main.cpp /Fe:web_panel.exe /link ws2_32.lib wininet.lib user32.lib >web_panel_build_log.txt 2>&1
    if !errorlevel! equ 0 (
        echo [OK] web_panel.exe created
    ) else (
        echo [FAILED] web_panel.exe — check web_panel\web_panel_build_log.txt
    )
) else (
    echo [SKIP] main.cpp not found
)
echo.
popd

:: ============================================================
:: 5. Install Node.js dependencies (if npm available)
:: ============================================================
echo  === [5/5] Installing render-cheat-panel npm packages ===
echo.
where npm >nul 2>nul
if !errorlevel! equ 0 (
    pushd "%~dp0render-cheat-panel"
    if exist package.json (
        call npm install
        if !errorlevel! equ 0 (
            echo [OK] npm packages installed
        ) else (
            echo [FAILED] npm install — check render-cheat-panel
        )
    ) else (
        echo [SKIP] package.json not found
    )
    popd
) else (
    echo [SKIP] npm not found — install Node.js to use the render panel
)
echo.

:: ============================================================
:: Summary
:: ============================================================
echo  ╔══════════════════════════════════════╗
echo  ║     Build Complete                    ║
echo  ╚══════════════════════════════════════╝
echo.
echo  check logs in each folder for details.
echo.
echo  next steps:
echo    1. configure auth_bot\config.json with your Discord token
echo    2. set environment variables (see auth_bot\.env.example)
echo    3. run: python auth_bot.py  (inside auth_bot folder)
echo    4. deploy render-cheat-panel to render.com
echo    5. run ghost.exe --server=https://your-panel.onrender.com
echo.
pause
