// Unloader - servidor web standalone com botao de unload
// Compilar: cl.exe unloader.cpp /Fe:unloader.exe /link ws2_32.lib
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

const char* HTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Unloader</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#0a0a0f;color:#e0e0e0;min-height:100vh;display:flex;justify-content:center;align-items:center}
.card{background:#13131a;border-radius:16px;padding:2rem;width:360px;box-shadow:0 0 40px rgba(255,60,60,0.1);border:1px solid #1e1e2a;text-align:center}
h1{font-size:1.4rem;font-weight:600;margin-bottom:0.5rem;color:#ff4444}
p{color:#666;font-size:0.85rem;margin-bottom:2rem}
.btn{width:100%;padding:12px;border:none;border-radius:8px;cursor:pointer;font-size:14px;font-weight:500;transition:all .25s;margin-bottom:8px}
.btn-unload{background:rgba(255,60,60,0.15);color:#ff4444;border:1px solid rgba(255,60,60,0.2)}
.btn-unload:hover{background:rgba(255,60,60,0.25)}
.btn-unload:disabled{opacity:0.4;cursor:not-allowed}
.btn-status{width:100%;padding:8px;border-radius:6px;font-size:12px;margin-top:12px;display:none}
.btn-status.show{display:block}
.status-ok{background:rgba(0,214,115,0.1);color:#00d673;border:1px solid rgba(0,214,115,0.2)}
.status-err{background:rgba(255,60,60,0.1);color:#ff4444;border:1px solid rgba(255,60,60,0.2)}
.ip-info{font-size:11px;color:#444;margin-top:16px;padding-top:12px;border-top:1px solid #1e1e2a}
</style>
</head>
<body>
<div class="card">
<h1>&#9889; Unloader</h1>
<p>unload total da Client.dll</p>
<button class="btn btn-unload" id="unloadBtn" onclick="unload()">UNLOAD DLL</button>
<div id="status" class="btn-status"></div>
<div class="ip-info" id="ipInfo"></div>
</div>
<script>
async function unload(){
 const btn=document.getElementById('unloadBtn');
 const st=document.getElementById('status');
 btn.disabled=true;btn.textContent='Unloading...';
 st.className='btn-status';
 try{
  const r=await fetch('/unload',{method:'POST'});
  const d=await r.json();
  st.className='btn-status show '+(d.success?'status-ok':'status-err');
  st.textContent=d.success?'DLL removida com sucesso!':'Falha: '+d.error;
  if(d.success) btn.textContent='Unloaded';
  else{btn.disabled=false;btn.textContent='UNLOAD DLL';}
 }catch(e){
  st.className='btn-status show status-err';
  st.textContent='Erro de conexao';
  btn.disabled=false;btn.textContent='UNLOAD DLL';
 }
}
fetch('/ip').then(r=>r.text()).then(ip=>{
 document.getElementById('ipInfo').textContent='servidor: '+ip;
});
</script>
</body>
</html>
)rawliteral";

SOCKET server_socket = INVALID_SOCKET;
std::atomic<bool> running{ false };

std::string handle_request(const std::string& method, const std::string& path) {
    if (path == "/" || path == "/index.html") {
        return HTML;
    }
    else if (path == "/ip") {
        char host[256] = {0};
        gethostname(host, sizeof(host));
        return "http://" + std::string(host) + ":8081";
    }
    else if (path == "/unload" && method == "POST") {
        HMODULE hMod = GetModuleHandleA("Client.dll");
        if (!hMod) {
            // tenta achar por outros nomes
            hMod = GetModuleHandleA("cheat_web.dll");
        }
        if (!hMod) {
            return "{\"success\":false,\"error\":\"DLL nao encontrada\"}";
        }
        std::thread([hMod]() {
            Sleep(300);
            FreeLibraryAndExitThread(hMod, 0);
        }).detach();
        return "{\"success\":true}";
    }
    return "{\"error\":\"not found\"}";
}

void server_loop() {
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        int activity = select(0, &read_fds, nullptr, nullptr, &timeout);
        if (activity <= 0) continue;

        sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client = accept(server_socket, (sockaddr*)&client_addr, &addr_len);
        if (client == INVALID_SOCKET) continue;

        char buffer[4096] = {0};
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (bytes > 0) {
            std::string request(buffer);
            size_t first_space = request.find(' ');
            size_t second_space = request.find(' ', first_space + 1);
            std::string method, path;
            if (first_space != std::string::npos && second_space != std::string::npos) {
                method = request.substr(0, first_space);
                path = request.substr(first_space + 1, second_space - first_space - 1);
            }

            std::string response_body = handle_request(method, path);
            std::string content_type = path.find("/api") == 0 ? "application/json" : "text/html; charset=utf-8";

            std::stringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Access-Control-Allow-Origin: *\r\n";
            response << "Content-Type: " << content_type << "\r\n";
            response << "Content-Length: " << response_body.length() << "\r\n";
            response << "Connection: close\r\n";
            response << "\r\n";
            response << response_body;

            std::string resp_str = response.str();
            send(client, resp_str.c_str(), resp_str.length(), 0);
        }
        closesocket(client);
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) return 1;

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8081);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(server_socket);
        WSACleanup();
        printf("[-] Porta 8081 ocupada\n");
        return 1;
    }

    listen(server_socket, 5);
    running = true;

    printf("[+] Unloader rodando em http://localhost:8081\n");
    printf("[+] No celular: http://192.168.1.4:8081\n");

    server_loop();

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
