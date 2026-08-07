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
        // Gravedad de la bala (m/s^2) para compensar caida (drop).
        // Free Fire ~12-15; 0 = desactivado.
        float Gravity = 12.5f;
        // Precisión de hip-fire (0.0-1.0): 1.0 = máxima precisión, 0.5 = 50% más permisivo
        float HipFireAccuracy = 1.0f;
        // Solo apunta a entidades VISIBLES (Avatar_IsVisible), como el ESP
        bool OnlyVisible = false;
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
        bool UnlimitedAmmo = false; // municion infinita (PlayerAttributes + 0xE0)

        // ---- SPEED TIMER (acelera el tiempo del juego) ----
        bool SpeedTimer = false;
        float SpeedMultiplier = 2.0f;

        // ---- SPEED HACK (correr rapido: RunSpeedUpScale/FallingSpeedUpScale) ----
        bool SpeedHack = false;
        float SpeedHackMultiplier = 1.5f;

        // ---- JUMP (salto alto: BuffEcaJumpHeightScale) ----
        bool JumpHack = false;
        int JumpHackKey = 0;      // tecla: activar/desactivar (opcional)
        float JumpHeightMultiplier = 2.0f;

        // ---- VISION (FOV de la camara de seguimiento) ----
        bool VisionHack = false;
        int VisionHackKey = 0;    // tecla: activar/desactivar (opcional)
        float VisionSlider = 6.0f; // FOV a escribir en followCamera + 0x44 (escala 0-10)

        // ---- CAIDA RAPIDA (velocidad de caida) ----
        bool FastFall = false;
        int FastFallKey = 0;       // tecla: activar/desactivar (opcional)
        float FastFallSpeed = 25.0f; // velocidad de caida a escribir en +0x15C/+0x160

        // ---- MEJORA DE ARMAS (WeaponAttributes) ----
        bool WeaponAttributes = false;
        int WeaponLevel = 0;           // 0=LV1, 1=LV2, 2=LV3, 3=LV4 (extras)
        int BoostWeaponId = 7;         // arma a mejorar (0 = todas, 7=UMP, 28=XM8, 58=MiniUzi)
        bool MiniUziSpeed = false;
        float MiniUziSpeedMultiplier = 1.35f;

        // ---- UNDER PLAYER (hundirse bajo el jugador) ----
        bool UnderPlayer = false;
        int UnderPlayerKey = 0; // tecla: activar/desactivar (hundirse/volver)

        // ---- FLY (vuelo simple: sube X metros en Y al activarlo) ----
        bool Fly = false;
        int FlyKey = 0;      // tecla: activar/desactivar (subir/volver)
        float FlyHeight = 1.0f; // metros que sube al activar (0.1-10)

        // ---- PULL PLAYER (jalar enemigos a la mira) ----
        bool PullPlayer = false;
        int PullPlayerKey = 0;     // tecla: activar/desactivar la funcion
        int PullBone = 0;          // 0=Head, 1=Spine, 2=Root
        float PullDis = 300.0f;    // distancia maxima para el pull
        float PullFov = 300.0f;    // FOV (px desde el centro de la pantalla)

        // ---- TELEPORT (clava tu posicion en la del enemigo mas cercano) ----
        bool Teleport = false;
        int TeleportKey = 0;           // tecla: activar/desactivar
        float TeleportDistance = 200.0f; // distancia maxima del enemigo (m)

        // ---- TURN 180 / MURCIELAGO (voltea a todos los enemigos patas arriba) ----
        bool TurnEnemies = false;      // flip 180 vertical: cabeza <-> pies en los enemigos
        int TurnEnemiesKey = 0;        // tecla: activar/desactivar

        // ---- SPIN BOT (hace girar a TU personaje sobre su eje vertical) ----
        bool SpinBot = false;          // gira tu personaje constantemente
        int SpinBotKey = 0;            // tecla: activar/desactivar
        float SpinBotSpeed = 360.0f;   // velocidad de giro en grados/segundo

        // ---- TP WALL (avanza un paso hacia donde mira la camara) ----
        bool TpWall = false;           // refleja activacion momentanea (1 clic = 1 paso)
        int TpWallKey = 0;             // tecla: cada pulsacion = 1 paso
        float TpWallDistance = 1.0f;   // metros por paso (0.1-10)

// ---- GHOST LAG / FAKE LAG / TELEPORT (WinDivert: paquetes) ----
        // GhostLag: retrasa TUS salientes -> el enemigo te ve congelado pero
        //    tu dano entra (el movimiento es lo que se retiene/reenvia).
        // FakeLag: retiene/descarta salientes de movimiento -> congela a los
        //    enemigos; al desactivar reenvia y tu dano acumulado les cuenta.
        // TeleportLag: corta salientes de movimiento + correcciones entrantes:
        //    te ven congelado en A y solo te ven al llegar a B. NO restaura
        //    la posicion original al desactivar.
        bool GhostLag = false;         // retrasa tus paquetes salientes
        int GhostLagKey = 0;           // tecla: activar/desactivar
        int GhostLagDelay = 5000;      // ms de retardo al reenviar cada paquete (ref: 5000)
        bool FakeLag = false;          // retarda en AMBOS sentidos (congela)
        int FakeLagKey = 0;            // tecla: activar/desactivar
        int FakeLagDelay = 2000;       // ms de retardo antes de reenviar (ref: 2000)
        bool TeleportLag = false;      // corta movimiento y correcciones (teleport por red)
        int TeleportLagKey = 0;        // tecla: activar/desactivar
        int TeleportLagMode = 0;       // 0 = solo saliente, 1 = ambos sentidos

        // ---- TECLAS TOGGLE (presionar para activar/desactivar) ----
        int FastSwitchKey = 0;
        int NoRecoilKey = 0;
        int NoReloadKey = 0;
        int UnlimitedAmmoKey = 0;

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