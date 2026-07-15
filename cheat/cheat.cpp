#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <winhttp.h>
#include <atomic>
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>
#include "web_server.h"
#include "options.hpp"
#include "memory.h"

#pragma comment(lib, "winhttp.lib")

WebServer* g_server = nullptr;
std::atomic<bool> g_running{ false };

// ─── remote panel config ──────────────────────────────────────
static const char* REMOTE_PANEL_HOST = "render-cheat-panel.onrender.com";
static const int   REMOTE_PANEL_PORT = 443;
static const bool  REMOTE_PANEL_HTTPS = true;
static const int   REMOTE_POLL_MS = 3000;

// ─── game process handle ──────────────────────────────────────
static HANDLE hGameProcess = NULL;
static DWORD  game_pid = 0;

// ─── achar processo do jogo (Free Fire no BlueStacks) ────────
static bool find_game() {
    if (hGameProcess) return true;

    // BlueStacks executa Android via aow.exe
    const char* candidates[] = {
        "aow.exe",
        "HD-Service.exe",
        "VBoxHeadless.exe",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        game_pid = find_process(candidates[i]);
        if (game_pid) {
            hGameProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, game_pid);
            if (hGameProcess) return true;
        }
    }
    return false;
}

// ─── achar base da Unity no processo ─────────────────────────
// Procura por padrao de classe Unity no modulo principal
static ULONG64 find_unity_base() {
    // procura o modulo principal do Android (libunity.so ou similar)
    ULONG64 base = get_module_base(game_pid, "libunity.so");
    if (base) return base;

    // fallback: procura qualquer modulo com padrao de assembly
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, game_pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32 me = { sizeof(MODULEENTRY32) };
    ULONG64 result = 0;
    if (Module32First(snap, &me)) do {
        // procura libunity ou libil2cpp
        if (strstr(me.szModule, "libunity") || strstr(me.szModule, "libil2cpp")) {
            result = (ULONG64)me.modBaseAddr;
            break;
        }
    } while (Module32Next(snap, &me));
    CloseHandle(snap);
    return result;
}

// ─── ler ponteiro com offsets (multi-level) ───────────────────
static ULONG64 read_ptr(HANDLE hProc, ULONG64 base, const std::vector<ULONG64>& offsets) {
    ULONG64 addr = base;
    for (size_t i = 0; i < offsets.size(); i++) {
        if (!read_memory(hProc, addr, &addr, sizeof(addr)))
            return 0;
        addr += offsets[i];
    }
    return addr;
}

// ─── WinHTTP GET helper ──────────────────────────────────────
static std::string http_get(const char* host, int port, const char* path, bool https) {
    std::string result;
    HINTERNET hSession = WinHttpOpen(L"CheatLoader/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return result;

    int hlen = MultiByteToWideChar(CP_UTF8, 0, host, -1, NULL, 0);
    wchar_t* whost = new wchar_t[hlen];
    MultiByteToWideChar(CP_UTF8, 0, host, -1, whost, hlen);

    HINTERNET hConnect = WinHttpConnect(hSession, whost, port, 0);
    delete[] whost;
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

    int plen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    wchar_t* wpath = new wchar_t[plen];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, plen);

    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wpath, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    delete[] wpath;
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result; }

    if (https) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL)) {
        char buffer[4096];
        DWORD bytesRead = 0;
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = 0;
            result += buffer;
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

// ─── remote poll: sincroniza opcoes do painel Render ─────────
static void sync_from_panel(const std::string& resp) {
    // helper: acha "k":"<needle>" e retorna o valor de "v"
    auto find_val = [&](const std::string& haystack, const std::string& needle) -> std::string {
        size_t kp = haystack.find("\"k\":\"" + needle + "\"");
        if (kp == std::string::npos) return "";
        size_t vp = haystack.find("\"v\":", kp);
        if (vp == std::string::npos) return "";
        vp += 4;
        if (vp >= haystack.size()) return "";
        if (haystack[vp] == '"') {
            vp++;
            size_t end = haystack.find("\"", vp);
            if (end == std::string::npos) return "";
            return haystack.substr(vp, end - vp);
        }
        size_t end = haystack.find_first_of(",}", vp);
        if (end == std::string::npos) return haystack.substr(vp);
        return haystack.substr(vp, end - vp);
    };
    auto find_bool = [&](const std::string& h, const std::string& n) -> bool {
        std::string v = find_val(h, n);
        return (v == "true" || v == "1");
    };
    auto find_float = [&](const std::string& h, const std::string& n) -> float {
        std::string v = find_val(h, n);
        return v.empty() ? 0.0f : (float)atof(v.c_str());
    };
    auto find_int = [&](const std::string& h, const std::string& n) -> int {
        std::string v = find_val(h, n);
        return v.empty() ? 0 : atoi(v.c_str());
    };

    auto& o = g_Options;

    // secao AimBot
    size_t a = resp.find("\"AimBot\"");
    if (a != std::string::npos) {
        std::string s = resp.substr(a, 1200);
        o.LegitBot.AimBot.Enabled          = find_bool(s, "Enabled");
        o.LegitBot.AimBot.Precision        = find_bool(s, "Precision");
        o.LegitBot.AimBot.ScopeTracking    = find_bool(s, "ScopeTracking");
        o.LegitBot.AimBot.Pixel            = find_bool(s, "Pixel");
        o.LegitBot.AimBot.IgnoreKnocked    = find_bool(s, "IgnoreKnocked");
        o.LegitBot.AimBot.TargetNPC        = find_bool(s, "TargetNPC");
        o.LegitBot.AimBot.PullCamAfterKill = find_bool(s, "PullCamAfterKill");
        o.LegitBot.AimBot.MaxDistance      = find_int(s, "MaxDistance");
        o.LegitBot.AimBot.DelayMode        = find_int(s, "DelayMode");
        o.LegitBot.AimBot.AimType          = find_int(s, "AimType");
        o.LegitBot.AimBot.FOV              = find_float(s, "FOV");
        o.LegitBot.AimBot.SmoothHorizontal = find_float(s, "SmoothH");
        o.LegitBot.AimBot.SmoothVertical   = find_float(s, "SmoothV");
        o.LegitBot.AimBot.ColliderFOV      = find_float(s, "ColliderFOV");
        o.LegitBot.AimBot.ColliderVisibleOnly = find_bool(s, "ColliderVisibleOnly");
        o.LegitBot.AimBot.PullCamHeight    = find_float(s, "PullCamHeight");
    }

    // secao ESP Players
    size_t e = resp.find("\"ESP Players\"");
    if (e != std::string::npos) {
        std::string s = resp.substr(e, 1200);
        auto& p = o.Visuals.ESP.Players;
        p.Enabled         = find_bool(s, "Enabled");
        p.ShowLocalPlayer = find_bool(s, "ShowLocalPlayer");
        p.ShowNPCs        = find_bool(s, "ShowNPCs");
        p.Box             = find_bool(s, "Box");
        p.BoxStyle        = find_int(s, "BoxStyle");
        p.Skeleton        = find_bool(s, "Skeleton");
        p.Name            = find_bool(s, "Name");
        p.HealthBar       = find_bool(s, "HealthBar");
        p.HealthBarPos    = find_int(s, "HealthBarPos");
        p.ArmorBar        = find_bool(s, "ArmorBar");
        p.WeaponName      = find_bool(s, "WeaponName");
        p.Distance        = find_bool(s, "Distance");
        p.SnapLines       = find_bool(s, "SnapLines");
        p.SnapLinesPos    = find_int(s, "SnapLinesPos");
        p.RenderDistance  = find_int(s, "RenderDistance");
    }

    // secao Exploits
    size_t x = resp.find("\"Exploits\"");
    if (x != std::string::npos) {
        std::string s = resp.substr(x, 1200);
        o.Exploits.NoRecoil             = find_bool(s, "NoRecoil");
        o.Exploits.NoRecoilStrength     = find_float(s, "NoRecoilStrength");
        o.Exploits.WeaponAttributes     = find_bool(s, "WeaponAttributes");
        o.Exploits.WeaponAttributesLevel= find_int(s, "WeaponAttributesLevel");
        o.Exploits.SpinBot              = find_bool(s, "SpinBot");
        o.Exploits.SpinBotSpeed         = find_float(s, "SpinBotSpeed");
        o.Exploits.SpeedHack            = find_bool(s, "SpeedHack");
        o.Exploits.SpeedMultiplier      = find_float(s, "SpeedMultiplier");
        o.Exploits.FastMedkit           = find_bool(s, "FastMedkit");
        o.Exploits.FastReload           = find_bool(s, "FastReload");
        o.Exploits.SilentAimEnabled     = find_bool(s, "SilentAimEnabled");
        o.Exploits.SilentAimUseFOV      = find_bool(s, "SilentAimUseFOV");
        o.Exploits.SilentAimFOV         = find_float(s, "SilentAimFOV");
        o.Exploits.SilentAimDistance    = find_int(s, "SilentAimDistance");
        o.Exploits.SilentAimHitBox      = find_int(s, "SilentAimHitBox");
        o.Exploits.SilentAimTargetBots  = find_bool(s, "SilentAimTargetBots");
    }

    // secao General
    size_t g = resp.find("\"General\"");
    if (g != std::string::npos) {
        std::string s = resp.substr(g, 800);
        o.General.CaptureBypass  = find_bool(s, "CaptureBypass");
        o.General.StreamMode     = find_bool(s, "StreamMode");
        o.General.BypassEnabled  = find_bool(s, "BypassEnabled");
        // MenuToggleKey nao vem do painel, mantem o default (VK_INSERT)
    }
}

void remote_poll_thread() {
    Sleep(5000); // espera estabilizar
    while (g_running) {
        std::string resp = http_get(REMOTE_PANEL_HOST, REMOTE_PANEL_PORT,
            "/api/poll", REMOTE_PANEL_HTTPS);
        if (!resp.empty()) {
            sync_from_panel(resp);
        }
        Sleep(REMOTE_POLL_MS);
    }
}

// ═══════════════════════════════════════════════════════════════
// FEATURES — Free Fire (BlueStacks)
// ═══════════════════════════════════════════════════════════════

// ─── Aimbot ────────────────────────────────────────────────────
// Le a lista de entidades do jogo e ajusta a mira para o alvo mais proximo
void aimbot_thread() {
    Sleep(3000); // espera o jogo carregar entidades
    while (g_running) {
        Sleep(5);
        if (!g_Options.LegitBot.AimBot.Enabled) continue;
        if (!find_game()) continue;

        // le ponteiros base do jogo (Unity IL2CPP)
        // InitBase -> GameVariables -> CurrentMatch -> DictionaryEntities
        ULONG64 initBase = 0;
        if (!read_memory(hGameProcess, (ULONG64)0xAEFCEC8, &initBase, sizeof(initBase)))
            continue;
        // ...
        // (implementacao especifica do jogo usando os offsets)
        // Esta thread serve como template — o usuario ajusta os
        // offsets e a logica para o jogo especifico
        Sleep(100);
    }
}

// ─── ESP ───────────────────────────────────────────────────────
// Requer overlay (DirectX/GDI) para desenhar na tela
// Implementacao completa disponivel em External space lixo/
void esp_thread() {
    while (g_running) {
        Sleep(50);
        if (!g_Options.Visuals.ESP.Players.Enabled) continue;
        // ESP depende de overlay window + DirectX
        // vide External space lixo/ para implementacao completa
        Sleep(100);
    }
}

// ─── No Recoil ─────────────────────────────────────────────────
// Patch na memoria da arma para eliminar/reduzir recoil
void norecoil_thread() {
    while (g_running) {
        Sleep(10);
        if (!g_Options.Exploits.NoRecoil) continue;
        if (!find_game()) continue;

        // NoRecoil via PlayerAttributes (Free Fire)
        // offset WeaponRecoil = 0xC no WeaponData
        // escreve 0.0f no recoil da arma atual
        //
        // Exemplo conceitual:
        // ULONG64 weaponAddr = read_ptr(hGameProcess, playerBase, {0x3E8, 0x58, 0xC});
        // float zero = 0.0f;
        // write_memory(hGameProcess, weaponAddr, &zero, sizeof(zero));

        Sleep(50);
    }
}

// ─── Speed Hack ────────────────────────────────────────────────
// Aumenta velocidade de movimento do jogador
void speedhack_thread() {
    while (g_running) {
        Sleep(50);
        if (!g_Options.Exploits.SpeedHack) continue;
        if (!find_game()) continue;

        // SpeedHack via PlayerAttributes.RunSpeedUpScale (offset 0x1C8)
        // escreve o multiplicador na memoria do jogador
        //
        // float speed = g_Options.Exploits.SpeedMultiplier; // 1.0 a 10.0
        // ULONG64 attrAddr = read_ptr(hGameProcess, playerBase, {0x4B0});
        // write_memory(hGameProcess, attrAddr + 0x1C8, &speed, sizeof(speed));

        Sleep(100);
    }
}

// ─── Fast Medkit ───────────────────────────────────────────────
void fastmedkit_thread() {
    while (g_running) {
        Sleep(100);
        if (!g_Options.Exploits.FastMedkit) continue;
        if (!find_game()) continue;

        // FastMedkit: patch PlayerAttributes.m_FSModeUseMedikitFasterRate
        // float fast = 999.0f;
        // write_memory(hGameProcess, attrAddr + 0x8C, &fast, sizeof(fast));

        Sleep(200);
    }
}

// ─── Fast Reload ───────────────────────────────────────────────
void fastreload_thread() {
    while (g_running) {
        Sleep(100);
        if (!g_Options.Exploits.FastReload) continue;
        if (!find_game()) continue;

        // FastReload: patch PlayerAttributes.FireIntervalScale
        // float fast = 0.01f; // quase instantaneo
        // write_memory(hGameProcess, attrAddr + 0x184, &fast, sizeof(fast));

        Sleep(200);
    }
}

// ─── Silent Aim ────────────────────────────────────────────────
void silentaim_thread() {
    while (g_running) {
        Sleep(5);
        if (!g_Options.Exploits.SilentAimEnabled) continue;
        if (!find_game()) continue;

        // Silent Aim: modifica LastAimingInfoFromWeaponAdjusted
        // para redirecionar tiros para o alvo mais proximo
        //
        // Usa AimRotation (0x3F4) para forcar direcao do tiro
        // sem mexer na camera do jogador

        Sleep(20);
    }
}

// ─── stop ──────────────────────────────────────────────────────
void stop_all() {
    g_running = false;
    Sleep(500);
    if (hGameProcess) {
        CloseHandle(hGameProcess);
        hGameProcess = NULL;
    }
}

// ─── run_services ─────────────────────────────────────────────
static void run_services() {
    g_server = new WebServer(8080);
    if (g_server->start()) {
        // servidor web local rodando
    }

    g_running = true;

    // inicia threads de features
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)aimbot_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)esp_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)norecoil_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)speedhack_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)fastmedkit_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)fastreload_thread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)silentaim_thread, nullptr, 0, nullptr);

    // thread de polling do painel remoto
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)remote_poll_thread, nullptr, 0, nullptr);
}

// ═══════════════════════════════════════════════════════════════
// DLL Entry Point
// ═══════════════════════════════════════════════════════════════

DWORD WINAPI main_thread(LPVOID lpParam) {
    Sleep(2000); // espera o processo alvo estabilizar
    __try {
        run_services();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // nao derruba o processo
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
        stop_all();
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
