@echo off
title building web panel — loader

echo.
echo  === web panel — hd-player loader ===
echo.

setlocal enabledelayedexpansion

:: verifica se o cl (visual studio) existe
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] visual studio build tools nao encontrados.
    echo.
    echo     instale pelo link:
    echo     https://visualstudio.microsoft.com/visual-cpp-build-tools/
    echo.
    echo     ou abra o "Developer Command Prompt for VS" e rode esse script.
    echo.
    pause
    exit /b 1
)

echo [*] compilando...
echo.

cl /EHsc /std:c++17 /O2 /W3 main.cpp /Fe:web_panel.exe /link ws2_32.lib wininet.lib user32.lib

if %errorlevel% equ 0 (
    echo.
    echo  [+] build concluido — web_panel.exe criado
    echo.
    echo     execute web_panel.exe e abra http://localhost:8080
    echo.
    echo     nao feche o terminal se quiser ver logs.
    echo     para fechar o servidor: Ctrl+C
    echo.
) else (
    echo.
    echo  [!] erro no build — verifique os warnings acima.
    echo.
)

pause
