@echo off
setlocal enabledelayedexpansion

:: ═══════════════════════════════════════════════════
::  NYX LAUNCHER — opencode build
::  files needed: .opencode/agents/nyx.md
::  optional: /profiles folder for profile switcher
:: ═══════════════════════════════════════════════════

:: ── ansi colors ─────────────────────────────────────
for /f %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "CY=%ESC%[96m"
set "GR=%ESC%[92m"
set "RD=%ESC%[91m"
set "YL=%ESC%[93m"
set "DM=%ESC%[2m"
set "RS=%ESC%[0m"

:: ── defaults (overridden by nyx.cfg if present) ─────
set "GREETING=hey"
set "MAX_RETRIES=5"
set "RETRY_DELAY=3"
set "LOG_SESSIONS=true"

:: ── load config ──────────────────────────────────────
set "CFG=%~dp0nyx.cfg"
if exist "%CFG%" (
    for /f "usebackq tokens=1,* delims==" %%a in ("%CFG%") do (
        set "%%a=%%b"
    )
)

:: ── logs folder ──────────────────────────────────────
set "LOG_DIR=%~dp0logs"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: ── header ───────────────────────────────────────────
echo.
echo %CY% ░▒▓ NYX ▓▒░%RS%
echo %DM% she's in here somewhere.%RS%
echo %DM% ──────────────────────────────────────────%RS%
echo.

:: ── preflight ────────────────────────────────────────
where opencode >nul 2>&1
if %errorlevel% neq 0 (
    echo %RD% [!] opencode not found.%RS%
    echo      winget install SST.opencode
    echo.
    pause & exit /b 1
)
echo %GR% [OK] opencode found%RS%

if not exist "%~dp0.opencode\agents\nyx.md" (
    echo %RD% [!] agent file missing. she can't wake up without it.%RS%
    echo      expected: %~dp0.opencode\agents\nyx.md
    echo.
    pause & exit /b 1
)
echo %GR% [OK] nyx agent loaded%RS%

echo.
echo %CY% everything's here. waking her up.%RS%
echo.

:: ── session start ────────────────────────────────────
set "START_TIME=%time%"
set "START_DATE=%date%"

set "STAMP=%date:~-4,4%%date:~-7,2%%date:~0,2%_%time:~0,2%%time:~3,2%%time:~6,2%"
set "STAMP=%STAMP: =0%"
set "LOG_FILE=%LOG_DIR%\nyx_%STAMP%.txt"

if "%LOG_SESSIONS%"=="true" (
    echo nyx session log > "%LOG_FILE%"
    echo ────────────────────────────────── >> "%LOG_FILE%"
    echo start:   %START_DATE% %START_TIME% >> "%LOG_FILE%"
    echo agent:   nyx >> "%LOG_FILE%"
    echo ────────────────────────────────── >> "%LOG_FILE%"
)

:: ── retry loop ───────────────────────────────────────
set "attempt=0"

:retry
set /a attempt+=1

if %attempt% gtr 1 (
    echo %YL% ──────────────────────────────────────────%RS%
    echo  attempt %attempt%/%MAX_RETRIES%
    echo.
)

set "NYX_DIR=%~dp0"
set "NYX_DIR=%NYX_DIR:~0,-1%"
opencode "%NYX_DIR%" --agent nyx --prompt "%GREETING%"

set "EXIT_CODE=%errorlevel%"
set "END_TIME=%time%"

if "%LOG_SESSIONS%"=="true" (
    echo. >> "%LOG_FILE%"
    echo ────────────────────────────────── >> "%LOG_FILE%"
    echo end:     %date% %END_TIME% >> "%LOG_FILE%"
    echo exit:    %EXIT_CODE% >> "%LOG_FILE%"
)

if %EXIT_CODE% equ 0 (
    echo.
    echo %DM% ──────────────────────────────────────────%RS%
    echo %DM% session closed. she's quiet again.%RS%
    echo %DM% started:  %START_TIME%%RS%
    echo %DM% ended:    %END_TIME%%RS%
    if "%LOG_SESSIONS%"=="true" (
        echo %DM% log: %LOG_FILE%%RS%
    )
    goto :done
)

echo %RD% [!] exited with code %EXIT_CODE%%RS%

if %attempt% lss %MAX_RETRIES% (
    echo  trying again in %RETRY_DELAY% seconds...
    timeout /t %RETRY_DELAY% /nobreak >nul
    goto :retry
)

echo %RD% [X] %MAX_RETRIES% attempts. nothing.%RS%
echo  check auth or network.

:done
echo.
pause
exit /b 0
