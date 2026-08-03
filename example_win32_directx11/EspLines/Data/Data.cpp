#include "Data.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <EspLines/Aimbot/MemoryAim.hpp>
#define NOMINMAX
#include <Windows.h>
#undef min
#include <vector>
#include <cmath>

namespace FWork {

// ============================================================================
// FLUJO PRINCIPAL: SE EJECUTA CADA FRAME
// ============================================================================
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

    // --- 6. Actualizar el aimbot con la lista de entidades ---
    Aim::MemoryAimWork();
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
            if (avatarData) {
                player.IsTeam = Mem.Read<bool>(avatarData + Offsets::Avatar_Data_IsTeam)
                    ? Player::Bool3::True : Player::Bool3::False;
            }

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
                }
            }

            // ==================================================================
            // HUESOS (Head, Hip y Root)
            // ==================================================================
            uint32_t boneAddr = 0;
            if (Mem.Read(entity + Offsets::Bones::Head, boneAddr) && boneAddr) {
                TransformUtils::GetNodePosition(boneAddr, player.Head);
            }
            if (Mem.Read(entity + Offsets::Bones::Hip, boneAddr) && boneAddr) {
                TransformUtils::GetNodePosition(boneAddr, player.Hip);
            }
            if (Mem.Read(entity + Offsets::Bones::Root, boneAddr) && boneAddr) {
                TransformUtils::GetNodePosition(boneAddr, player.Root);
            }

            // ==================================================================
            // DISTANCIA AL JUGADOR LOCAL (usa Head, o Hip como respaldo)
            // ==================================================================
            Vector3 refPos = (player.Head != Vector3::Zero()) ? player.Head : player.Hip;
            if (refPos != Vector3::Zero()) {
                player.Distance = Vector3::Distance(mainPos, refPos);
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
}

} // namespace FWork
