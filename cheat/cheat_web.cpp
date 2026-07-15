#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include "web_server.h"
#include "options.hpp"

WebServer* g_server = nullptr;

// ─── threads dos recursos ─────────────────────────────────────
void aimbot_thread() {
    while (true) {
        Sleep(5);
        if (g_Options.LegitBot.AimBot.Enabled) {
            // logica do aimbot aqui
            // usar g_Options.LegitBot.AimBot.*
        }
    }
}

void esp_thread() {
    while (true) {
        Sleep(50);
        if (g_Options.Visuals.ESP.Players.Enabled) {
            // logica do esp aqui
            // usar g_Options.Visuals.ESP.Players.*
        }
    }
}

void exploits_thread() {
    while (true) {
        Sleep(10);
        if (g_Options.Exploits.NoRecoil) {
            // logica do norecoil
        }
        if (g_Options.Exploits.SpeedHack) {
            // logica do speedhack
        }
        if (g_Options.Exploits.SpinBot) {
            // logica do spinbot
        }
        if (g_Options.Exploits.FastMedkit) {
            // logica do fast medkit
        }
        if (g_Options.Exploits.FastReload) {
            // logica do fast reload
        }
        if (g_Options.Exploits.SilentAimEnabled) {
            // logica do silent aim
        }
        if (g_Options.Exploits.WeaponAttributes) {
            // logica do weapon attributes
        }
    }
}

void general_thread() {
    while (true) {
        Sleep(100);
        // logica de capture bypass, etc
    }
}

// helper chamado de dentro do __try — sem C++ objects no escopo do __try
static void run_services() {
    g_server = new WebServer(8080);
    if (g_server->start()) {
        // painel web interno rodando
    }

    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)aimbot_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)esp_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)exploits_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)general_thread, nullptr, 0, nullptr);
}

DWORD WINAPI main_thread(LPVOID lpParam) {
    // delay pra dar tempo do processo estabilizar
    Sleep(2000);

    __try {
        run_services();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // evita crash no processo
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        __try {
            if (g_server) {
                g_server->stop();
                delete g_server;
                g_server = nullptr;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // ignora erro no detach
        }
    }
    return TRUE;
}
