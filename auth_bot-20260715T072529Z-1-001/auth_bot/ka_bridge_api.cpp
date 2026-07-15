#define _CRT_SECURE_NO_WARNINGS
#include "ka_bridge_api.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <windows.h>
#include <winhttp.h>
#include <iphlpapi.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "iphlpapi.lib")

static std::string g_user, g_err;
static long long g_expiry = 0;
static std::string g_token, g_api_url = "https://satella-auth-bot.onrender.com";

static std::string get_hwid() {
    HW_PROFILE_INFOA hw;
    if (GetCurrentHwProfileA(&hw))
        return hw.szHwProfileGuid;
    return "UNKNOWN";
}

static std::string http_post(const std::string& path, const std::string& data) {
    std::string host, url_path;
    int port = 80;
    bool useSsl = false;

    size_t proto_end = g_api_url.find("://");
    if (proto_end == std::string::npos) return "";
    std::string proto = g_api_url.substr(0, proto_end);
    std::string rest = g_api_url.substr(proto_end + 3);
    useSsl = (proto == "https");
    port = useSsl ? 443 : 80;

    size_t colon = rest.find(':');
    size_t slash = rest.find('/');
    if (colon != std::string::npos) {
        host = rest.substr(0, colon);
        size_t end = (slash != std::string::npos) ? slash : rest.size();
        port = std::stoi(rest.substr(colon + 1, end - colon - 1));
    } else if (slash != std::string::npos) {
        host = rest.substr(0, slash);
    } else {
        host = rest;
    }

    if (path[0] == '/')
        url_path = path;
    else
        url_path = "/" + path;

    HINTERNET hSession = WinHttpOpen(L"SatellaAuth/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return "";

    int hostLen = MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, NULL, 0);
    wchar_t* whost = new wchar_t[hostLen];
    MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, whost, hostLen);

    HINTERNET hConnect = WinHttpConnect(hSession, whost, port, 0);
    delete[] whost;
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    int pathLen = MultiByteToWideChar(CP_UTF8, 0, url_path.c_str(), -1, NULL, 0);
    wchar_t* wpath = new wchar_t[pathLen];
    MultiByteToWideChar(CP_UTF8, 0, url_path.c_str(), -1, wpath, pathLen);

    DWORD flags = 0;
    if (useSsl) flags |= WINHTTP_FLAG_SECURE;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    delete[] wpath;
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    if (useSsl) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    LPCWSTR headers = L"Content-Type: application/json\r\nUser-Agent: SatellaAuth/1.0\r\n";

    if (!WinHttpSendRequest(hRequest, headers, (DWORD)wcslen(headers),
        (LPVOID)data.c_str(), (DWORD)data.length(), (DWORD)data.length(), 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession); return "";
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession); return "";
    }

    std::string result;
    DWORD bytesRead = 0;
    char buffer[4096];
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        result += buffer;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

static std::string json_str(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        pos++;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\') { pos++; if (pos < json.size()) val += json[pos]; }
            else val += json[pos];
            pos++;
        }
        return val;
    }
    std::string val;
    while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']') {
        if (json[pos] != ' ' && json[pos] != '\t' && json[pos] != '\n') val += json[pos];
        pos++;
    }
    return val;
}

static bool json_bool(const std::string& json, const std::string& key) {
    auto val = json_str(json, key);
    return val == "true" || val == "1";
}

static long long json_int64(const std::string& json, const std::string& key) {
    auto val = json_str(json, key);
    if (val.empty()) return 0;
    return std::stoll(val);
}

int ka_init() {
    g_err.clear();
    return 1;
}

int ka_login(const char* username, const char* password) {
    std::string hwid = get_hwid();
    std::string data = "{\"username\":\"" + std::string(username) +
        "\",\"password\":\"" + std::string(password) +
        "\",\"hwid\":\"" + hwid + "\"}";

    std::string resp = http_post("/api/login", data);
    if (resp.empty()) {
        g_err = "Connection failed";
        return 0;
    }

    if (!json_bool(resp, "success")) {
        g_err = json_str(resp, "error");
        if (g_err.empty()) g_err = "Login failed";
        return 0;
    }

    g_user = username;
    g_token = json_str(resp, "token");
    g_expiry = json_int64(resp, "expires_at");
    g_err.clear();
    return 1;
}

int ka_register(const char* username, const char* password, const char* key) {
    std::string hwid = get_hwid();
    std::string data = "{\"username\":\"" + std::string(username) +
        "\",\"password\":\"" + std::string(password) +
        "\",\"key\":\"" + std::string(key) +
        "\",\"hwid\":\"" + hwid + "\"}";

    std::string resp = http_post("/api/register", data);
    if (resp.empty()) {
        g_err = "Connection failed";
        return 0;
    }

    if (!json_bool(resp, "success")) {
        g_err = json_str(resp, "error");
        if (g_err.empty()) g_err = "Registration failed";
        return 0;
    }

    g_user = username;
    g_token = json_str(resp, "token");
    g_err.clear();
    return 1;
}

int ka_verify() {
    if (g_token.empty()) return 0;
    std::string data = "{\"token\":\"" + g_token + "\"}";
    std::string resp = http_post("/api/verify", data);
    if (resp.empty() || !json_bool(resp, "success")) {
        g_err = json_str(resp, "error");
        if (g_err.empty()) g_err = "Session expired";
        return 0;
    }
    g_expiry = json_int64(resp, "expires_at");
    return 1;
}

const char* ka_get_username() { return g_user.c_str(); }
const char* ka_get_error() { return g_err.empty() ? NULL : g_err.c_str(); }
int ka_get_days_remaining() {
    if (g_expiry <= 0) return 0;
    long long now = (long long)std::time(nullptr);
    long long diff = g_expiry - now;
    return diff > 0 ? (int)(diff / 86400) : 0;
}

const char* ka_get_token() { return g_token.c_str(); }
void ka_set_api_url(const char* url) { g_api_url = url; }

void ka_cleanup() {
    g_user.clear();
    g_err.clear();
    g_token.clear();
    g_expiry = 0;
}
