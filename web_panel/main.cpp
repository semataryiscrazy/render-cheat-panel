#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

#include "server.h"
#include "injector.h"
#include "downloader.h"

// ─── CONFIG ─────────────────────────────────────────────────────
const std::string TARGET   = "HD-Player.exe";
const std::string DLL_URL  = "https://render-cheat-panel.onrender.com/api/loader/dll/cheat.dll";  // URL da DLL no panel
const std::string DLL_PATH = "cheat.dll";
const int PORT = 8080;

// ─── JSON SIMPLES (sem dependências) ────────────────────────────
std::string json_string(const std::string& key, const std::string& val) {
    return "\"" + key + "\":\"" + val + "\"";
}

std::string json_bool(const std::string& key, bool val) {
    return "\"" + key + "\":" + (val ? "true" : "false");
}

std::string json_int(const std::string& key, int val) {
    return "\"" + key + "\":" + std::to_string(val);
}

std::string build_json(const std::vector<std::string>& pairs) {
    std::string r = "{";
    for (size_t i = 0; i < pairs.size(); i++) {
        r += pairs[i];
        if (i < pairs.size() - 1) r += ",";
    }
    r += "}";
    return r;
}

// ─── ESTADO GLOBAL ──────────────────────────────────────────────
bool dll_ready    = false;
bool injected     = false;
bool downloading  = false;
std::string log_msg = "[system] aguardando servidor...";

void set_log(const std::string& msg) { log_msg = msg; }

// ─── HANDLER DE REQUISIÇÕES ─────────────────────────────────────
std::string handle_request(const std::string& method, const std::string& path, const std::string& body) {
    // ── servir arquivos estáticos ──
    if (path == "/" || path.empty()) {
        std::string html = read_file_bin("web\\index.html");
        if (html.empty()) return build_http_response(404, "text/plain", "index.html nao encontrado");
        return build_http_response(200, "text/html", html);
    }

    if (path == "/style.css") {
        std::string css = read_file_bin("web\\style.css");
        if (css.empty()) return build_http_response(404, "text/plain", "style.css nao encontrado");
        return build_http_response(200, "text/css", css);
    }

    if (path == "/script.js") {
        std::string js = read_file_bin("web\\script.js");
        if (js.empty()) return build_http_response(404, "text/plain", "script.js nao encontrado");
        return build_http_response(200, "application/javascript", js);
    }

    // ── API ──
    if (path == "/api/status" && method == "GET") {
        DWORD pid = get_pid(TARGET.c_str());
        std::vector<std::string> pairs = {
            json_bool("dll_ready", dll_ready),
            json_bool("injected", injected),
            json_bool("downloading", downloading),
            json_string("target", TARGET),
            json_int("pid", (int)pid),
            json_string("log", log_msg)
        };
        return build_http_response(200, "application/json", build_json(pairs));
    }

    if (path == "/api/download" && method == "POST") {
        if (downloading) {
            return build_http_response(200, "application/json",
                build_json({json_string("status", "already_downloading")}));
        }
        if (dll_ready) {
            return build_http_response(200, "application/json",
                build_json({json_string("status", "already_downloaded")}));
        }
        downloading = true;
        dll_ready = false;
        set_log("[system] baixando dll...");
        std::thread([&]() {
            bool ok = download_file(DLL_URL, DLL_PATH);
            dll_ready = ok;
            downloading = false;
            if (ok) set_log("[system] dll baixada com sucesso");
            else    set_log("[system] erro no download — url invalida?");
        }).detach();
        return build_http_response(200, "application/json",
            build_json({json_string("status", "downloading")}));
    }

    if (path == "/api/inject" && method == "POST") {
        if (!dll_ready) {
            return build_http_response(200, "application/json",
                build_json({json_string("status", "dll_not_ready")}));
        }
        DWORD pid = get_pid(TARGET.c_str());
        if (!pid) {
            set_log("[system] processo " + TARGET + " nao encontrado");
            return build_http_response(200, "application/json",
                build_json({json_string("status", "process_not_found")}));
        }
        bool ok = inject_dll(pid, DLL_PATH);
        injected = ok;
        if (ok) set_log("[system] dll injetada em " + TARGET + " (PID: " + std::to_string(pid) + ")");
        else    set_log("[system] falha na injecao");
        return build_http_response(200, "application/json",
            build_json({json_string("status", ok ? "injected" : "failed")}));
    }

    return build_http_response(404, "text/plain", "not found");
}

// ─── SERVIDOR ────────────────────────────────────────────────────
void run_server() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        MessageBoxA(NULL, "WSAStartup failed", "Error", MB_ICONERROR);
        return;
    }

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        MessageBoxA(NULL, "socket failed", "Error", MB_ICONERROR);
        WSACleanup();
        return;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        MessageBoxA(NULL, "bind failed — porta em uso?", "Error", MB_ICONERROR);
        closesocket(server_sock);
        WSACleanup();
        return;
    }

    listen(server_sock, 5);
    set_log("[system] servidor rodando em http://localhost:" + std::to_string(PORT));

    while (true) {
        sockaddr_in client;
        int client_size = sizeof(client);
        SOCKET client_sock = accept(server_sock, (sockaddr*)&client, &client_size);
        if (client_sock == INVALID_SOCKET) continue;

        char buf[16384] = {};
        int received = recv(client_sock, buf, sizeof(buf) - 1, 0);
        if (received > 0) {
            buf[received] = '\0';
            std::string raw(buf);

            std::string method, path, body;
            parse_request(raw, method, path, body);

            std::string response = handle_request(method, path, body);
            send(client_sock, response.c_str(), response.size(), 0);
        }

        closesocket(client_sock);
    }

    closesocket(server_sock);
    WSACleanup();
}

// ─── ENTRY POINT ─────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // roda escondido — sem console window
    run_server();
    return 0;
}
