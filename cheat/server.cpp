#include "server.h"
#include "config.h"
#include <sstream>
#include <algorithm>

static const char* WEBUI_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Cheat Remote Panel</title>
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
        font-family: 'Segoe UI', system-ui, sans-serif;
        background: #0a0a0f;
        color: #e0e0e0;
        min-height: 100vh;
        display: flex;
        justify-content: center;
        align-items: center;
    }
    .container {
        background: #13131a;
        border-radius: 16px;
        padding: 2rem;
        width: 480px;
        box-shadow: 0 0 40px rgba(0, 100, 255, 0.1);
        border: 1px solid #1e1e2a;
    }
    h1 {
        font-size: 1.5rem;
        font-weight: 600;
        margin-bottom: 0.25rem;
        background: linear-gradient(135deg, #00b4ff, #7b2ffc);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
    }
    .subtitle {
        color: #666;
        font-size: 0.85rem;
        margin-bottom: 1.5rem;
        padding-bottom: 1rem;
        border-bottom: 1px solid #1e1e2a;
    }
    .feature-grid {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
    .feature {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 0.75rem 1rem;
        background: #1a1a24;
        border-radius: 10px;
        transition: background 0.2s;
    }
    .feature:hover { background: #22222e; }
    .feature-name {
        font-weight: 500;
        text-transform: capitalize;
    }
    .feature-status {
        font-size: 0.75rem;
        padding: 0.2rem 0.6rem;
        border-radius: 20px;
        background: #2a2a3a;
        color: #888;
        margin-right: 0.75rem;
    }
    .feature-status.on { background: #00b44a22; color: #00ff6a; }
    .feature-status.off { background: #b4000022; color: #ff4444; }
    .toggle {
        position: relative;
        width: 44px;
        height: 24px;
        cursor: pointer;
    }
    .toggle input {
        opacity: 0;
        width: 0;
        height: 0;
    }
    .toggle .slider {
        position: absolute;
        inset: 0;
        background: #2a2a3a;
        border-radius: 24px;
        transition: 0.3s;
    }
    .toggle .slider::before {
        content: '';
        position: absolute;
        height: 18px;
        width: 18px;
        left: 3px;
        bottom: 3px;
        background: #666;
        border-radius: 50%;
        transition: 0.3s;
    }
    .toggle input:checked + .slider { background: #00b44a; }
    .toggle input:checked + .slider::before { 
        transform: translateX(20px);
        background: #fff;
    }
    .footer {
        margin-top: 1.5rem;
        padding-top: 1rem;
        border-top: 1px solid #1e1e2a;
        display: flex;
        justify-content: space-between;
        align-items: center;
        font-size: 0.8rem;
        color: #555;
    }
    .status-dot {
        display: inline-block;
        width: 8px;
        height: 8px;
        border-radius: 50%;
        margin-right: 6px;
        animation: pulse 1.5s ease-in-out infinite;
    }
    .status-dot.connected { background: #00ff6a; }
    @keyframes pulse {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.4; }
    }
    .connection-status { display: flex; align-items: center; }
</style>
</head>
<body>
<div class="container">
    <h1>&#9889; Remote Panel</h1>
    <div class="subtitle">controle remoto do cheat</div>
    
    <div class="feature-grid" id="featureGrid">
        <div class="feature">
            <span class="feature-name">Aimbot</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-aimbot">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-aimbot" onchange="toggleFeature('aimbot')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="feature">
            <span class="feature-name">ESP</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-esp">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-esp" onchange="toggleFeature('esp')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="feature">
            <span class="feature-name">Wallhack</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-wallhack">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-wallhack" onchange="toggleFeature('wallhack')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="feature">
            <span class="feature-name">Triggerbot</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-triggerbot">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-triggerbot" onchange="toggleFeature('triggerbot')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="feature">
            <span class="feature-name">No Recoil</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-norecoil">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-norecoil" onchange="toggleFeature('norecoil')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="feature">
            <span class="feature-name">Speedhack</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-speedhack">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-speedhack" onchange="toggleFeature('speedhack')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="feature">
            <span class="feature-name">God Mode</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-godmode">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-godmode" onchange="toggleFeature('godmode')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="feature">
            <span class="feature-name">Radar</span>
            <div style="display:flex;align-items:center;">
                <span class="feature-status off" id="status-radar">off</span>
                <label class="toggle">
                    <input type="checkbox" id="toggle-radar" onchange="toggleFeature('radar')">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
    </div>
    
    <div class="footer">
        <div class="connection-status">
            <span class="status-dot connected" id="statusDot"></span>
            <span id="connectionLabel">conectado</span>
        </div>
        <span id="gameProcess">processo: &mdash;</span>
    </div>
</div>

<script>
const BASE = '';
let lastStatus = {};

async function toggleFeature(name) {
    try {
        const resp = await fetch(`${BASE}/toggle/${name}`);
        const data = await resp.json();
        updateUI(data);
    } catch(e) {
        console.error('erro:', e);
    }
}

async function refreshStatus() {
    try {
        const resp = await fetch(`${BASE}/status`);
        const data = await resp.json();
        updateUI(data);
        document.getElementById('connectionLabel').textContent = 'conectado';
        document.getElementById('statusDot').className = 'status-dot connected';
        if (data.process) {
            document.getElementById('gameProcess').textContent = `processo: ${data.process}`;
        }
    } catch(e) {
        document.getElementById('connectionLabel').textContent = 'desconectado';
        document.getElementById('statusDot').className = 'status-dot';
    }
}

function updateUI(data) {
    for (const [feature, enabled] of Object.entries(data)) {
        if (feature === 'process') continue;
        const toggle = document.getElementById(`toggle-${feature}`);
        const status = document.getElementById(`status-${feature}`);
        if (toggle) {
            toggle.checked = enabled;
        }
        if (status) {
            status.textContent = enabled ? 'on' : 'off';
            status.className = `feature-status ${enabled ? 'on' : 'off'}`;
        }
    }
}

setInterval(refreshStatus, 1000);
refreshStatus();
</script>
</body>
</html>
)rawliteral";

HttpServer::HttpServer(int port) : port(port), running(false) {
    server_socket = INVALID_SOCKET;
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;
    
    int ports[] = { port, 9090, 8081, 7070, 0 };
    
    for (int i = 0; ports[i] != 0; i++) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            continue;
        }
        
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(ports[i]);
        
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
            continue;
        }
        
        port = ports[i]; // porta que deu certo
        break;
    }
    
    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }
    
    listen(server_socket, 5);
    running = true;
    server_thread = std::thread(server_loop, this);
    
    return true;
}

void HttpServer::stop() {
    running = false;
    if (server_thread.joinable()) {
        server_thread.join();
    }
    if (server_socket != INVALID_SOCKET) {
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
    }
    WSACleanup();
}

void HttpServer::server_loop(HttpServer* server) {
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    while (server->running) {
        FD_ZERO(&read_fds);
        FD_SET(server->server_socket, &read_fds);
        
        int activity = select(0, &read_fds, nullptr, nullptr, &timeout);
        if (activity <= 0) continue;
        
        sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client = accept(server->server_socket, (sockaddr*)&client_addr, &addr_len);
        if (client == INVALID_SOCKET) continue;
        
        char buffer[4096] = {0};
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes > 0) {
            std::string request(buffer);
            std::string method, path;
            
            size_t first_space = request.find(' ');
            size_t second_space = request.find(' ', first_space + 1);
            if (first_space != std::string::npos && second_space != std::string::npos) {
                method = request.substr(0, first_space);
                path = request.substr(first_space + 1, second_space - first_space - 1);
            }
            
            std::string response_body = server->handle_request(method, path);
            std::stringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Access-Control-Allow-Origin: *\r\n";
            response << "Content-Type: text/html; charset=utf-8\r\n";
            response << "Content-Length: " << response_body.length() << "\r\n";
            response << "Connection: close\r\n";
            response << "\r\n";
            response << response_body;
            
            std::string resp_str = response.str();
            send(client, resp_str.c_str(), (int)resp_str.length(), 0);
        }
        
        closesocket(client);
    }
}

std::string HttpServer::handle_request(const std::string& method, const std::string& path) {
    if (path == "/" || path == "/index.html") {
        return serve_webui();
    }
    else if (path == "/status") {
        return serve_status();
    }
    else if (path.find("/toggle/") == 0) {
        std::string feature = path.substr(8);
        return serve_toggle(feature);
    }
    
    return "{\"error\":\"not found\"}";
}

std::string HttpServer::serve_webui() {
    return std::string(WEBUI_HTML);
}

std::string HttpServer::serve_status() {
    std::lock_guard<std::mutex> lock(g_config_mutex);
    std::stringstream json;
    json << "{";
    auto features = g_config.getFeatureMap();
    bool first = true;
    for (const auto& [name, ptr] : features) {
        if (!first) json << ",";
        json << "\"" << name << "\":" << (*ptr ? "true" : "false");
        first = false;
    }
    json << ",\"process\":\"injected\"";
    json << "}";
    return json.str();
}

std::string HttpServer::serve_toggle(const std::string& feature) {
    std::lock_guard<std::mutex> lock(g_config_mutex);
    auto features = g_config.getFeatureMap();
    
    std::string lower_feature = feature;
    std::transform(lower_feature.begin(), lower_feature.end(), lower_feature.begin(), ::tolower);
    
    if (features.find(lower_feature) != features.end()) {
        bool* ptr = features[lower_feature];
        *ptr = !(*ptr);
    }
    
    std::stringstream json;
    json << "{";
    bool first = true;
    for (const auto& [name, ptr] : features) {
        if (!first) json << ",";
        json << "\"" << name << "\":" << (*ptr ? "true" : "false");
        first = false;
    }
    json << "}";
    return json.str();
}
