#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <winhttp.h>
#include <atomic>
#include <string>
#include <sstream>
#include <cstring>
#include "server.h"
#include "config.h"
#include "memory.h"

#pragma comment(lib, "winhttp.lib")

HttpServer* g_server = nullptr;
std::atomic<bool> g_running{ false };
HANDLE g_feature_threads[8] = {};

// ─── remote panel config ──────────────────────────────────────
static const char* REMOTE_PANEL_HOST = "render-cheat-panel.onrender.com";
static const int   REMOTE_PANEL_PORT = 443;
static const bool  REMOTE_PANEL_HTTPS = true;
static const int   REMOTE_POLL_MS = 3000; // poll a cada 3s

// ─── features threads ────────────────────────────────────────
void aimbot_thread() {
    while (g_running) {
        Sleep(5);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.aimbot;
        }
        if (enabled) {
            // logica do aimbot aqui
        }
    }
}

void esp_thread() {
    while (g_running) {
        Sleep(50);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.esp;
        }
        if (enabled) {
            // logica do esp aqui
        }
    }
}

void wallhack_thread() {
    while (g_running) {
        Sleep(100);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.wallhack;
        }
        if (enabled) {
            // logica do wallhack aqui
        }
    }
}

void triggerbot_thread() {
    while (g_running) {
        Sleep(5);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.triggerbot;
        }
        if (enabled) {
            // logica do triggerbot aqui
        }
    }
}

// ─── no recoil via memoria ───────────────────────────────────
// Procura o processo do jogo dentro do emulador e faz patch
// sem criar arquivos, sem logs, sem padrões detectaveis.
//
// Processos comuns no BlueStacks:
//   - aow.exe (Android on Windows)
//   - HD-Service.exe
//   - VBoxHeadless.exe
//   - ou o nome do processo Android do jogo
//
// O offset e o patch variam por jogo — aqui vai um template
// funcional que o usuario adapta com os offsets corretos.

static HANDLE hGameProcess = NULL;
static DWORD  game_pid = 0;

static void find_game_process() {
    if (hGameProcess) return;  // ja achou
	
    // tentar processos comuns do Android/emu
    const char* candidates[] = {
        "aow.exe",           // BlueStacks Android
        "HD-Service.exe",    // BlueStacks service
        "VBoxHeadless.exe",  // VirtualBox backend
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        game_pid = find_process(candidates[i]);
        if (game_pid) {
            hGameProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, game_pid);
            if (hGameProcess) break;
        }
    }
}

void norecoil_thread() {
    while (g_running) {
        Sleep(10);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.noRecoil;
        }
        
        if (enabled && !hGameProcess) {
            find_game_process();
        }
        
        if (enabled && hGameProcess) {
            // ─── AQUI: patch do no-recoil ───────────────────────
            // Exemplo: escrever bytes NOP (0x90) num offset especifico
            // da memoria do processo do jogo.
            //
            // ULONG64 recoil_offset = 0x12345678; // <-- trocar pelo offset real
            // BYTE nop[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
            // write_memory(hGameProcess, recoil_offset, nop, sizeof(nop));
            
            // placeholder: so existe pra mostrar que a thread roda
            Sleep(100);
        }
    }
}

void speedhack_thread() {
    while (g_running) {
        Sleep(50);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.speedhack;
        }
        if (enabled) {
            // logica do speedhack aqui
        }
    }
}

void godmode_thread() {
    while (g_running) {
        Sleep(100);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.godMode;
        }
        if (enabled) {
            // logica do godmode aqui
        }
    }
}

void radar_thread() {
    while (g_running) {
        Sleep(200);
        bool enabled = false;
        {
            std::lock_guard<std::mutex> lock(g_config_mutex);
            enabled = g_config.radar;
        }
        if (enabled) {
            // logica do radar aqui
        }
    }
}

void stop_feature_threads() {
    g_running = false;
    Sleep(300); // espera as threads sairem dos Sleep()
    for (int i = 0; i < 8; i++) {
        if (g_feature_threads[i]) {
            WaitForSingleObject(g_feature_threads[i], 1000);
            CloseHandle(g_feature_threads[i]);
            g_feature_threads[i] = NULL;
        }
    }
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

// ─── localiza string value num JSON simples sem parser ─────
static std::string json_find_str(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.length();
    // skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        // quoted string
        pos++;
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }
    // bool or number
    size_t end = json.find_first_of(",}]", pos);
    if (end == std::string::npos) return json.substr(pos);
    return json.substr(pos, end - pos);
}

// ─── remote poll thread: busca opcoes do painel Render ──────
void remote_poll_thread() {
    // espera o servidor local estabilizar e o jogo carregar
    Sleep(5000);

    while (g_running) {
        // faz GET no /api/poll do Render
        std::string resp = http_get(REMOTE_PANEL_HOST, REMOTE_PANEL_PORT,
            "/api/poll", REMOTE_PANEL_HTTPS);

        if (!resp.empty()) {
            // busca "changed" e "options"
            std::string changed = json_find_str(resp, "changed");
            if (changed == "true" || changed == "1") {
                // helper: dentro de um trecho json, acha "k":"<needle>" e le o "v": depois
                auto find_bool_in = [&](const std::string& haystack, const std::string& needle) -> bool {
                    size_t kp = haystack.find("\"k\":\"" + needle + "\"");
                    if (kp == std::string::npos) return false;
                    size_t vp = haystack.find("\"v\":", kp);
                    if (vp == std::string::npos) return false;
                    vp += 4; // skip \"v\":
                    return (haystack.find("true", vp) < haystack.find_first_of(",}", vp));
                };

                {
                    std::lock_guard<std::mutex> lock(g_config_mutex);

                    // secao AimBot -> g_config.aimbot
                    size_t aimSec = resp.find("\"AimBot\"");
                    if (aimSec != std::string::npos) {
                        std::string sec = resp.substr(aimSec, 800);
                        g_config.aimbot = find_bool_in(sec, "Enabled");
                    }

                    // secao ESP Players -> g_config.esp
                    size_t espSec = resp.find("\"ESP Players\"");
                    if (espSec != std::string::npos) {
                        std::string sec = resp.substr(espSec, 800);
                        g_config.esp = find_bool_in(sec, "Enabled");
                    }

                    // secoes Exploits -> opcoes diretas
                    size_t expSec = resp.find("\"Exploits\"");
                    if (expSec != std::string::npos) {
                        std::string sec = resp.substr(expSec, 800);
                        g_config.noRecoil = find_bool_in(sec, "NoRecoil");
                        g_config.speedhack = find_bool_in(sec, "SpeedHack");
                    }
                }
            }
        }

        Sleep(REMOTE_POLL_MS);
    }
}

// helper sem C++ objects no escopo do __try
static void run_services() {
    g_server = new HttpServer(8080);
    if (g_server->start()) {
        // painel web interno rodando na porta que deu certo
    }
    
    g_running = true;
    g_feature_threads[0] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)aimbot_thread, nullptr, 0, nullptr);
    g_feature_threads[1] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)esp_thread, nullptr, 0, nullptr);
    g_feature_threads[2] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)wallhack_thread, nullptr, 0, nullptr);
    g_feature_threads[3] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)triggerbot_thread, nullptr, 0, nullptr);
    g_feature_threads[4] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)norecoil_thread, nullptr, 0, nullptr);
    g_feature_threads[5] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)speedhack_thread, nullptr, 0, nullptr);
    g_feature_threads[6] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)godmode_thread, nullptr, 0, nullptr);
    g_feature_threads[7] = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)radar_thread, nullptr, 0, nullptr);
    // thread de polling do painel remoto (nao precisa de handle pois termina com g_running)
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)remote_poll_thread, nullptr, 0, nullptr);
}

DWORD WINAPI main_thread(LPVOID lpParam) {
    // delay pra dar tempo do processo estabilizar
    Sleep(2000);
    
    __try {
        run_services();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // se algo crashar, não derruba o processo
    }
    
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // cria a thread principal com delay
        HANDLE hThread = CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        // PRIMEIRO: para as feature threads (evita crash por memoria liberada)
        stop_feature_threads();
        
        // SEGUNDO: para o servidor web
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
