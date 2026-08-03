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
        // Hueso a apuntar con el silent aim (selector propio en la pestana AIMBOT)
        Config::Bone TargetBone = Config::Bone::Head;
        // Velocidad de bala (m/s) para predecir el movimiento del target:
        // lead = velocidad del target * tiempo de vuelo.
        float BulletSpeed = 950.0f;
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

        // ---- EXPLOITS POR FRAME (pestana EXPLOITS) ----
        bool FastSwitch = false;  // cambio de arma instantaneo
        bool NoRecoil = false;    // sin retroceso
        bool NoReload = false;    // sin recarga

        // ---- SPEED TIMER (acelera el tiempo del juego) ----
        bool SpeedTimer = false;
        float SpeedMultiplier = 2.0f;

        // ---- SPEED HACK (correr rapido: RunSpeedUpScale/FallingSpeedUpScale) ----
        bool SpeedHack = false;
        float SpeedHackMultiplier = 1.5f;

        // ---- MEJORA DE ARMAS (WeaponAttributes) ----
        bool WeaponAttributes = false;
        int WeaponLevel = 0;           // 0=LV1, 1=LV2, 2=LV3, 3=LV4 (extras)
        int BoostWeaponId = 7;         // arma a mejorar (0 = todas, 7=UMP, 28=XM8, 58=MiniUzi)
        bool MiniUziSpeed = false;
        float MiniUziSpeedMultiplier = 1.35f;

        // ---- UNDER PLAYER (hundirse bajo el jugador) ----
        bool UnderPlayer = false;
        int UnderPlayerKey = 0; // tecla: volver a la posicion original

        // ---- PULL PLAYER (jalar enemigos al disparar) ----
        bool PullPlayer = false;
        int PullPlayerKey = 0;     // tecla: mantener para jalar (ademas de disparar)
        int PullBone = 0;          // 0=Head, 1=Spine, 2=Root
        float PullDistance = 10.0f;  // alcance maximo en metros
        float MinPullDistance = 1.0f; // distancia minima para no jalar a quien esta encima
        float PullScreenRange = 150.0f; // px desde el centro de la pantalla
        float PullSmooth = 1.0f;     // velocidad del jale (mas alto = mas rapido)

        // ---- TECLAS TOGGLE (presionar para activar/desactivar) ----
        int FastSwitchKey = 0;
        int NoRecoilKey = 0;
        int NoReloadKey = 0;

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

        // ---- Esqueleto (pestana ESP -> VISUALES) ----
        bool Skeleton = false;        // huesos de los enemigos
        bool LocalSkeleton = false;   // huesos del personaje local
        float SkeletonColor[4] = { 0.f, 1.f, 0.5f, 1.f };

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
        Player::SkeletonBones LocalSkeleton;  // huesos del personaje local
    } EspConfig;

};

inline Globals g_Globals;