#include "SilentAim.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#define NOMINMAX
#include <Windows.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <intrin.h>

namespace {

    // ==================================================================
    // PORT de BrutalSilnetAim.cs (genzkids no fame), con la infraestructura
    // de escritura fisica de este proyecto:
    //  - El hilo principal (SilentAimUpdate, 1x por frame) selecciona el
    //    target mas cercano al centro de pantalla y traduce la direccion de
    //    escritura a fisica (la cache SOLO la toca el hilo principal).
    //  - El hilo de 1ms escribe la direccion con HookWrite directo a fisica.
    // Logica identica al C#: delta (Head + 0.1) - StartPosition SIN
    // normalizar, escrito en RayDir (sAim4) del arma (sAim2).
    // ==================================================================
    std::atomic<bool> g_running{ false };
    std::atomic<bool> g_valid{ false };
    std::atomic<uint32_t> g_weaponPhys{ 0 }; // fisica de (weaponData + sAim4)
    std::atomic<float> g_dirX{ 0.0f };
    std::atomic<float> g_dirY{ 0.0f };
    std::atomic<float> g_dirZ{ 0.0f };
    std::thread g_thread;

} // namespace

namespace Aim {

    // ====================================================================
    // HILO DE ESCRITURA: escribe el delta en RayDir (weaponData + sAim4)
    // con PGMPhysSimpleWriteGCPhys directo (HookWrite), SIN pasar por la
    // cache. Rafaga de 60 escrituras cada ~20us con espera QPC: cubre el
    // instante exacto del disparo igual que el loop del C#.
    // ====================================================================
    static void Worker() {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        const LONGLONG step = freq.QuadPart / 50000; // 20 us de cadencia

        while (g_running) {
            try {
                if (g_valid && g_weaponPhys != 0) {
                    Vector3 dir(g_dirX, g_dirY, g_dirZ);
                    for (int i = 0; i < 60; i++)
                        Mem.HookWrite(Mem.pVMAddr, g_weaponPhys, (void*)&dir, sizeof(Vector3));

                    LARGE_INTEGER now;
                    QueryPerformanceCounter(&now);
                    LONGLONG target = now.QuadPart + step;
                    do {
                        QueryPerformanceCounter(&now);
                        _mm_pause();
                    } while (now.QuadPart < target && g_running);
                }
                else {
                    // Sin target activo: dormir para no consumir CPU
                    std::this_thread::sleep_for(std::chrono::microseconds(200));
                }
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    void SilentAimStart() {
        if (g_running) return;
        g_running = true;
        g_thread = std::thread(Worker);
    }

    void SilentAimStop() {
        if (!g_running) return;
        g_running = false;
        if (g_thread.joinable())
            g_thread.join();
    }

    // ====================================================================
    // UNA VEZ POR FRAME (Data::Work). Logica identica al BrutalSilnetAim.cs:
    // 1) target = entidad viva/visible mas cercana al centro de pantalla
    // 2) SIN gate de IsFiring: la direccion se publica y se escribe de forma
    //    continua mientras haya target en el FOV, asi la PRIMERA bala ya
    //    sale apuntada (el juego solo consume RayDir en el instante del tiro)
    //    adjustedTauko = Head + (0, +0.1, 0)
    //    startPos = weaponData + sAim3 (StartPosition del canon)
    //    aimPosition = adjustedTauko - startPos   <- delta SIN normalizar
    //    Write Vector3 en weaponData + sAim4 (RayDir)
    // ====================================================================
    void SilentAimUpdate() {
        // Switch del panel
        if (!g_Globals.Silent.Enabled) {
            g_valid = false;
            return;
        }

        // Precondiciones base (como el C#: Width/Height y matrix)
        if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0 || !g_Globals.EspConfig.Matrix) {
            g_valid = false;
            return;
        }
        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
        if (localPlayer == 0) {
            g_valid = false;
            return;
        }

        // ---- 1. SELECCION DE TARGET (mejoras del backup): DENTRO del FOV,
        // con margen de pantalla, filtros de bots/enemigos y RETENCION de
        // target. El backup de la version estable probaba que sin retencion
        // el spray salta entre enemigos y las balas se van a donde quieren:
        // se conserva la entidad que ya se dispara hasta que aparezca otra
        // claramente mejor (puntaje a menos de la mitad).
        Player* target = nullptr;
        float bestCombined = FLT_MAX;
        static uint32_t s_lastTargetAddr = 0;

        const float maxFov = (float)g_Globals.AimBot.DistanceAim; // px desde el centro
        const float refDistance = 300.0f; // distancia de referencia para normalizar
        const Vector2 screenCenter(
            (float)g_Globals.EspConfig.Width / 2.0f,
            (float)g_Globals.EspConfig.Height / 2.0f);

        for (auto& pair : g_Globals.EspConfig.Entities) {
            Player* entity = &pair.second;

            // filtros del C# + backup: IsKnown / IsDead / IgnoreKnocked / solo enemigos.
            // OJO: NO se filtran bots (como el C# original) - los bots tambien
            // se apuntan. Si alguien activa IgnoreBots del aimbot aqui no aplica.
            if (!entity->IsKnown || entity->IsDead) continue;
            if (g_Globals.AimBot.IgnoreKnocked && entity->IsKnocked) continue;
            if (g_Globals.AimBot.OnlyEnemies && entity->IsTeam == Player::Bool3::True) continue;
            if (g_Globals.Silent.OnlyVisible && !entity->IsVisible) continue;
            if (entity->Head == Vector3::Zero()) continue;

            Vector2 head2D = W2S::WorldToScreen(
                g_Globals.EspConfig.ViewMatrix, entity->Head,
                g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

            // fuera de pantalla o detras de la camara (W2S devuelve -9999)
            if (head2D.X < -5000.0f || head2D.Y < -5000.0f) continue;

            // margen de seguridad: debe estar dentro de la pantalla
            if (head2D.X < 5.0f || head2D.Y < 5.0f ||
                head2D.X > g_Globals.EspConfig.Width - 5.0f || head2D.Y > g_Globals.EspConfig.Height - 5.0f)
                continue;

            const float crosshairDist = Vector2::Distance(screenCenter, head2D);

            // FUERA DEL FOV: se ignora (aunque sea el mas cercano al centro)
            if (crosshairDist > maxFov) continue;

            // Distancia fisica al jugador local (cercania = 30% del puntaje)
            const float dist3D = Vector3::Distance(g_Globals.EspConfig.MainCamera, entity->Head);

            // Puntaje combinado: centro del FOV (70%) + cercania (30%)
            float fovNorm = maxFov > 0.0f ? crosshairDist / maxFov : 1.0f;
            float distNorm = dist3D / refDistance;
            if (distNorm > 1.0f) distNorm = 1.0f;
            float combined = fovNorm * 0.70f + distNorm * 0.30f;

            // RETENCION: el target actual se mantiene (mitad de puntaje)
            // hasta que aparezca otro claramente mejor
            if (entity->Address == s_lastTargetAddr) combined *= 0.5f;

            if (combined < bestCombined) {
                bestCombined = combined;
                target = entity;
            }
        }

        if (target == nullptr) {
            s_lastTargetAddr = 0;
            g_valid = false;
            return;
        }
        s_lastTargetAddr = target->Address;

        // ---- 2. SIN gate de isShooting: la direccion se escribe SIEMPRE
        // que haya un target valido en el FOV (el juego solo consume RayDir
        // en el instante del disparo). El gate por frame hacia que la 1a
        // bala saliera con la direccion vieja (el flag del juego recien se
        // activa al disparar): habia que soltar 2-3 balas para que entrara.

        // ---- 3. weaponData (sAim2 = 0x978) ----
        uint32_t weaponData = 0;
        if (!Mem.Read<uint32_t>(localPlayer + Offsets::sAim2, weaponData) || weaponData == 0) {
            g_valid = false;
            return;
        }

        // ---- 4. Delta bruto: (Head + 0.1) - StartPosition ----
        // Sane-check (del backup): nunca escribir un vector NaN/Inf o el
        // origen zero del juego -> era una fuente de balas a la deriva.
        if (!std::isfinite(target->Head.X) || !std::isfinite(target->Head.Y) || !std::isfinite(target->Head.Z)) {
            g_valid = false;
            return;
        }

        Vector3 adjustedTauko = target->Head;
        adjustedTauko.Y += 0.1f;

        Vector3 startPos;
        if (!Mem.Read<Vector3>(weaponData + Offsets::sAim3, startPos)) {
            g_valid = false;
            return;
        }
        if (!std::isfinite(startPos.X) || !std::isfinite(startPos.Y) || !std::isfinite(startPos.Z)) {
            g_valid = false;
            return;
        }

        Vector3 aimPosition = adjustedTauko - startPos;
        if (!std::isfinite(aimPosition.X) || !std::isfinite(aimPosition.Y) || !std::isfinite(aimPosition.Z)) {
            g_valid = false;
            return;
        }

        // ---- 5. Traducir a fisica AQUI (cache solo en hilo principal) ----
        uintptr_t physWeapon = 0;
        if (!Mem.Convert(weaponData + Offsets::sAim4, physWeapon)) {
            g_valid = false;
            return;
        }

        // Publicar para el hilo de escritura
        g_weaponPhys = (uint32_t)physWeapon;
        g_dirX = aimPosition.X;
        g_dirY = aimPosition.Y;
        g_dirZ = aimPosition.Z;
        g_valid = true;
    }

} // namespace Aim