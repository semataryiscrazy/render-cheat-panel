#pragma once
#include <string>

namespace Cheat
{
    class Options
    {
    public:
        struct LegitBot
        {
            struct AimBot
            {
                bool Enabled = false;
                bool Precision = false;
                bool ScopeTracking = false;
                bool Pixel = false;
                int KeyBind = 1;
                int KeyBindState = 0;
                bool TargetNPC = false;
                bool IgnoreKnocked = true;
                int HitBox = 0;
                int MaxDistance = 250;
                int DelayMode = 1;
                int AimType = 0;
                float FOV = 10.f;
                float SmoothHorizontal = 80.f;
                float SmoothVertical = 80.f;
                float ColliderFOV = 100.f;
                bool ColliderVisibleOnly = false;
                bool PullCamAfterKill = false;
                float PullCamHeight = 0.5f;
            } AimBot;
        } LegitBot;

        struct Visuals
        {
            struct ESP
            {
                struct Players
                {
                    bool Enabled = true;
                    bool ShowLocalPlayer = true;
                    bool ShowNPCs = true;
                    int RenderDistance = 250;
                    bool Box = false;
                    int BoxStyle = 1; // 0=Full, 1=Cornered
                    bool Skeleton = false;
                    bool Name = false;
                    bool HealthBar = false;
                    int HealthBarPos = 0; // 0=Left, 1=Right, 2=Top, 3=Bottom
                    bool ArmorBar = false;
                    bool WeaponName = false;
                    bool Distance = false;
                    bool SnapLines = false;
                    int SnapLinesPos = 1; // 0=Top, 1=Bottom
                } Players;

                struct Vehicles
                {
                    bool Enabled = false;
                    bool LocalVehicle = false;
                    bool SnapLines = false;
                    bool Box = false;
                    bool Name = false;
                } Vehicles;
            } ESP;
        } Visuals;

        struct Exploits
        {
            bool NoRecoil = false;
            float NoRecoilStrength = 100.f;

            bool WeaponAttributes = false;
            int WeaponAttributesLevel = 1;

            bool SpinBot = false;
            float SpinBotSpeed = 10.f;

            bool SpeedHack = false;
            float SpeedMultiplier = 1.35f;

            bool FastMedkit = false;
            bool FastReload = false;

            bool SilentAimEnabled = false;
            int  SilentAimKeyBind = 1;
            int  SilentAimKeyBindState = 0;
            bool SilentAimUseFOV = true;
            float SilentAimFOV = 100.f;
            int SilentAimDistance = 250;
            int SilentAimHitBox = 0;
            bool SilentAimTargetBots = true;
        } Exploits;

        struct General
        {
            char Username[255] = "";
            char Password[255] = "";
            char Key[255] = "";
            bool CaptureBypass = true;
            bool StreamMode = false;
            bool BypassEnabled = false;
            int MenuToggleKey = 0x2D;
            int MenuToggleKeyState = 0;
        } General;
    };
}

extern Cheat::Options g_Options;
