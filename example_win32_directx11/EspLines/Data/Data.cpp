#include "Data.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <EspLines/Aimbot/MemoryAim.hpp>
#include <EspLines/Aimbot/RageAimbot.hpp>
#include <EspLines/Aimbot/SilentAim.hpp>
#include <EspLines/Exploits/FastSwitch.hpp>
#include <EspLines/Exploits/NoRecoil.hpp>
#include <EspLines/Exploits/NoReload.hpp>
#include <EspLines/Exploits/UnlimitedAmmo.hpp>
#include <EspLines/Exploits/PullPlayer.hpp>
#include <EspLines/Exploits/SpeedTimer.hpp>
#include <EspLines/Exploits/TeleKill.hpp>
#include <EspLines/Exploits/UnderPlayer.hpp>
#include <EspLines/Exploits/Fly.hpp>
#include <EspLines/Exploits/Teleport.hpp>
#include <EspLines/Exploits/WeaponAttributes.hpp>
#include <EspLines/Exploits/LagManager.hpp>
#include <EspLines/Exploits/TurnEnemies.hpp>
#include <EspLines/Exploits/SpinBot.hpp>
#include <EspLines/Exploits/TpWall.hpp>
#define NOMINMAX
#include <Windows.h>
#undef min
#include <vector>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <intrin.h>

namespace {
    // ==================================================================
    // HILO DE ESCRITURA DEL VISION HACK: el juego reescribe el FOV de la
    // camara cada frame, por eso la escritura por frame no alcanza. Este
    // hilo escribe el FOV deseado a cadencia fija (~250us, spin QPC).
    // La direccion virtual la publica Frame() (hilo principal) recien leida.
    // ==================================================================
    std::atomic<bool> s_visionRunning{ false };
    std::atomic<uint32_t> s_visionCamera{ 0 }; // followCamera + 0x44 es el float FOV
    std::atomic<float> s_visionFov{ 6.0f };
    std::thread s_visionThread;

    void VisionWorker() {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        const LONGLONG step = freq.QuadPart / 8000; // ~125 us

        while (s_visionRunning) {
            try {
                uint32_t cam = s_visionCamera.load(std::memory_order_acquire);
                if (cam != 0) {
                    const float fov = s_visionFov.load(std::memory_order_relaxed);
                    for (int i = 0; i < 16; i++)
                        Mem.Write<float>(cam + 0x44, fov);

                    LARGE_INTEGER now;
                    QueryPerformanceCounter(&now);
                    LONGLONG target = now.QuadPart + step;
                    do {
                        QueryPerformanceCounter(&now);
                        _mm_pause();
                    } while (now.QuadPart < target && s_visionRunning);
                }
                else {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void VisionStart() {
        if (s_visionRunning) return;
        s_visionRunning = true;
        s_visionThread = std::thread(VisionWorker);
    }

    void VisionStop() {
        if (!s_visionRunning) return;
        s_visionRunning = false;
        if (s_visionThread.joinable())
            s_visionThread.join();
    }
} // namespace

namespace FWork {

// ============================================================================
// FLUJO PRINCIPAL: SE EJECUTA CADA FRAME
// ============================================================================

// Prototipos (definidos mas abajo en este archivo)
static void ReadBone(uint32_t entity, uintptr_t boneOffs, Vector3& out);
static void ReadEntitySkeleton(uint32_t entity, Player::SkeletonBones& s);
static void UpdateExploitKeys();

void Data::Work() {
    // --- 1. Obtener el juego actual ---
    GameContext ctx;
    ctx.currentGame = GetCurrentGame();
    if (!ctx.currentGame) {
        // No hay juego: limpiamos todo y salimos
        Reset();
        Mem.Cache.clear();
        return;
    }

    // --- 2. Obtener la partida actual ---
    ctx.currentMatch = GetCurrentMatch(ctx.currentGame);
    if (!ctx.currentMatch) {
        // No hay partida activa: limpiamos todo y salimos
        Reset();
        Mem.Cache.clear();
        return;
    }

    // --- 3. Detectar cambio de partida: limpiamos entidades antiguas ---
    uint32_t currentMatchId = (uint32_t)ctx.currentMatch;
    if (g_Globals.EspConfig.LastMatchId != currentMatchId) {
        g_Globals.EspConfig.LastMatchId = currentMatchId;
        Reset();
    }

    // --- 3b. Teclas toggle de las funciones activas: se procesan SIEMPRE
    // (incluso en lobby/menu, sin camara valida) para que las teclas
    // asignadas activen/desactiven su funcion aunque no haya partida ---
    UpdateExploitKeys();

    // --- 4. Obtener el jugador local y configurar la camara ---
    ctx.localPlayer = Mem.Read<uint32_t>(ctx.currentMatch + Offsets::LocalPlayer);
    if (!ctx.localPlayer || !SetupLocalPlayerAndCamera(ctx.currentMatch)) {
        // Sin jugador local o camara invalida: limpiamos y salimos
        Reset();
        Mem.Cache.clear();
        return;
    }

    // --- 5. Procesar todas las entidades (jugadores) de la partida ---
    ProcessEntities(ctx);

    // --- 5b. Esqueleto del personaje local (solo si el switch esta activo) ---
    if (g_Globals.Visuals.LocalSkeleton) {
        ReadEntitySkeleton(ctx.localPlayer, g_Globals.EspConfig.LocalSkeleton);
    }

    // --- 5c. Exploits por frame (FastSwitch / NoRecoil / NoReload / SpeedTimer / TeleKill) ---
    FastSwitch::OnFrame(ctx.localPlayer);
    uint32_t weaponAddr = Mem.Read<uint32_t>(ctx.localPlayer + Offsets::Weapon);
    NoRecoil::OnFrame(ctx.localPlayer, weaponAddr);
    uint32_t playerAttrs = Mem.Read<uint32_t>(ctx.localPlayer + Offsets::LocalPlayerAttributes);
    NoReload::OnFrame(ctx.localPlayer, playerAttrs);
    UnlimitedAmmo::OnFrame(ctx.localPlayer, playerAttrs);
    SpeedTimer::Frame();
    TeleKill::Frame();
    UnderPlayer::Frame();
    Fly::Frame();
    PullPlayer::Frame();
    Teleport::Frame();
    LagManager::Frame();
    TurnEnemies::Frame();
    SpinBot::Frame();
    TpWall::Frame();

    // --- 5d. SPEED HACK (correr rapido) y MEJORA DE ARMAS (WeaponAttributes) ---
    // Speed hack: multiplica la velocidad de carrera/caida del jugador.
    // Al apagarse solo restaura si WeaponAttributes no controla los campos.
    static bool s_speedHackPrev = false;
    const bool speedHackOn = g_Globals.Exploits.SpeedHack;
    if (speedHackOn && playerAttrs) {
        const float s = g_Globals.Exploits.SpeedHackMultiplier;
        Mem.Write<float>(playerAttrs + Offsets::RunSpeedUpScale, s);
        Mem.Write<float>(playerAttrs + Offsets::FallingSpeedUpScale, s);
    }
    else if (s_speedHackPrev && !g_Globals.Exploits.MiniUziSpeed && !g_Globals.Exploits.WeaponAttributes) {
        Mem.Write<float>(playerAttrs + Offsets::RunSpeedUpScale, 1.0f);
        Mem.Write<float>(playerAttrs + Offsets::FallingSpeedUpScale, 1.0f);
    }
    s_speedHackPrev = speedHackOn;

    // --- 5c-2. JUMP HACK (salto alto): multiplica la altura del salto.
    // Al apagarse (o si JumpHack no controla el campo) restaura a 1.0 ---
    static bool s_jumpHackPrev = false;
    const bool jumpHackOn = g_Globals.Exploits.JumpHack;
    if (jumpHackOn && playerAttrs) {
        const float j = g_Globals.Exploits.JumpHeightMultiplier;
        Mem.Write<float>(playerAttrs + Offsets::PlayerAttributes_JumpHeightScale, j);
    }
    else if (s_jumpHackPrev) {
        Mem.Write<float>(playerAttrs + Offsets::PlayerAttributes_JumpHeightScale, 1.0f);
    }
    s_jumpHackPrev = jumpHackOn;

    // --- 5c-3. VISION HACK (FOV): escribe el FOV deseado en la camara de
    // seguimiento. Al apagarse se restaura cualquier valor que tuviera. ---
static bool s_visionPrev = false;
    static float s_visionOriginal = 0.0f;
    const bool visionOn = g_Globals.Exploits.VisionHack;
    if (visionOn) {
        uint32_t followCamera = 0;
        if (Mem.Read<uint32_t>(ctx.localPlayer + Offsets::FollowCamera, followCamera) && followCamera != 0) {
            if (!s_visionPrev) {
                Mem.Read<float>(followCamera + 0x44, s_visionOriginal);
                VisionStart();
            }
            s_visionCamera.store(followCamera, std::memory_order_release);
            s_visionFov.store(g_Globals.Exploits.VisionSlider, std::memory_order_relaxed);
        }
    }
    else {
        // Restaurar el FOV original que se capturo al activarse
        if (s_visionPrev) {
            uint32_t followCamera = 0;
            if (Mem.Read<uint32_t>(ctx.localPlayer + Offsets::FollowCamera, followCamera) && followCamera != 0)
                Mem.Write<float>(followCamera + 0x44, s_visionOriginal);
        }
        VisionStop();
        s_visionCamera.store(0, std::memory_order_release);
    }
    s_visionPrev = visionOn;

    // --- 5c-4. CAIDA RAPIDA: escribe la velocidad de caida en los dos campos
    // del PlayerAttributes (0x404). Al apagarse restaura los valores previos. ---
    static bool s_fastFallPrev = false;
    static float s_fallOrig1 = 0.0f, s_fallOrig2 = 0.0f;
    const bool fastFallOn = g_Globals.Exploits.FastFall;
    if (fastFallOn) {
        uint32_t fallAttrs = 0;
        if (Mem.Read<uint32_t>(ctx.localPlayer + Offsets::FallAttributes, fallAttrs) && fallAttrs != 0) {
            if (!s_fastFallPrev) {
                Mem.Read<float>(fallAttrs + Offsets::FallSpeedScale, s_fallOrig1);
                Mem.Read<float>(fallAttrs + Offsets::FallSpeedScaleTwo, s_fallOrig2);
            }
            const float f = g_Globals.Exploits.FastFallSpeed;
            Mem.Write<float>(fallAttrs + Offsets::FallSpeedScale, f);
            Mem.Write<float>(fallAttrs + Offsets::FallSpeedScaleTwo, f);
        }
    }
    else if (s_fastFallPrev) {
        uint32_t fallAttrs = 0;
        if (Mem.Read<uint32_t>(ctx.localPlayer + Offsets::FallAttributes, fallAttrs) && fallAttrs != 0) {
            Mem.Write<float>(fallAttrs + Offsets::FallSpeedScale, s_fallOrig1);
            Mem.Write<float>(fallAttrs + Offsets::FallSpeedScaleTwo, s_fallOrig2);
        }
    }
    s_fastFallPrev = fastFallOn;

    WeaponAttributes::Apply(ctx.localPlayer, g_Globals.Exploits.WeaponLevel,
        g_Globals.Exploits.WeaponAttributes, g_Globals.Exploits.MiniUziSpeed,
        g_Globals.Exploits.MiniUziSpeedMultiplier);

    // --- 6. Actualizar los aimbots con la lista de entidades ---
    // El hilo del silent aim arranca aqui (lazy), con la memoria verificada.
    Aim::SilentAimStart();
    Aim::MemoryAimWork();
    Aim::RageAimbot::Aimbot();
    Aim::SilentAimUpdate();
}

// ============================================================================
// OBTENER EL JUEGO ACTUAL
// Cadena: Il2Cpp -> GameFacade -> StaticClass -> Instancia estatica
// ============================================================================
uint32_t Data::GetCurrentGame() {
    // Leer la fachada base del juego
    uint32_t baseGameFacade = Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase);
    if (!baseGameFacade) return 0;

    // Leer la fachada del juego
    uint32_t gameFacade = Mem.Read<uint32_t>(baseGameFacade);
    if (!gameFacade) return 0;

    // Leer la clase estatica
    uint32_t staticGameFacade = Mem.Read<uint32_t>(gameFacade + Offsets::StaticClass);
    if (!staticGameFacade) return 0;

    // Devolver la instancia estatica del juego
    return Mem.Read<uint32_t>(staticGameFacade);
}

// ============================================================================
// OBTENER LA PARTIDA ACTUAL
// ============================================================================
uint32_t Data::GetCurrentMatch(uint32_t currentGame) {
    return currentGame ? Mem.Read<uint32_t>(currentGame + Offsets::CurrentMatch) : 0;
}

// ============================================================================
// CONFIGURAR JUGADOR LOCAL Y CAMARA
// Cadena: LocalPlayer -> MainCameraTransform -> posicion
//         LocalPlayer -> FollowCamera -> Camera -> ViewMatrix
// ============================================================================
bool Data::SetupLocalPlayerAndCamera(uint32_t currentMatch) {
    if (!currentMatch) return false;

    // --- Jugador local ---
    uint32_t localPlayer = Mem.Read<uint32_t>(currentMatch + Offsets::LocalPlayer);
    if (!localPlayer) return false;
    g_Globals.EspConfig.LocalPlayer = localPlayer;

    // --- Posicion de la camara principal ---
    uint32_t mainTransform = Mem.Read<uint32_t>(localPlayer + Offsets::MainCameraTransform);
    if (!mainTransform) return false;
    TransformUtils::GetPosition(mainTransform, g_Globals.EspConfig.MainCamera);

    // --- View matrix de la camara ---
    uint32_t followCamera = Mem.Read<uint32_t>(localPlayer + Offsets::FollowCamera);
    if (!followCamera) return false;
    uint32_t camera = Mem.Read<uint32_t>(followCamera + Offsets::Camera);
    if (!camera) return false;
    uint32_t cameraBase = Mem.Read<uint32_t>(camera + 0x8);
    if (!cameraBase) return false;

    g_Globals.EspConfig.ViewMatrix = Mem.Read<Matrix4x4>(cameraBase + Offsets::ViewMatrix);
    g_Globals.EspConfig.Matrix = true;
    return true;
}

// ============================================================================
// TECLAS TOGGLE DE LAS FUNCIONES ACTIVAS: presionar la tecla asignada
// enciende/apaga el switch correspondiente (pestana EXPLOITS).
// ============================================================================
static void UpdateExploitKeys() {
    static bool pFs = false, pNr = false, pNl = false, pTk = false;
    static bool pUa = false;
    static bool pFl = false, pFr = false, pFr2 = false, pTp = false, pV2 = false;
    static bool pTe = false, pSp = false, pJmp = false, pVsn = false, pFf = false;
    auto toggle = [](int key, bool& prev, bool& flag) {
        if (key == 0) {
            // Sin tecla asignada: reiniciar el estado anterior para que
            // el siguiente flanco (al reasignar la tecla) cuente como
            // pulso nuevo y no quede "comido" por un prev stale.
            prev = false;
            return;
        }
        bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
        if (down && !prev) flag = !flag;
        prev = down;
    };
    toggle(g_Globals.Exploits.FastSwitchKey, pFs, g_Globals.Exploits.FastSwitch);
    toggle(g_Globals.Exploits.NoRecoilKey, pNr, g_Globals.Exploits.NoRecoil);
    toggle(g_Globals.Exploits.NoReloadKey, pNl, g_Globals.Exploits.NoReload);
    toggle(g_Globals.Exploits.UnlimitedAmmoKey, pUa, g_Globals.Exploits.UnlimitedAmmo);
    toggle(g_Globals.Exploits.TeleKillKey, pTk, g_Globals.Exploits.TeleKill);
    toggle(g_Globals.Exploits.GhostLagKey, pFl, g_Globals.Exploits.GhostLag);
    toggle(g_Globals.Exploits.FakeLagKey, pFr, g_Globals.Exploits.FakeLag);
    toggle(g_Globals.Exploits.TeleportLagKey, pTp, g_Globals.Exploits.TeleportLag);
    toggle(g_Globals.Exploits.TurnEnemiesKey, pTe, g_Globals.Exploits.TurnEnemies);
    toggle(g_Globals.Exploits.SpinBotKey, pSp, g_Globals.Exploits.SpinBot);
    toggle(g_Globals.Exploits.JumpHackKey, pJmp, g_Globals.Exploits.JumpHack);
    toggle(g_Globals.Exploits.VisionHackKey, pVsn, g_Globals.Exploits.VisionHack);
    toggle(g_Globals.Exploits.FastFallKey, pFf, g_Globals.Exploits.FastFall);
}

// ============================================================================
// LEER UN HUESO DEL ESQUELETO (address del nodo -> posicion mundial)
// ============================================================================
static void ReadBone(uint32_t entity, uintptr_t boneOffs, Vector3& out) {
    uint32_t boneAddr = 0;
    if (Mem.Read(entity + boneOffs, boneAddr) && boneAddr) {
        TransformUtils::GetNodePosition(boneAddr, out);
    }
}

// ============================================================================
// LEER EL ESQUELETO COMPLETO DE UN JUGADOR (solo con el ESP skeleton activo)
// ============================================================================
static void ReadEntitySkeleton(uint32_t entity, Player::SkeletonBones& s) {
    ReadBone(entity, Offsets::Bones::Head, s.Head);
    ReadBone(entity, Offsets::Bones::Neck, s.Neck);
    ReadBone(entity, Offsets::Bones::Spine, s.Spine);
    ReadBone(entity, Offsets::Bones::Pelvis, s.Pelvis);
    ReadBone(entity, Offsets::Bones::LeftShoulder, s.LeftShoulder);
    ReadBone(entity, Offsets::Bones::LeftElbow, s.LeftElbow);
    ReadBone(entity, Offsets::Bones::LeftHand, s.LeftHand);
    ReadBone(entity, Offsets::Bones::RightShoulder, s.RightShoulder);
    ReadBone(entity, Offsets::Bones::RightElbow, s.RightElbow);
    ReadBone(entity, Offsets::Bones::RightHand, s.RightHand);
    ReadBone(entity, Offsets::Bones::LeftAnkle, s.LeftAnkle);
    ReadBone(entity, Offsets::Bones::LeftFoot, s.LeftFoot);
    ReadBone(entity, Offsets::Bones::RightAnkle, s.RightAnkle);
    ReadBone(entity, Offsets::Bones::RightFoot, s.RightFoot);
}

// ============================================================================
// PROCESAR ENTIDADES DE LA PARTIDA
// Recorre el diccionario de entidades y agrega TODAS sin excepcion.
// No se aplica ningun filtro (visibilidad, equipo, muerto, distancia, etc.).
// ============================================================================
void Data::ProcessEntities(const GameContext& ctx) {
    // --- Obtener el diccionario de entidades ---
    uint32_t entityDictionary = Mem.Read<uint32_t>(ctx.currentGame + Offsets::DictionaryEntities);
    if (!entityDictionary) return;

    // --- Obtener la lista de entidades (solo players reales, offset 0x10) ---
    uint32_t entries = Mem.Read<uint32_t>(entityDictionary + 0xC);
    if (!entries) return;

    uint32_t entities = entries + 0x10;
    uint32_t entitiesCount = Mem.Read<uint32_t>(entityDictionary + 0x10);
    g_Globals.EspConfig.previousCount = entitiesCount;

    // Limite de seguridad para evitar lecturas fuera de rango
    if (entitiesCount < 1 || entitiesCount > 10000) return;

    // Datos auxiliares
    Vector3 mainPos = g_Globals.EspConfig.MainCamera;
    float currentTime = (float)GetTickCount64() / 1000.0f;

    // --- Recorrer cada entrada del diccionario ---
    for (uint32_t i = 0; i < entitiesCount; ++i) {
        uint32_t entry = entities + (i * 0x10);

        // Hash de la entidad (debe ser positivo)
        int hash = 0;
        if (!Mem.Read(entry + 0x0, hash) || hash < 0)
            continue;

        // Direccion de la entidad (no puede ser nula ni el jugador local)
        uint32_t entity = 0;
        if (!Mem.Read(entry + 0xC, entity) || entity == 0 || entity == ctx.localPlayer)
            continue;

        try {
            // Agregar la entidad sin excepcion
            Player& player = g_Globals.EspConfig.Entities[entity];
            player.Address = entity;

            // ==================================================================
            // DATOS DEL AVATAR (si existen, se leen; si no, quedan por defecto)
            // ==================================================================
            uint32_t avatarManager = Mem.Read<uint32_t>(entity + Offsets::AvatarManager);
            uint32_t avatar = avatarManager ? Mem.Read<uint32_t>(avatarManager + Offsets::Avatar) : 0;
            uint32_t avatarData = avatar ? Mem.Read<uint32_t>(avatar + Offsets::Avatar_Data) : 0;

            if (avatar) {
                player.IsVisible = Mem.Read<bool>(avatar + Offsets::Avatar_IsVisible);
            }
            else {
                player.IsVisible = false;
            }

            if (avatarData) {
                player.IsTeam = Mem.Read<bool>(avatarData + Offsets::Avatar_Data_IsTeam)
                    ? Player::Bool3::True : Player::Bool3::False;
            }
            else {
                player.IsTeam = Player::Bool3::Unknown;
            }

            // Flags de bot de los offsets nuevos (se combinan con la heuristica del nombre)
            bool botFlag = false;
            if (avatarData && Mem.Read<bool>(avatarData + Offsets::Avatar_Data_IsBot, botFlag) && botFlag)
                player.IsBot = true;
            if (Mem.Read<bool>(entity + Offsets::IsClientBot, botFlag) && botFlag)
                player.IsBot = true;

            // ==================================================================
            // ESTADOS DE VIDA (muerto / derribado)
            // ==================================================================
            player.IsDead = Mem.Read<bool>(entity + Offsets::Player_IsDead);

            uint32_t shadowBase = Mem.Read<uint32_t>(entity + Offsets::Player_ShadowBase);
            player.IsKnocked = shadowBase ? (Mem.Read<int>(shadowBase + Offsets::XPose) == 8) : false;

            // ==================================================================
            // VIDA Y ARMA (lectura opcional)
            // ==================================================================
            uint32_t dataPool = Mem.Read<uint32_t>(entity + Offsets::Player_Data);
            if (dataPool) {
                uint32_t poolObj = Mem.Read<uint32_t>(dataPool + 0x8);
                if (poolObj) {
                    uint32_t pool = Mem.Read<uint32_t>(poolObj + 0x10);
                    if (pool) {
                        player.Health = Mem.Read<short>(pool + Offsets::Vida);

                        uint32_t weaponptr = Mem.Read<uint32_t>(poolObj + 0x20);
                        player.WeaponID = weaponptr ? Mem.Read<short>(weaponptr + Offsets::Vida) : (short)0;
                    }
                }
            }

            // ==================================================================
            // NOMBRE DEL JUGADOR
            // ==================================================================
            uint32_t nameAddr = Mem.Read<uint32_t>(entity + Offsets::Player_Name);
            if (nameAddr) {
                int nameLen = Mem.Read<int>(nameAddr + 0x8);
                if (nameLen > 0 && nameLen < 128) {
                    player.Name = Mem.String(nameAddr + 0xC, nameLen * 2, true);
                    player.IsKnown = true;
                    // Heuristica por nombre: los bots del juego usan "Player" + solo digitos
                    const std::string& n = player.Name;
                    if (n.size() >= 7 && n.compare(0, 6, "Player") == 0) {
                        player.IsBot = player.IsBot ||
                            (n.find_first_not_of("0123456789", 6) == std::string::npos);
                    }
                }
            }

            // ==================================================================
            // HUESOS (Head, Hip y Root)
            // ==================================================================
            uint32_t boneAddr = 0;
            if (Mem.Read(entity + Offsets::Bones::Head, boneAddr) && boneAddr) {
                TransformUtils::GetNodePosition(boneAddr, player.Head);
            }
            if (Mem.Read(entity + Offsets::Bones::Neck, boneAddr) && boneAddr) {
                TransformUtils::GetNodePosition(boneAddr, player.Neck);
            }
            if (Mem.Read(entity + Offsets::Bones::Hip, boneAddr) && boneAddr) {
                TransformUtils::GetNodePosition(boneAddr, player.Hip);
            }
            if (Mem.Read(entity + Offsets::Bones::Root, boneAddr) && boneAddr) {
                TransformUtils::GetNodePosition(boneAddr, player.Root);
            }

            // ==================================================================
            // ESQUELETO COMPLETO (solo si el ESP skeleton esta activo)
            // ==================================================================
            if (g_Globals.Visuals.Skeleton) {
                ReadEntitySkeleton(entity, player.Skeleton);
            }

            // ==================================================================
            // DISTANCIA AL JUGADOR LOCAL (usa Head, o Hip como respaldo)
            // ==================================================================
            Vector3 refPos = (player.Head != Vector3::Zero()) ? player.Head : player.Hip;
            if (refPos != Vector3::Zero()) {
                player.Distance = Vector3::Distance(mainPos, refPos);
            }

            // ==================================================================
            // VELOCIDAD ESTIMADA (para prediccion de balas del silent aim)
            // ==================================================================
            float dt = currentTime - player.LastUpdateTime;
            if (dt > 0.0f && dt < 0.25f) {
                Vector3 inst = (player.Head - player.LastHead) * (1.0f / dt);
                player.Velocity = (player.Velocity + inst) * 0.5f; // suavizado anti-jitter
            }
            else {
                player.Velocity = Vector3::Zero();
            }

            // ==================================================================
            // CACHE: Guardar ultimas posiciones y tiempo de actualizacion
            // ==================================================================
            player.LastHead = player.Head;
            player.LastHip = player.Hip;
            player.LastUpdateTime = currentTime;

        }
        catch (...) {
            // Si falla la lectura, la entidad se descarta
            g_Globals.EspConfig.Entities.erase(entity);
        }
    }

    // --- Limpieza agresiva: quitar entidades que desaparecieron (timeout 0.05s) ---
    for (auto it = g_Globals.EspConfig.Entities.begin(); it != g_Globals.EspConfig.Entities.end(); ) {
        if (currentTime - it->second.LastUpdateTime > 0.05f) {
            it = g_Globals.EspConfig.Entities.erase(it);
        }
        else {
            ++it;
        }
    }
}

// ============================================================================
// RESET: Limpia la lista de entidades
// ============================================================================
void Data::Reset() {
    g_Globals.EspConfig.Entities.clear();
    g_Globals.EspConfig.LocalSkeleton = Player::SkeletonBones{};
}

} // namespace FWork
