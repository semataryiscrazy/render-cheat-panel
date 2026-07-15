#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

struct CheatConfig {
    bool aimbot = false;
    bool esp = false;
    bool wallhack = false;
    bool triggerbot = false;
    bool noRecoil = false;
    bool speedhack = false;
    bool godMode = false;
    bool radar = false;
    
    std::unordered_map<std::string, bool*> getFeatureMap() {
        return {
            {"aimbot", &aimbot},
            {"esp", &esp},
            {"wallhack", &wallhack},
            {"triggerbot", &triggerbot},
            {"norecoil", &noRecoil},
            {"speedhack", &speedhack},
            {"godmode", &godMode},
            {"radar", &radar}
        };
    }
};

extern CheatConfig g_config;
extern std::mutex g_config_mutex;
