@echo off
title Lunar Web Injector
echo ============================================
echo  Lunar Network - Web Panel Injector
echo ============================================
echo.
echo Injetando cheat_web.dll no HD-Player.exe...
echo.
if exist injector_cli.exe (
    injector_cli.exe --process HD-Player.exe --dll cheat_web.dll
) else (
    injector.exe HD-Player.exe cheat_web.dll
)
echo.
if %errorlevel% equ 0 (
    echo Abrindo navegador...
    start http://localhost:8080
) else (
    echo Falha na injecao. Tente executar como administrador.
)
echo.
pause
