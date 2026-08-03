#pragma once
#include <Windows.h>
#include <EspLines/Player.h>
#include <EspLines/Math/Matrix4v4.hpp>
#include <unordered_map>
#include <mutex>
#include <string>
#include <imgui.h>

// 1. Definições de Tipos
namespace Config {
    enum class HitBox {
        Head,
        Peito
    };

    // Hueso objetivo para el Memory Aim
    enum class Bone {
        Head,
        Neck,
        Hip
    };

    struct MenuConfig {

        bool Opened = true;
        bool IsLoggedIn = false;
        char Username[256] = "";      // ? Username para KeyAuth
        char Password[256] = "";      // ? Password para KeyAuth  
        char LicenseKey[256] = "";    // ? Key se quiser usar Register_key
        // STATUS DO LOGIN
        std::string StatusText = "";
        ImVec4 StatusColor = ImVec4(0, 1, 0.6f, 1);
    };
}

// 2. Classe Global
class Globals {
public:
    // Membros de Configuração
    Config::MenuConfig Menu;

    struct AimBot {
        bool Enabled;
        bool MemoryAim = false; // Adicione esta linha aimbot novo do pai
        bool AimShoulder;
        bool NoRecoil;
        bool FastReload;
        bool AimbotFFMAX;
        bool IgnoreKnocked;
        bool IgnoreBots;
        int DistanceAim = 250;
        int Fov = 200.0f;
        float Fillcolor[4] = { 0.f, 0.f, 0.f, 0.2f };
        int AimbotBind = VK_LBUTTON;
        int UpdateInterval = 10;
        float AimSmooth = 1.0f; // Suavização do Aimbot
        Config::HitBox HitBox = Config::HitBox::Head;

        // ---- RAGE AIM (modo 2: fija el collider del enemigo) ----
        bool RageAim = false;

        // ---- TARGET del Memory Aim (hueso a apuntar) ----
        Config::Bone TargetBone = Config::Bone::Head;

        // Filtro opcional: solo aimear a enemigos (por defecto apunta a cualquier entidad)
        bool OnlyEnemies = false;
    }
    AimBot;

    // HitBox lo usa TeleKill; Enabled es el switch del silent aim
    struct Silent {
        bool Enabled = false;
        Config::HitBox HitBox = Config::HitBox::Head;
    }
    Silent;
    struct Exploits {
       

        int TeleKillBind = 0;
       

        //INICIO AIMBOT DO PAI 
        bool MemoryAim = false;
        //FIM AIMBOT DO PAI

        bool TeleKill = false;
        int TeleKillKey = 0; // Começa como "None"
        float TeleKillDistance = 10.0f; //

    }
    Exploits;

    struct Visuals
    {
        float x, y;

        float LeftKneeOffset = -0.25f;
        float RightKneeOffset = -0.35f;

        int DistanceEsp = 250;
        bool Enable = true;
        bool Watermark = false;
        bool Enemy = true;
        bool Lines = false;
        int EspLines = 0;
        float LinesColor[4] = { 1.f, 1.f, 1.f, 1.f };
        float KnockedColor[4] = { 0.f, 153.f / 255.f, 153.f / 255.f, 1.f }; // Alterado aqui
        bool FilledBox = false;
        float Filledboxcolor[4] = { 0.f, 0.f, 0.f, 0.4f };

        bool Box = false;
        float BoxColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool ESPHealthTEXT;
        bool HealthBar = false;
        float texthColor[4] = { 1.f, 1.f, 1.f, 1.f };

        //bool ESPWeapon = false;
       // bool ESPWeaponIcon = false;
       // float ESPWeaponColor[4] = { 1.f, 1.f, 1.f, 1.f };

        // bool Skeleton = false; (REMOVIDO PARA OTIMIZAÇÃO)
        // float SkeletonColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Alvo = false;
        float AlvoColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Name = false;
        float NameColor[4] = { 1.f, 1.f, 1.f, 1.f };

        bool Distance = false;
        float DistColor[4] = { 0.7f, 0.7f, 0.7f, 1.f };

        bool ESPGranada = false;
        float ESPGranadaColor[4] = { 0.f, 153.f / 255.f, 153.f / 255.f, 1.f }; // Alterado aqui

        float WatermarkColor[4] = { 0.f, 153.f / 255.f, 153.f / 255.f, 1.f }; // Alterado aqui
        float EnemyColor[4] = { 0.f, 153.f / 255.f, 153.f / 255.f, 1.f }; // Alterado aqui

        bool Debug = false;
        float HipHeightOffset = 0.7f;
        float HipWidthOffset = 0.33f;
        float HipWidthScale = 0.19f;
        float LeftHipHeightOffset = 0.82f;
        float RightHipHeightOffset = 0.98f;

        float LeftKneeHeightOffset = 0.5f;
        float RightKneeHeightOffset = 0.5f;
        float KneeWidthScale = 0.5f;
        float LeftKneeWidthOffset = 0.1f;
        float RightKneeWidthOffset = 0.1f;
        float LeftKneeFlexionOffset = 0.1f;
        float RightKneeFlexionOffset = 0.1f;

        // Dentro do struct de Visuals
        bool Minimap = false;

        // ---- Configuracion avanzada de ESP (pestana ESP -> SETTINGS) ----
        bool Glow = false;            // halo resaltado en caja/linea
        int GlowIntensity = 40;       // intensidad del halo (0-100)
        bool IgnoreBots = false;      // filtro: no mostrar bots
        bool IgnoreKnocked = false;   // filtro: no mostrar derribados
        bool OnlyVisible = false;     // filtro: solo enemigos visibles

        // ---- Colores por grupo (pestana ESP -> COLORS) ----
        struct EspGroupColors {
            float Line[4] = { 1.f, 1.f, 1.f, 1.f };
            float Box[4] = { 1.f, 1.f, 1.f, 1.f };
            float Fill[4] = { 0.f, 0.f, 0.f, 0.4f };
        };
        EspGroupColors ColNormal;
        EspGroupColors ColBots;
        EspGroupColors ColVisible;
        EspGroupColors ColKnocked = { { 1.f, 0.35f, 0.35f, 1.f }, { 1.f, 0.35f, 0.35f, 1.f }, { 0.4f, 0.f, 0.f, 0.4f } };

    } Visuals;

    struct Misc
    {
        bool ShowAimbotFov;
   //   float AimbotFovColor[4] = { 0.f, 153.f / 255.f, 153.f / 255.f, 1.f }; // Alterado aqui
        float AimbotFovColor[4] = { 0.92f, 0.92f, 0.92f, 1.f };
    } Misc;

    struct General
    {
        bool ShutDown = false;
        bool Capture = false;
        int Delay = 0;
        int MenuKey = VK_INSERT;
    } General;

    struct Esp {
        std::unordered_map<long, Player> Entities;
        std::mutex EntitiesMutex;
        Matrix4x4 ViewMatrix{};
        Vector3 MainCamera{};
        uint32_t LocalPlayer = 0;
        uint32_t previousCount = 0;
        uint32_t LastMatchId = 0;  // Para detectar mudança de partida
        bool Matrix = false;
        int Width = 0;
        int Height = 0;
    } EspConfig;

};

inline Globals g_Globals;