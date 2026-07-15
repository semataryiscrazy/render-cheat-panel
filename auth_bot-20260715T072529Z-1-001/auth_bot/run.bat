@echo off
title Auth Bot — Nyx Project
cd /d "%~dp0"

:: Check for Python
where python >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Python not found. Install Python 3.10+ from https://python.org
    pause
    exit /b 1
)

:: Check for config token
python -c "import json; cfg=json.load(open('config.json')); assert cfg['token'] != 'DISCORD_TOKEN_AQUI', 'Token not configured'; print('[OK] token configured')" 2>nul
if %errorlevel% neq 0 (
    echo [!] Discord token not configured yet.
    echo     Run: python setup.py
    echo     Or edit config.json directly.
    echo.
    choice /C YN /M "Run setup now?"
    if !errorlevel! equ 1 (
        python setup.py
    ) else (
        echo [!] Continuing without token — bot will fail to connect.
    )
)

:: Run the bot
echo [*] Starting Auth Bot...
python auth_bot.py
if %errorlevel% neq 0 (
    echo [FAILED] Bot crashed with error %errorlevel%
    pause
)
