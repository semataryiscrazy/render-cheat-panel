#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <atomic>
#include "server.h"
#include "config.h"
#include "memory.h"

HttpServer* g_server = nullptr;
std::atomic<bool> g_running{ false };
HANDLE g_feature_threads[8] = {};

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
