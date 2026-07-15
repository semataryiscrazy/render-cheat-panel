#include "web_server.h"
#include "options.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// ─── helpers json ────────────────────────────────────────────

static std::string auth_api_base = "https://satella-auth-bot.onrender.com";

static std::string http_post_json(const std::string& url, const std::string& data) {
    std::string host, path;
    int port = 443;
    bool useSsl = true;

    size_t proto = url.find("://");
    if (proto == std::string::npos) return "{\"success\":false,\"error\":\"invalid url\"}";
    std::string rest = url.substr(proto + 3);
    size_t slash = rest.find('/');
    if (slash != std::string::npos) {
        host = rest.substr(0, slash);
        path = rest.substr(slash);
    } else {
        host = rest;
        path = "/";
    }

    HINTERNET hSession = WinHttpOpen(L"CheatWebPanel/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return "{\"success\":false,\"error\":\"WinHttpOpen failed\"}";

    int hlen = MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, NULL, 0);
    wchar_t* whost = new wchar_t[hlen];
    MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, whost, hlen);

    HINTERNET hConnect = WinHttpConnect(hSession, whost, port, 0);
    delete[] whost;
    if (!hConnect) { WinHttpCloseHandle(hSession); return "{\"success\":false,\"error\":\"connect failed\"}"; }

    int plen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
    wchar_t* wpath = new wchar_t[plen];
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath, plen);

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    delete[] wpath;
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return "{\"success\":false,\"error\":\"request failed\"}"; }

    DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));

    LPCWSTR headers = L"Content-Type: application/json\r\n";

    if (!WinHttpSendRequest(hRequest, headers, (DWORD)wcslen(headers),
        (LPVOID)data.c_str(), (DWORD)data.length(), (DWORD)data.length(), 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession); return "{\"success\":false,\"error\":\"send failed\"}";
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession); return "{\"success\":false,\"error\":\"recv failed\"}";
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
static std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

static std::string bool_str(bool v) { return v ? "true" : "false"; }

// ─── serializa as opções como JSON ───────────────────────────
static void serialize_aimbot(std::stringstream& j) {
    auto& o = g_Options.LegitBot.AimBot;
    j << "{";
    j << "\"name\":\"AimBot\",\"options\":[";
    j << "{\"k\":\"Enabled\",\"t\":\"bool\",\"v\":" << bool_str(o.Enabled) << "},";
    j << "{\"k\":\"Precision\",\"t\":\"bool\",\"v\":" << bool_str(o.Precision) << "},";
    j << "{\"k\":\"ScopeTracking\",\"t\":\"bool\",\"v\":" << bool_str(o.ScopeTracking) << "},";
    j << "{\"k\":\"Pixel\",\"t\":\"bool\",\"v\":" << bool_str(o.Pixel) << "},";
    j << "{\"k\":\"IgnoreKnocked\",\"t\":\"bool\",\"v\":" << bool_str(o.IgnoreKnocked) << "},";
    j << "{\"k\":\"TargetNPC\",\"t\":\"bool\",\"v\":" << bool_str(o.TargetNPC) << "},";
    j << "{\"k\":\"PullCamAfterKill\",\"t\":\"bool\",\"v\":" << bool_str(o.PullCamAfterKill) << "},";
    j << "{\"k\":\"MaxDistance\",\"t\":\"int\",\"v\":" << o.MaxDistance << ",\"min\":0,\"max\":600},";
    j << "{\"k\":\"DelayMode\",\"t\":\"combo\",\"v\":" << o.DelayMode << ",\"opts\":[\"0 Peito\",\"1 Peito\",\"2 Peitos\",\"3 Peitos\",\"4 Peitos\",\"5 Peitos\",\"Random\"]},";
    j << "{\"k\":\"AimType\",\"t\":\"combo\",\"v\":" << o.AimType << ",\"opts\":[\"Aim Rage\",\"Aim Neck\",\"Aim Legit\"]},";
    j << "{\"k\":\"FOV\",\"t\":\"float\",\"v\":" << o.FOV << ",\"min\":0,\"max\":360},";
    j << "{\"k\":\"SmoothH\",\"t\":\"float\",\"v\":" << o.SmoothHorizontal << ",\"min\":0,\"max\":200,\"l\":\"Smooth Horizontal\"},";
    j << "{\"k\":\"SmoothV\",\"t\":\"float\",\"v\":" << o.SmoothVertical << ",\"min\":0,\"max\":200,\"l\":\"Smooth Vertical\"},";
    j << "{\"k\":\"ColliderFOV\",\"t\":\"float\",\"v\":" << o.ColliderFOV << ",\"min\":0,\"max\":360},";
    j << "{\"k\":\"ColliderVisibleOnly\",\"t\":\"bool\",\"v\":" << bool_str(o.ColliderVisibleOnly) << "},";
    j << "{\"k\":\"PullCamHeight\",\"t\":\"float\",\"v\":" << o.PullCamHeight << ",\"min\":0.1,\"max\":1.0}";
    j << "]}";
}

static void serialize_esp(std::stringstream& j) {
    auto& p = g_Options.Visuals.ESP.Players;
    j << "{";
    j << "\"name\":\"ESP Players\",\"options\":[";
    j << "{\"k\":\"Enabled\",\"t\":\"bool\",\"v\":" << bool_str(p.Enabled) << "},";
    j << "{\"k\":\"ShowLocalPlayer\",\"t\":\"bool\",\"v\":" << bool_str(p.ShowLocalPlayer) << "},";
    j << "{\"k\":\"ShowNPCs\",\"t\":\"bool\",\"v\":" << bool_str(p.ShowNPCs) << "},";
    j << "{\"k\":\"Box\",\"t\":\"bool\",\"v\":" << bool_str(p.Box) << "},";
    j << "{\"k\":\"BoxStyle\",\"t\":\"combo\",\"v\":" << p.BoxStyle << ",\"opts\":[\"Full\",\"Cornered\"]},";
    j << "{\"k\":\"Skeleton\",\"t\":\"bool\",\"v\":" << bool_str(p.Skeleton) << "},";
    j << "{\"k\":\"Name\",\"t\":\"bool\",\"v\":" << bool_str(p.Name) << "},";
    j << "{\"k\":\"HealthBar\",\"t\":\"bool\",\"v\":" << bool_str(p.HealthBar) << "},";
    j << "{\"k\":\"HealthBarPos\",\"t\":\"combo\",\"v\":" << p.HealthBarPos << ",\"opts\":[\"Left\",\"Right\",\"Top\",\"Bottom\"]},";
    j << "{\"k\":\"ArmorBar\",\"t\":\"bool\",\"v\":" << bool_str(p.ArmorBar) << "},";
    j << "{\"k\":\"WeaponName\",\"t\":\"bool\",\"v\":" << bool_str(p.WeaponName) << "},";
    j << "{\"k\":\"Distance\",\"t\":\"bool\",\"v\":" << bool_str(p.Distance) << "},";
    j << "{\"k\":\"SnapLines\",\"t\":\"bool\",\"v\":" << bool_str(p.SnapLines) << "},";
    j << "{\"k\":\"SnapLinesPos\",\"t\":\"combo\",\"v\":" << p.SnapLinesPos << ",\"opts\":[\"Top\",\"Bottom\"]},";
    j << "{\"k\":\"RenderDistance\",\"t\":\"int\",\"v\":" << p.RenderDistance << ",\"min\":0,\"max\":600}";
    j << "]}";
}

static void serialize_exploits(std::stringstream& j) {
    auto& e = g_Options.Exploits;
    j << "{";
    j << "\"name\":\"Exploits\",\"options\":[";
    j << "{\"k\":\"NoRecoil\",\"t\":\"bool\",\"v\":" << bool_str(e.NoRecoil) << "},";
    j << "{\"k\":\"NoRecoilStrength\",\"t\":\"float\",\"v\":" << e.NoRecoilStrength << ",\"min\":0,\"max\":100,\"l\":\"Recoil Reduction %\"},";
    j << "{\"k\":\"WeaponAttributes\",\"t\":\"bool\",\"v\":" << bool_str(e.WeaponAttributes) << "},";
    j << "{\"k\":\"WeaponAttributesLevel\",\"t\":\"combo\",\"v\":" << e.WeaponAttributesLevel << ",\"opts\":[\"LV 1\",\"LV 2\",\"LV 3\",\"LV 4\"]},";
    j << "{\"k\":\"SpinBot\",\"t\":\"bool\",\"v\":" << bool_str(e.SpinBot) << "},";
    j << "{\"k\":\"SpinBotSpeed\",\"t\":\"float\",\"v\":" << e.SpinBotSpeed << ",\"min\":0,\"max\":50},";
    j << "{\"k\":\"SpeedHack\",\"t\":\"bool\",\"v\":" << bool_str(e.SpeedHack) << "},";
    j << "{\"k\":\"SpeedMultiplier\",\"t\":\"float\",\"v\":" << e.SpeedMultiplier << ",\"min\":1,\"max\":10},";
    j << "{\"k\":\"FastMedkit\",\"t\":\"bool\",\"v\":" << bool_str(e.FastMedkit) << "},";
    j << "{\"k\":\"FastReload\",\"t\":\"bool\",\"v\":" << bool_str(e.FastReload) << "},";
    j << "{\"k\":\"SilentAimEnabled\",\"t\":\"bool\",\"v\":" << bool_str(e.SilentAimEnabled) << "},";
    j << "{\"k\":\"SilentAimUseFOV\",\"t\":\"bool\",\"v\":" << bool_str(e.SilentAimUseFOV) << "},";
    j << "{\"k\":\"SilentAimFOV\",\"t\":\"float\",\"v\":" << e.SilentAimFOV << ",\"min\":0,\"max\":360},";
    j << "{\"k\":\"SilentAimDistance\",\"t\":\"int\",\"v\":" << e.SilentAimDistance << ",\"min\":0,\"max\":600},";
    j << "{\"k\":\"SilentAimHitBox\",\"t\":\"combo\",\"v\":" << e.SilentAimHitBox << ",\"opts\":[\"Head\",\"Neck\",\"Chest\"]},";
    j << "{\"k\":\"SilentAimTargetBots\",\"t\":\"bool\",\"v\":" << bool_str(e.SilentAimTargetBots) << "}";
    j << "]}";
}

static void serialize_general(std::stringstream& j) {
    auto& g = g_Options.General;
    j << "{";
    j << "\"name\":\"General\",\"options\":[";
    j << "{\"k\":\"CaptureBypass\",\"t\":\"bool\",\"v\":" << bool_str(g.CaptureBypass) << "},";
    j << "{\"k\":\"StreamMode\",\"t\":\"bool\",\"v\":" << bool_str(g.StreamMode) << "},";
    j << "{\"k\":\"BypassEnabled\",\"t\":\"bool\",\"v\":" << bool_str(g.BypassEnabled) << "},";
    j << "{\"k\":\"MenuToggleKey\",\"t\":\"int\",\"v\":" << g.MenuToggleKey << ",\"min\":0,\"max\":255}";
    j << "]}";
}

static std::string build_options_json() {
    std::stringstream j;
    j << "{\"sections\":[";
    j << "\"LegitBot\",";
    serialize_aimbot(j);
    j << ",";
    j << "\"Visuals\",";
    serialize_esp(j);
    j << ",";
    j << "\"Exploits\",";
    serialize_exploits(j);
    j << ",";
    j << "\"General\",";
    serialize_general(j);
    j << "]}";
    return j.str();
}

// ─── aplica uma mudança via path ────────────────────────────
static bool parse_path(const std::string& path, const std::string& value) {
    // value vem como string, converte conforme tipo
    auto& o = g_Options;

    auto set_bool = [&](bool& target) {
        target = (value == "true" || value == "1");
    };
    auto set_int = [&](int& target) {
        target = std::stoi(value);
    };
    auto set_float = [&](float& target) {
        target = std::stof(value);
    };

    // LegitBot
    if (path == "LegitBot.AimBot.Enabled") { set_bool(o.LegitBot.AimBot.Enabled); return true; }
    if (path == "LegitBot.AimBot.Precision") { set_bool(o.LegitBot.AimBot.Precision); return true; }
    if (path == "LegitBot.AimBot.ScopeTracking") { set_bool(o.LegitBot.AimBot.ScopeTracking); return true; }
    if (path == "LegitBot.AimBot.Pixel") { set_bool(o.LegitBot.AimBot.Pixel); return true; }
    if (path == "LegitBot.AimBot.IgnoreKnocked") { set_bool(o.LegitBot.AimBot.IgnoreKnocked); return true; }
    if (path == "LegitBot.AimBot.TargetNPC") { set_bool(o.LegitBot.AimBot.TargetNPC); return true; }
    if (path == "LegitBot.AimBot.PullCamAfterKill") { set_bool(o.LegitBot.AimBot.PullCamAfterKill); return true; }
    if (path == "LegitBot.AimBot.MaxDistance") { set_int(o.LegitBot.AimBot.MaxDistance); return true; }
    if (path == "LegitBot.AimBot.DelayMode") { set_int(o.LegitBot.AimBot.DelayMode); return true; }
    if (path == "LegitBot.AimBot.AimType") { set_int(o.LegitBot.AimBot.AimType); return true; }
    if (path == "LegitBot.AimBot.FOV") { set_float(o.LegitBot.AimBot.FOV); return true; }
    if (path == "LegitBot.AimBot.SmoothH") { set_float(o.LegitBot.AimBot.SmoothHorizontal); return true; }
    if (path == "LegitBot.AimBot.SmoothV") { set_float(o.LegitBot.AimBot.SmoothVertical); return true; }
    if (path == "LegitBot.AimBot.ColliderFOV") { set_float(o.LegitBot.AimBot.ColliderFOV); return true; }
    if (path == "LegitBot.AimBot.ColliderVisibleOnly") { set_bool(o.LegitBot.AimBot.ColliderVisibleOnly); return true; }
    if (path == "LegitBot.AimBot.PullCamHeight") { set_float(o.LegitBot.AimBot.PullCamHeight); return true; }

    // Visuals/ESP
    if (path == "Visuals.ESP.Players.Enabled") { set_bool(o.Visuals.ESP.Players.Enabled); return true; }
    if (path == "Visuals.ESP.Players.ShowLocalPlayer") { set_bool(o.Visuals.ESP.Players.ShowLocalPlayer); return true; }
    if (path == "Visuals.ESP.Players.ShowNPCs") { set_bool(o.Visuals.ESP.Players.ShowNPCs); return true; }
    if (path == "Visuals.ESP.Players.Box") { set_bool(o.Visuals.ESP.Players.Box); return true; }
    if (path == "Visuals.ESP.Players.BoxStyle") { set_int(o.Visuals.ESP.Players.BoxStyle); return true; }
    if (path == "Visuals.ESP.Players.Skeleton") { set_bool(o.Visuals.ESP.Players.Skeleton); return true; }
    if (path == "Visuals.ESP.Players.Name") { set_bool(o.Visuals.ESP.Players.Name); return true; }
    if (path == "Visuals.ESP.Players.HealthBar") { set_bool(o.Visuals.ESP.Players.HealthBar); return true; }
    if (path == "Visuals.ESP.Players.HealthBarPos") { set_int(o.Visuals.ESP.Players.HealthBarPos); return true; }
    if (path == "Visuals.ESP.Players.ArmorBar") { set_bool(o.Visuals.ESP.Players.ArmorBar); return true; }
    if (path == "Visuals.ESP.Players.WeaponName") { set_bool(o.Visuals.ESP.Players.WeaponName); return true; }
    if (path == "Visuals.ESP.Players.Distance") { set_bool(o.Visuals.ESP.Players.Distance); return true; }
    if (path == "Visuals.ESP.Players.SnapLines") { set_bool(o.Visuals.ESP.Players.SnapLines); return true; }
    if (path == "Visuals.ESP.Players.SnapLinesPos") { set_int(o.Visuals.ESP.Players.SnapLinesPos); return true; }
    if (path == "Visuals.ESP.Players.RenderDistance") { set_int(o.Visuals.ESP.Players.RenderDistance); return true; }

    // Exploits
    if (path == "Exploits.NoRecoil") { set_bool(o.Exploits.NoRecoil); return true; }
    if (path == "Exploits.NoRecoilStrength") { set_float(o.Exploits.NoRecoilStrength); return true; }
    if (path == "Exploits.WeaponAttributes") { set_bool(o.Exploits.WeaponAttributes); return true; }
    if (path == "Exploits.WeaponAttributesLevel") { set_int(o.Exploits.WeaponAttributesLevel); return true; }
    if (path == "Exploits.SpinBot") { set_bool(o.Exploits.SpinBot); return true; }
    if (path == "Exploits.SpinBotSpeed") { set_float(o.Exploits.SpinBotSpeed); return true; }
    if (path == "Exploits.SpeedHack") { set_bool(o.Exploits.SpeedHack); return true; }
    if (path == "Exploits.SpeedMultiplier") { set_float(o.Exploits.SpeedMultiplier); return true; }
    if (path == "Exploits.FastMedkit") { set_bool(o.Exploits.FastMedkit); return true; }
    if (path == "Exploits.FastReload") { set_bool(o.Exploits.FastReload); return true; }
    if (path == "Exploits.SilentAimEnabled") { set_bool(o.Exploits.SilentAimEnabled); return true; }
    if (path == "Exploits.SilentAimUseFOV") { set_bool(o.Exploits.SilentAimUseFOV); return true; }
    if (path == "Exploits.SilentAimFOV") { set_float(o.Exploits.SilentAimFOV); return true; }
    if (path == "Exploits.SilentAimDistance") { set_int(o.Exploits.SilentAimDistance); return true; }
    if (path == "Exploits.SilentAimHitBox") { set_int(o.Exploits.SilentAimHitBox); return true; }
    if (path == "Exploits.SilentAimTargetBots") { set_bool(o.Exploits.SilentAimTargetBots); return true; }

    // General
    if (path == "General.CaptureBypass") { set_bool(o.General.CaptureBypass); return true; }
    if (path == "General.StreamMode") { set_bool(o.General.StreamMode); return true; }
    if (path == "General.BypassEnabled") { set_bool(o.General.BypassEnabled); return true; }
    if (path == "General.MenuToggleKey") { set_int(o.General.MenuToggleKey); return true; }

    return false;
}

// ─── frontend HTML embutido (split em 2 partes p/ evitar C2026) ──
static const char* FRONTEND_HTML_P1 = R"rawliteral1(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Lunar Network</title>
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css">
<style>
*{margin:0;padding:0;box-sizing:border-box;user-select:none}
body{font-family:Inter,'Segoe UI',system-ui,sans-serif;background:#000;overflow:hidden;height:100vh;display:flex;align-items:center;justify-content:center}
#app{position:relative;width:640px;height:440px;border-radius:6px;overflow:hidden}
#app.login{width:659px;height:431px}
.login-sidebar{position:absolute;left:0;top:0;width:101px;height:100%;border-radius:6px 0 0 6px;z-index:2;background:rgba(0,0,0,0.85)}
.login-content{position:absolute;left:101px;top:0;width:558px;height:100%}
.login-letter{position:absolute;left:0;width:101px;text-align:center;font-size:15px;color:rgba(255,255,255,0.8);letter-spacing:0}
.login-letter span{display:block;text-align:center}
.lunar-letters{top:59px}
.network-letters{top:259px}
.login-letter span{line-height:22px}
.login-logo-img{position:absolute;left:50%;transform:translateX(-50%);top:148px;width:140px;height:140px;background:linear-gradient(135deg,#2a1a5e55,#1a1a3e55);border-radius:12px}
.login-welcome{position:absolute;left:0;top:50px;width:558px;text-align:center;font-size:16px;color:rgb(174,174,174)}
.login-subtitle{position:absolute;left:0;top:78px;width:558px;text-align:center;font-size:12px;color:rgb(69,69,69)}
.login-input-wrap{position:absolute;left:101px;top:197px;width:358px;height:37px;left:50%;transform:translateX(-50%)}
.login-input-wrap input{width:358px;height:37px;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.08);border-radius:6px;padding:0 12px;font-size:14px;color:rgba(255,255,255,0.8);outline:none}
.login-input-wrap input:focus{border-color:rgba(123,47,252,0.27)}
.login-input-wrap input::placeholder{color:rgba(255,255,255,0.2)}
.login-btn-wrap{position:absolute;left:50%;transform:translateX(-50%);top:249px}
.login-btn-wrap button{width:358px;height:34px;border:none;border-radius:6px;cursor:pointer;font-size:14px;color:rgba(255,255,255,0.8);transition:.15s}
.login-btn-wrap button{background:rgba(255,255,255,0.06)}
.login-btn-wrap button:hover{background:rgba(255,255,255,0.1)}
.login-btn-wrap button:active{background:rgba(255,255,255,0.03)}

.loading-overlay{position:absolute;inset:0;background:#000;z-index:20;display:flex;align-items:center;justify-content:center;flex-direction:column;transition:opacity .3s}
.loading-overlay.hidden{display:none}
.loading-spinner{width:60px;height:60px;position:relative}
.loading-spinner::before{content:'';position:absolute;inset:0;border-radius:50%;border:2px solid rgba(255,255,255,0.1);border-top-color:rgba(255,255,255,0.8);animation:spin .8s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
.loading-text{position:absolute;bottom:calc(50% - 55px);font-size:14px;color:rgba(255,255,255,0.6)}

/* header */
.main-header{position:absolute;top:0;left:0;width:100%;height:60px;border-bottom:1px solid rgb(23,24,25);display:none;z-index:10}
.header-logo{position:absolute;left:-8px;top:-13px;width:86px;height:86px;background:linear-gradient(135deg,#2a1a5e55,#1a1a3e55);border-radius:8px}
.header-title{position:absolute;left:50%;top:22px;transform:translateX(-50%);font-size:14px;color:rgba(255,255,255,0.8)}
.header-avatar{position:absolute;right:39.5px;top:10px;width:40px;height:40px;border-radius:50%;border:1.5px solid rgb(60,60,60);background:rgb(21,21,21);display:flex;align-items:center;justify-content:center;overflow:hidden}
.header-avatar i{font-size:20px;color:rgb(85,85,85)}
.header-name{position:absolute;right:86px;top:16px;font-size:14px;color:rgba(255,255,255,0.8);text-align:right}
.header-role{position:absolute;right:86px;top:31px;font-size:11px;color:rgb(128,128,128);text-align:right}

/* content */
.main-content{position:absolute;top:60px;left:0;width:100%;height:calc(100% - 60px - 39px);display:none;padding:19px}
.content-child{width:100%;height:100%;border:1px solid rgb(34,34,34);border-radius:4px;overflow-y:auto;padding:15px 20px;position:relative;background:rgba(0,0,0,0.85)}
.content-child::-webkit-scrollbar{width:6px}
.content-child::-webkit-scrollbar-track{background:transparent}
.content-child::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.12);border-radius:9px}
.content-child::-webkit-scrollbar-thumb:hover{background:rgba(255,70,175,0.47)}

.section-title{font-size:14px;color:rgb(128,128,128);margin-bottom:16px;padding-bottom:8px;border-bottom:1px solid rgb(23,24,25)}
.subsection-title{font-size:14px;color:rgb(128,128,128);margin:12px 0 8px;padding-bottom:8px;border-bottom:1px solid rgb(23,24,25)}
.opt-row{display:flex;align-items:center;justify-content:space-between;padding:6px 0}
.opt-row-cols{display:grid;grid-template-columns:1fr 1fr;gap:4px}
.opt-label{font-size:14px;color:rgba(255,255,255,0.8)}
.opt-control{display:flex;align-items:center;gap:8px}
.checkbox-wrap{display:flex;align-items:center;gap:8px;cursor:pointer}
.checkbox-wrap input[type=checkbox]{display:none}
.checkbox-box{width:16px;height:16px;border:1px solid rgba(255,255,255,0.2);border-radius:3px;display:flex;align-items:center;justify-content:center;transition:.15s;flex-shrink:0}
.checkbox-wrap input:checked+.checkbox-box{border-color:rgba(255,255,255,0.4)}
.checkbox-wrap input:checked+.checkbox-box::after{content:'\2713';color:rgba(255,255,255,0.8);font-size:11px}
.inp-num{width:80px;padding:4px 8px;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.08);border-radius:6px;color:rgba(255,255,255,0.8);font-size:13px;outline:none}
.inp-num:focus{border-color:rgba(123,47,252,0.27)}
select{background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.08);color:rgba(255,255,255,0.8);padding:4px 8px;border-radius:6px;font-size:13px;outline:none;min-width:120px}
select:focus{border-color:rgba(123,47,252,0.27)}
input[type=range]{width:120px;background:transparent;accent-color:rgb(123,47,252);vertical-align:middle}
.range-val{font-size:12px;color:rgb(136,136,136);min-width:35px;text-align:right}
.btn-unload{padding:4px 10px;border:none;border-radius:4px;cursor:pointer;font-size:12px;background:rgba(90,28,28,0.86);color:rgba(255,255,255,0.8)}
.btn-unload:hover{background:rgba(140,38,38,0.86)}

/* bottom nav */
.bottom-nav{position:absolute;bottom:0;left:0;width:100%;height:39px;display:none;justify-content:center;align-items:center;z-index:10}
.nav-bg{position:absolute;width:172px;height:39px;border:1px solid rgb(34,34,34);border-radius:4px}
.nav-selector{position:absolute;width:32px;height:32px;top:3.5px;border:1px solid rgb(32,32,32);border-radius:3px;transition:left .2s cubic-bezier(.4,.0,.2,1);background:#000}
.nav-items{position:absolute;display:flex;width:172px;height:39px;align-items:center;justify-content:center;gap:5.5px}
.nav-item{width:32px;height:32px;display:flex;align-items:center;justify-content:center;cursor:pointer;font-size:17px;color:rgb(100,100,100);transition:color .15s;border-radius:3px}
.nav-item.active{color:#fff}

.sig{position:absolute;bottom:45px;right:19px;font-size:13px;color:rgb(180,180,180);z-index:5;display:none}

/* notifications */
.notif-container{position:fixed;bottom:20px;right:20px;z-index:999;display:flex;flex-direction:column-reverse;gap:10px;pointer-events:none}
.notif{width:280px;min-height:38px;background:rgba(10,10,10,0.96);border:1px solid rgba(30,30,30,0.96);border-radius:8px;padding:0;display:flex;align-items:center;gap:0;opacity:0;transform:translateX(40px);animation:notifIn .3s forwards;pointer-events:auto}
.notif.out{animation:notifOut .3s forwards}
@keyframes notifIn{to{opacity:1;transform:translateX(0)}}
@keyframes notifOut{to{opacity:0;transform:translateX(40px)}}
.notif-icon{width:20px;height:20px;border-radius:50%;display:flex;align-items:center;justify-content:center;flex-shrink:0;margin:9px 0 9px 12px;font-size:11px;color:#fff}
.notif-text{font-size:13px;color:rgba(255,255,255,0.9);margin:0 12px;flex:1}
.notif-bar{position:absolute;bottom:2px;left:10px;right:10px;height:2px;border-radius:1px;background:rgba(255,255,255,0.2);overflow:hidden}
.notif-bar-inner{height:100%;background:rgba(255,255,255,0.8);transition:width linear}
</style>
</head>
)rawliteral1";

static const char* FRONTEND_HTML_P2 = R"rawliteral2(
<body>
<div id="app" class="login">
<div class="login-sidebar"></div>
<div class="login-content">
<div class="login-letter lunar-letters">
<span>L</span><span>U</span><span>N</span><span>A</span><span>R</span>
</div>
<div class="login-logo-img"></div>
<div class="login-letter network-letters">
<span>N</span><span>E</span><span>T</span><span>W</span><span>O</span><span>R</span><span>K</span>
</div>
            <div class="login-welcome">Welcome to Lunar Network</div>
            <div class="login-subtitle">The best undetected external cheat for your gameplay.</div>
            <div class="login-input-wrap" style="top:185px"><input id="keyInput" placeholder="Enter your license key" maxlength="255"></div>
            <div class="login-btn-wrap" style="top:240px"><button id="loginBtn" onclick="doLogin()">Login</button></div>
            <div id="loginError" style="position:absolute;left:101px;top:170px;width:558px;text-align:center;font-size:12px;color:#ff4444;display:none"></div>
            <div id="loadingText" style="position:absolute;top:295px;left:101px;width:558px;text-align:center;font-size:12px;color:rgb(69,69,69);display:none">Validating key...</div>
</div>
</div>

<div class="loading-overlay hidden" id="loadingOverlay">
<div class="loading-spinner"></div>
<div class="loading-text" id="loadingText">Connecting to Lunar</div>
</div>

<div class="main-header" id="mainHeader">
<div class="header-logo"></div>
<div class="header-title" id="headerTitle">Aimbot</div>
<div class="header-avatar"><i class="fas fa-user"></i></div>
<div class="header-name" id="headerName">Lunar</div>
<div class="header-role" id="headerRole">Cliente</div>
</div>

<div class="main-content" id="mainContent">
<div class="content-child" id="tabContent"></div>
</div>

<div class="bottom-nav" id="bottomNav">
<div class="nav-bg"></div>
<div class="nav-selector" id="navSelector" style="left:5.5px"></div>
<div class="nav-items">
<div class="nav-item active" data-tab="0" onclick="switchTab(0)"><i class="fas fa-crosshairs"></i></div>
<div class="nav-item" data-tab="1" onclick="switchTab(1)"><i class="fas fa-user"></i></div>
<div class="nav-item" data-tab="2" onclick="switchTab(2)"><i class="fas fa-bomb"></i></div>
<div class="nav-item" data-tab="3" onclick="switchTab(3)"><i class="fas fa-gear"></i></div>
</div>
</div>

<div class="sig" id="sig">by: vypeexsw</div>
<div class="notif-container" id="notifContainer"></div>

<script>
const BASE=''; let opts=null; let curTab=0; let loggedIn=false; let notifId=0;
let authToken='';

function addNotif(text,color,icon){
 const c=document.getElementById('notifContainer');
 const id='n'+(notifId++);
 const div=document.createElement('div');div.className='notif';div.id=id;
 div.innerHTML=`<div class="notif-icon" style="background:${color}">${icon}</div><div class="notif-text">${text}</div><div class="notif-bar"><div class="notif-bar-inner" style="width:100%"></div></div>`;
 c.appendChild(div);
 const bar=div.querySelector('.notif-bar-inner');let t=4;const dur=4;
 const iv=setInterval(()=>{t-=0.05;bar.style.width=(t/dur*100)+'%';if(t<=0){clearInterval(iv);div.classList.add('out');setTimeout(()=>div.remove(),300)}},50);
 setTimeout(()=>{div.classList.add('out');setTimeout(()=>div.remove(),300)},4000);
}

async function loadOpts(){
 try{const r=await fetch(BASE+'/api/options');const d=await r.json();opts=d;if(loggedIn)renderTab(curTab)}catch(e){}
}

async function doLogin(){
 const key=document.getElementById('keyInput').value.trim();
 if(!key)return;
 const err=document.getElementById('loginError');
 const lt=document.getElementById('loadingText');
 err.style.display='none';
 lt.style.display='block';
 const btn=document.getElementById('loginBtn');
 btn.disabled=true;btn.textContent='...';
 try{
  const r=await fetch(BASE+'/api/auth/key',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({key:key})});
  const d=await r.json();
  if(d.success){
   authToken=d.token;
   addNotif('Key validated!','rgb(0,214,115)','\u2713');
   enterMain();
  }else{
   err.textContent=d.error||'Invalid key';
   err.style.display='block';
  }
 }catch(e){
  err.textContent='Connection error';
  err.style.display='block';
 }
 lt.style.display='none';
 btn.disabled=false;btn.textContent='Login';
}

async function verifySession(){
 if(!authToken)return false;
 try{
  const r=await fetch(BASE+'/api/auth/verify',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({token:authToken})});
  const d=await r.json();
  return d.success;
 }catch(e){return false}
}

function enterMain(){
 loggedIn=true;
 document.getElementById('mainHeader').style.display='block';
 document.getElementById('mainContent').style.display='block';
 document.getElementById('bottomNav').style.display='flex';
 document.getElementById('sig').style.display='block';
 document.getElementById('app').classList.remove('login');
 document.querySelector('.login-sidebar').style.display='none';
 document.querySelector('.login-content').style.display='none';
 loadOpts();renderTab(0);
 // verifica sessão a cada 30s
 setInterval(async()=>{
  if(loggedIn&&authToken){
   const ok=await verifySession();
   if(!ok){
    addNotif('Session expired. Please login again.','rgb(255,60,60)','\u2715');
    loggedIn=false;authToken='';
    document.getElementById('mainHeader').style.display='none';
    document.getElementById('mainContent').style.display='none';
    document.getElementById('bottomNav').style.display='none';
    document.getElementById('sig').style.display='none';
    document.getElementById('app').classList.add('login');
    document.querySelector('.login-sidebar').style.display='block';
    document.querySelector('.login-content').style.display='block';
    document.getElementById('keyInput').value='';
   }
  }
 },30000);
}

function switchTab(idx){
 curTab=idx;
 document.querySelectorAll('.nav-item').forEach((el,i)=>el.classList.toggle('active',i===idx));
 const pos=[5.5,48.5,91.5,134.5];
 document.getElementById('navSelector').style.left=pos[idx]+'px';
 document.getElementById('headerTitle').textContent=['Aimbot','ESP','Exploits','Configs'][idx];
 renderTab(idx);
}

function chk(label,path,checked){
 return `<div class="opt-row"><span class="opt-label">${label}</span><div class="opt-control"><label class="checkbox-wrap"><input type="checkbox" ${checked?'checked':''} onchange="update('${path}',this.checked)"><span class="checkbox-box"></span></label></div></div>`;
}

function ctrl(opt,path){
 const p=path||'';
 if(opt.t==='bool') return `<label class="checkbox-wrap"><input type="checkbox" ${opt.v?'checked':''} onchange="update('${p}',this.checked)"><span class="checkbox-box"></span></label>`;
 if(opt.t==='combo') return `<select onchange="update('${p}',this.value)">${opt.opts.map((x,i)=>`<option value="${i}" ${i===opt.v?'selected':''}>${x}</option>`).join('')}</select>`;
 if(opt.t==='int') return `<input class="inp-num" type="number" value="${opt.v}" min="${opt.min}" max="${opt.max}" onchange="update('${p}',this.value)">`;
 if(opt.t==='float'){const st=(opt.max-opt.min)>100?1:0.1;return `<div style="display:flex;align-items:center;gap:8px"><input type="range" value="${opt.v}" min="${opt.min}" max="${opt.max}" step="${st}" oninput="update('${p}',this.value);this.nextElementSibling.textContent=parseFloat(this.value).toFixed(1)"><span class="range-val">${parseFloat(opt.v).toFixed(1)}</span></div>`;}
 return '';
}

function row(label,ctrlHtml){
 return `<div class="opt-row"><span class="opt-label">${label}</span><div class="opt-control">${ctrlHtml}</div></div>`;
}

function renderTab(idx){
 const con=document.getElementById('tabContent');
 if(!opts){con.innerHTML='<div style="color:rgb(128,128,128);text-align:center;padding:40px">carregando...</div>';return}
 let html='';
 if(idx===0){
  const a=opts.sections[0].options;
  html=`<div class="section-title">Aimbot Control</div>
  ${chk('Enable Aimbot','LegitBot.AimBot.Enabled',a[0].v)}
  ${chk('Precision','LegitBot.AimBot.Precision',a[1].v)}
  ${chk('Pixel Estendido','LegitBot.AimBot.Pixel',a[3].v)}
  ${row('Distance',ctrl(a[7],'LegitBot.AimBot.MaxDistance'))}`;
  if(a[0].v){
   html+=`<div class="subsection-title">Aimbot Type</div>
   ${row('Type',ctrl(a[9],'LegitBot.AimBot.AimType'))}
   ${chk('Ignore Knocked','LegitBot.AimBot.IgnoreKnocked',a[4].v)}
   <div class="subsection-title">Delay Mode</div>
   ${row('Delay',ctrl(a[8],'LegitBot.AimBot.DelayMode'))}
   ${chk('Pull Camera After Kill','LegitBot.AimBot.PullCamAfterKill',a[6].v)}`;
   if(a[6].v) html+=`${row('Pull Cam Height',ctrl(a[15],'LegitBot.AimBot.PullCamHeight'))}`;
  }
 }
 else if(idx===1){
  const p=opts.sections[2].options;
  html=`<div class="section-title">ESP Settings</div>
  <div class="opt-row-cols">
  ${chk('Enable Players ESP','Visuals.ESP.Players.Enabled',p[0].v)}
  ${chk('Box ESP','Visuals.ESP.Players.Box',p[3].v)}
  ${chk('Snap Lines','Visuals.ESP.Players.SnapLines',p[11].v)}
  ${chk('Health Bar','Visuals.ESP.Players.HealthBar',p[7].v)}
  ${chk('Skeleton ESP','Visuals.ESP.Players.Skeleton',p[4].v)}
  ${chk('Name ESP','Visuals.ESP.Players.Name',p[5].v)}
  ${chk('Distance ESP','Visuals.ESP.Players.Distance',p[10].v)}
  ${chk('Weapon ESP','Visuals.ESP.Players.WeaponName',p[9].v)}
  </div>
  ${row('Render Distance',ctrl(p[14],'Visuals.ESP.Players.RenderDistance'))}`;
 }
 else if(idx===2){
  const e=opts.sections[4].options;
  html=`<div class="section-title">Exploits</div>
  <div class="opt-row-cols">
  ${chk('No Recoil','Exploits.NoRecoil',e[0].v)}
  ${chk('Weapon Attributes','Exploits.WeaponAttributes',e[2].v)}
  ${chk('Spin Bot','Exploits.SpinBot',e[4].v)}
  ${chk('Fast Medkit','Exploits.FastMedkit',e[8].v)}
  ${chk('Fast Reload','Exploits.FastReload',e[9].v)}
  ${chk('Silent Aim','Exploits.SilentAimEnabled',e[10].v)}
  </div>`;
  if(e[0].v) html+=`${row('Recoil Reduction %',ctrl(e[1],'Exploits.NoRecoilStrength'))}`;
  if(e[2].v) html+=`${row('Weapon Level',ctrl(e[3],'Exploits.WeaponAttributesLevel'))}`;
  if(e[4].v) html+=`${row('Spin Speed',ctrl(e[5],'Exploits.SpinBotSpeed'))}`;
  if(e[10].v){
   html+=`${row('Silent HitBox',ctrl(e[15],'Exploits.SilentAimHitBox'))}
   ${chk('Use FOV','Exploits.SilentAimUseFOV',e[11].v)}`;
   if(e[11].v) html+=`${row('Silent FOV',ctrl(e[12],'Exploits.SilentAimFOV'))}`;
   html+=`${row('Silent Distance',ctrl(e[13],'Exploits.SilentAimDistance'))}
   ${chk('Target Bots','Exploits.SilentAimTargetBots',e[14].v)}`;
  }
 }
 else if(idx===3){
  const g=opts.sections[5].options;
  html=`<div class="section-title">Configs &amp; Settings</div>
  ${chk('Capture Bypass','General.CaptureBypass',g[0].v)}
  ${chk('Stream Mode','General.StreamMode',g[1].v)}
  <div class="subsection-title">Bypass</div>
  <div class="opt-row"><span class="opt-label">Stream Proof Bypass</span><div class="opt-control"><button id="bypassBtn" onclick="triggerBypass()" style="padding:4px 12px;border:none;border-radius:4px;cursor:pointer;font-size:12px;background:rgba(123,47,252,0.7);color:rgba(255,255,255,0.9);transition:.15s">Activate Bypass</button></div></div>
  <div class="subsection-title">Menu Key</div>
  <div class="opt-row"><span class="opt-label">Insert</span><div class="opt-control" style="color:rgba(255,255,255,0.4);font-size:13px">VK_INSERT</div></div>
  <div style="margin-top:14px"><button class="btn-unload" onclick="triggerUnload()">Unload DLL</button></div>`;
 }
 con.innerHTML=html;
}

async function triggerBypass(){
 const btn=document.getElementById('bypassBtn');
 btn.disabled=true;btn.textContent='Activating...';
 try{
  const r=await fetch(BASE+'/api/bypass',{method:'POST'});
  const d=await r.json();
  if(d.success){
   addNotif('Bypass activated!','rgb(0,214,115)','\u2713');
   btn.textContent='Bypass Active';
   btn.style.background='rgba(0,214,115,0.6)';
   // marca o checkbox
  }else{
   addNotif('Bypass failed: '+d.error,'rgb(255,60,60)','\u2715');
   btn.disabled=false;btn.textContent='Activate Bypass';
  }
 }catch(e){
  addNotif('Bypass error','rgb(255,60,60)','\u2715');
  btn.disabled=false;btn.textContent='Activate Bypass';
 }
}

async function triggerUnload(){
 const btn=event.target;
 btn.disabled=true;btn.textContent='Unloading...';
 try{
  const r=await fetch(BASE+'/api/unload',{method:'POST'});
  const d=await r.json();
  if(d.success){
   addNotif('DLL unloaded!','rgb(255,60,60)','\u2715');
   btn.textContent='Unloaded';
  }else{
   addNotif('Unload failed: '+d.error,'rgb(255,60,60)','\u2715');
   btn.disabled=false;btn.textContent='Unload DLL';
  }
 }catch(e){
  addNotif('Unload error','rgb(255,60,60)','\u2715');
  btn.disabled=false;btn.textContent='Unload DLL';
 }
}

async function update(p,v){
 try{await fetch(BASE+'/api/option',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({path:p,value:v||''})})}catch(e){}
}

setInterval(loadOpts,3000);loadOpts();
</script>
</body>
</html>
)rawliteral2";

// ─── implementação do servidor ───────────────────────────────
WebServer::WebServer(int port) : port(port), running(false) {
    server_socket = INVALID_SOCKET;
}

WebServer::~WebServer() {
    stop();
}

bool WebServer::start() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;

    // fallback de portas
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
        port = ports[i];
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

void WebServer::stop() {
    running = false;
    // join na thread primeiro pra garantir que ela terminou de usar os recursos
    if (server_thread.joinable()) {
        server_thread.join();
    }
    if (server_socket != INVALID_SOCKET) {
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
    }
    WSACleanup();
}

void WebServer::server_loop(WebServer* server) {
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

        char buffer[8192] = {0};
        int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (bytes > 0) {
            std::string request(buffer);
            std::string method, path;
            std::string body;

            size_t first_space = request.find(' ');
            size_t second_space = request.find(' ', first_space + 1);
            if (first_space != std::string::npos && second_space != std::string::npos) {
                method = request.substr(0, first_space);
                path = request.substr(first_space + 1, second_space - first_space - 1);
            }

            // extrai body se for POST
            size_t body_start = request.find("\r\n\r\n");
            if (body_start != std::string::npos) {
                body = request.substr(body_start + 4);
            }

            std::string response_body = server->handle_request(method, path, body);

            std::string content_type = "text/html; charset=utf-8";
            if (path == "/api/options" || path.find("/api/") == 0) {
                content_type = "application/json";
            }

            std::stringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Access-Control-Allow-Origin: *\r\n";
            response << "Content-Type: " << content_type << "\r\n";
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

std::string WebServer::handle_request(const std::string& method, const std::string& path, const std::string& body) {
    if (path == "/" || path == "/index.html") {
        return serve_frontend();
    }
    else if (path == "/api/options") {
        return serve_options_json();
    }
    else if (path == "/api/option" && method == "POST") {
        return serve_update_option(body);
    }
    else if (path == "/api/auth/login" && method == "POST") {
        return http_post_json(auth_api_base + "/api/login", body);
    }
    else if (path == "/api/auth/register" && method == "POST") {
        return http_post_json(auth_api_base + "/api/register", body);
    }
    else if (path == "/api/auth/verify" && method == "POST") {
        return http_post_json(auth_api_base + "/api/verify", body);
    }
    else if (path == "/api/bypass" && method == "POST") {
        // ativa bypass - anti-scan, anti-debug, anti-dump, esconde threads
        g_Options.General.BypassEnabled = true;
        // o bypass real é ativado nas threads de proteção (DllEntry)
        return "{\"success\":true,\"message\":\"Bypass activated\"}";
    }
    else if (path == "/api/unload" && method == "POST") {
        // unload total da DLL
        std::thread([]() {
            Sleep(300);
            HMODULE hMod = GetModuleHandleA("Client.dll");
            if (hMod) {
                FreeLibraryAndExitThread(hMod, 0);
            }
        }).detach();
        return "{\"success\":true,\"message\":\"Unloading DLL...\"}";
    }
    else if (path == "/api/auth/key" && method == "POST") {
        // valida key: tenta registrar, se falhar tenta login
        auto find_json_str_local = [](const std::string& json, const std::string& key) -> std::string {
            std::string search = "\"" + key + "\":\"";
            size_t pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos += search.length();
            size_t end = json.find("\"", pos);
            if (end == std::string::npos) return "";
            return json.substr(pos, end - pos);
        };
        std::string key = find_json_str_local(body, "key");
        if (key.empty()) return "{\"success\":false,\"error\":\"Key required\"}";

        // 1. tenta registrar (username=key, password=key, license=key)
        std::string regData = "{\"username\":\"" + key + "\",\"password\":\"" + key + "\",\"key\":\"" + key + "\",\"hwid\":\"\"}";
        std::string regResp = http_post_json(auth_api_base + "/api/register", regData);

        // verifica se registrou
        if (regResp.find("\"success\":true") != std::string::npos) {
            // registro OK, extrai token
            auto tk = find_json_str_local(regResp, "token");
            return "{\"success\":true,\"token\":\"" + tk + "\"}";
        }

        // 2. falhou (key já usada ou inválida) — tenta login
        std::string loginData = "{\"username\":\"" + key + "\",\"password\":\"" + key + "\",\"hwid\":\"\"}";
        std::string loginResp = http_post_json(auth_api_base + "/api/login", loginData);

        if (loginResp.find("\"success\":true") != std::string::npos) {
            auto tk = find_json_str_local(loginResp, "token");
            return "{\"success\":true,\"token\":\"" + tk + "\"}";
        }

        // 3. tudo falhou
        return "{\"success\":false,\"error\":\"Invalid key\"}";
    }

    return "{\"error\":\"not found\"}";
}

std::string WebServer::serve_frontend() {
    return std::string(FRONTEND_HTML_P1) + FRONTEND_HTML_P2;
}

std::string WebServer::serve_options_json() {
    return build_options_json();
}

std::string WebServer::serve_update_option(const std::string& body) {
    // body: {"path":"LegitBot.AimBot.Enabled","value":"true"}
    // parse manual
    auto find_json_str = [](const std::string& json, const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) {
            // tenta sem aspas no value
            search = "\"" + key + "\":";
            pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos += search.length();
            size_t end = json.find_first_of(",}", pos);
            if (end == std::string::npos) return "";
            std::string val = json.substr(pos, end - pos);
            // remove aspas se tiver
            if (val.front() == '"') val = val.substr(1);
            if (val.back() == '"') val.pop_back();
            return val;
        }
        pos += search.length();
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    };

    std::string path = find_json_str(body, "path");
    std::string value = find_json_str(body, "value");

    if (path.empty()) {
        return "{\"success\":false,\"error\":\"path vazio\"}";
    }

    bool ok = parse_path(path, value);
    return ok ? "{\"success\":true}" : "{\"success\":false,\"error\":\"path invalido\"}";
}
