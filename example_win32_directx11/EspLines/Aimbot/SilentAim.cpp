#include "SilentAim.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#define NOMINMAX
#include <Windows.h>
#include <cmath>
#include <limits>
#include <atomic>
#include <thread>
#include <chrono>
#include <intrin.h>

namespace {

    // ==================================================================
    // Compartidos entre el hilo principal (calcula) y el hilo de 1ms
    // (escribe). SOLO std::atomic: el std::mutex crashea este emulador
    // (verificado por bisect con git).
    // El hilo secundario NO toca la cache de memoria, NO toca la lista
    // de entidades y NO usa std::mutex: solo escribe con la direccion
    // fisica que el hilo principal ya tradujo.
    // ==================================================================
    std::atomic<bool> g_running{ false };
    std::atomic<bool> g_valid{ false };
    std::atomic<uint32_t> g_weaponPhys{ 0 }; // fisica de (aimInstance + RayDir)
    std::atomic<uint32_t> g_altPhys{ 0 };    // fisica de (alternativa + RayDir)
    std::atomic<float> g_dirX{ 0.0f };
    std::atomic<float> g_dirY{ 0.0f };
    std::atomic<float> g_dirZ{ 0.0f };
    std::thread g_thread;

    // Valida que un vector tenga componentes finitas (nunca NaN/Inf)
    bool IsFiniteVector(const Vector3& v) {
        return std::isfinite(v.X) && std::isfinite(v.Y) && std::isfinite(v.Z);
    }

} // namespace

namespace Aim {

    // ====================================================================
    // HILO PRINCIPAL (Data::Work, una vez por frame):
    // selecciona el target y calcula la direccion. Traduce las direcciones
    // a fisicas aqui (la cache solo la toca este hilo) y las publica en
    // atomicos para que el hilo de 1ms solo tenga que escribir.
    // ====================================================================
    void SilentAimUpdate() {
        // Switch del panel
        if (!g_Globals.Silent.Enabled) {
            g_valid = false;
            return;
        }

        // Click izquierdo pulsado (como la version estable)
        if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
            g_valid = false;
            return;
        }

        // Seguridad base
        if (!g_Globals.EspConfig.Matrix) {
            g_valid = false;
            return;
        }
        if (g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0) {
            g_valid = false;
            return;
        }
        uint32_t localPlayer = g_Globals.EspConfig.LocalPlayer;
        if (localPlayer == 0) {
            g_valid = false;
            return;
        }

        // ==================================================================
        // SELECCION DE TARGET: filtros configurados + dentro del FOV.
        // Best target = el que mejor combina estar al centro del FOV
        // (70%) y estar cerca del jugador local (30%).
        // ==================================================================
        Player* bestTarget = nullptr;
        float bestCombined = FLT_MAX;
        Vector2 screenCenter((float)g_Globals.EspConfig.Width / 2.0f, (float)g_Globals.EspConfig.Height / 2.0f);
        const float maxFov = (float)g_Globals.AimBot.DistanceAim;
        const float refDistance = 300.0f; // distancia de referencia para normalizar

        for (auto& pair : g_Globals.EspConfig.Entities) {
            Player* entity = &pair.second;

            if (entity->IsDead || (g_Globals.AimBot.IgnoreKnocked && entity->IsKnocked)) continue;
            if (g_Globals.AimBot.IgnoreBots && entity->IsBot) continue;
            if (g_Globals.AimBot.OnlyEnemies && entity->IsTeam == Player::Bool3::True) continue;
            if (entity->Head == Vector3::Zero()) continue;

            // Proyeccion en pantalla
            ImVec2 target2D = W2S::WorldToScreenImVec2(
                g_Globals.EspConfig.ViewMatrix, entity->Head,
                g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

            // Debe estar en pantalla (con margen de seguridad)
            if (target2D.x < 5 || target2D.y < 5 ||
                target2D.x > g_Globals.EspConfig.Width - 5 || target2D.y > g_Globals.EspConfig.Height - 5)
                continue;

            float crosshairDist = Vector2::Distance(screenCenter, Vector2(target2D.x, target2D.y));

            // Debe estar dentro del FOV
            if (crosshairDist > maxFov) continue;

            // Distancia fisica al jugador local
            float dist3D = Vector3::Distance(g_Globals.EspConfig.MainCamera, entity->Head);

            // Puntaje combinado: centro del FOV (70%) + cercania (30%)
            float fovNorm = maxFov > 0.0f ? crosshairDist / maxFov : 1.0f;
            float distNorm = dist3D / refDistance;
            if (distNorm > 1.0f) distNorm = 1.0f;
            float combined = fovNorm * 0.70f + distNorm * 0.30f;

            if (combined < bestCombined) {
                bestCombined = combined;
                bestTarget = entity;
            }
        }

        if (bestTarget == nullptr) {
            g_valid = false;
            return;
        }

        // Hueso apuntado (Head / Neck / Hip), igual que MemoryAim
        Vector3 aimPosition;
        switch (g_Globals.AimBot.TargetBone) {
        case Config::Bone::Neck:
            aimPosition = bestTarget->Neck != Vector3::Zero() ? bestTarget->Neck : bestTarget->Head;
            break;
        case Config::Bone::Hip:
            aimPosition = bestTarget->Hip != Vector3::Zero() ? bestTarget->Hip : bestTarget->Head;
            break;
        case Config::Bone::Head:
        default:
            aimPosition = bestTarget->Head;
            break;
        }
        if (aimPosition == Vector3::Zero()) {
            g_valid = false;
            return;
        }

        // Instancia principal: LastAimingInfoFromWeapon (0x978)
        uint32_t aimInstance = 0;
        if (!Mem.Read<uint32_t>(localPlayer + Offsets::LastAimingInfoFromWeapon, aimInstance) || aimInstance == 0) {
            g_valid = false;
            return;
        }

        // Posicion de salida del proyectil StartPosition (0x38)
        Vector3 bulletPos;
        if (!Mem.Read<Vector3>(aimInstance + Offsets::StartPosition, bulletPos) || !IsFiniteVector(bulletPos)) {
            g_valid = false;
            return;
        }

        Vector3 direction = aimPosition - bulletPos;
        if (!IsFiniteVector(direction)) {
            g_valid = false;
            return;
        }

        // Instancia alternativa: IsFiring (0x540) como puntero (opcional)
        uint32_t aimAlternative = 0;
        Mem.Read<uint32_t>(localPlayer + Offsets::IsFiring, aimAlternative);

        // Traducir a fisicas AQUI: la cache solo la usa el hilo principal
        uintptr_t physWeapon = 0, physAlt = 0;
        if (!Mem.Convert(aimInstance + Offsets::RayDir, physWeapon)) {
            g_valid = false;
            return;
        }
        if (aimAlternative != 0) {
            Mem.Convert(aimAlternative + Offsets::RayDir, physAlt);
        }

        // Publicar para el hilo de 1ms
        g_weaponPhys = (uint32_t)physWeapon;
        g_altPhys = (uint32_t)physAlt;
        g_dirX = direction.X;
        g_dirY = direction.Y;
        g_dirZ = direction.Z;
        g_valid = true;
    }

    // ====================================================================
    // HILO DE ESCRITURA (cadencia 0.001ms = 1us): escribe la direccion
    // compartida en RayDir de ambas instancias con PGMPhysSimpleWriteGCPhys
    // directo (HookWrite), SIN pasar por la cache ni estructuras compartidas.
    // Bucle de espera por spin (sin sleep): cada ~1us se reescribe RayDir,
    // de modo que en el instante del disparo el valor siempre es el nuestro.
    // ====================================================================
    static void Worker() {
        timeBeginPeriod(1); // resolucion del timer del sistema a 1 ms
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        const LONGLONG step = freq.QuadPart / 1000000; // 0.001 ms en ticks

        while (g_running) {
            try {
                if (g_valid && g_weaponPhys != 0) {
                    Vector3 dir(g_dirX, g_dirY, g_dirZ);
                    for (int i = 0; i < 60; i++)
                        Mem.HookWrite(Mem.pVMAddr, g_weaponPhys, (void*)&dir, sizeof(Vector3));
                    if (g_altPhys != 0) {
                        for (int i = 0; i < 60; i++)
                            Mem.HookWrite(Mem.pVMAddr, g_altPhys, (void*)&dir, sizeof(Vector3));
                    }
                }

                // Espera de ~0.001 ms (spin, sin sleep)
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                LONGLONG target = now.QuadPart + step;
                do {
                    QueryPerformanceCounter(&now);
                    _mm_pause();
                } while (now.QuadPart < target && g_running);
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        timeEndPeriod(1);
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

} // namespace Aim
