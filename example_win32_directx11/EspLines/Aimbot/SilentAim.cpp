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
        // Retencion de target: conserva la entidad que ya se esta disparando
        // para que TODAS las balas del spray vayan a la misma entidad en vez
        // de saltar entre las mas cercanas al crosshair. Se mantiene mientras
        // su puntaje no sea el doble de malo que el mejor candidato nuevo.
        static uint32_t s_lastTargetAddr = 0;
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

            // Bonus de continuidad: el target actual disparando se mantiene
            // hasta que aparezca otro claramente mejor (puntaje a menos de la mitad)
            if (entity->Address == s_lastTargetAddr) combined *= 0.5f;

            if (combined < bestCombined) {
                bestCombined = combined;
                bestTarget = entity;
            }
        }

        if (bestTarget == nullptr) {
            s_lastTargetAddr = 0;
            g_valid = false;
            return;
        }
        s_lastTargetAddr = bestTarget->Address;

        // Hueso apuntado (Head / Neck / Hip), selector propio del silent
        Vector3 aimPosition;
        switch (g_Globals.Silent.TargetBone) {
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

        // ==================================================================
        // ORIGEN DEL RAYO: al disparar a la cadera (hip-fire) el proyectil
        // sale del canon, que queda mas abajo que la camara; apuntar desde
        // la camara entonces desvia las balas al pecho. Cuando se apunta
        // (ADS) el canon coincide con la camara y ahi la camara funciona.
        // Se usa el StartPosition real del arma si es valido (vector finito,
        // distinto de cero y pegado al jugador) y camara como respaldo.
        // MEJORA: Rango dinamico basado en HipFireAccuracy para mejor precision
        // ==================================================================
        Vector3 rayOrigin = g_Globals.EspConfig.MainCamera;
        Vector3 startPos;
        if (Mem.Read<Vector3>(aimInstance + Offsets::StartPosition, startPos) && IsFiniteVector(startPos)) {
            float originGap = Vector3::Distance(startPos, g_Globals.EspConfig.MainCamera);
            // Rango dinamico: mayor precision = rango mas estricto
            // HipFireAccuracy 1.0 = rango estrecho (0.15f a 3.0f)
            // HipFireAccuracy 0.5 = rango amplio (0.05f a 5.0f)
            float minGap = 0.05f + (g_Globals.Silent.HipFireAccuracy * 0.1f);
            float maxGap = 5.0f - (g_Globals.Silent.HipFireAccuracy * 2.0f);
            if (originGap > minGap && originGap < maxGap) {
                rayOrigin = startPos;
            }
        }

Vector3 aimPos = aimPosition;

        // ==================================================================
        // PREDICCION DE BALA (lead + drop + herencia velocidad local):
        // 1) Lead: target se mueve mientras la bala vuela
        // 2) Drop: la bala cae por gravedad (compensamos apuntando mas arriba)
        // 3) Herencia: la bala hereda velocidad del jugador local
        // ==================================================================
        Vector3 toTarget = aimPos - rayOrigin;
        float distToTarget = sqrtf(toTarget.X * toTarget.X + toTarget.Y * toTarget.Y + toTarget.Z * toTarget.Z);
        float flightTime = g_Globals.Silent.BulletSpeed > 10.0f
            ? distToTarget / g_Globals.Silent.BulletSpeed
            : 0.0f;
        if (flightTime > 0.25f) flightTime = 0.25f;

        // Lead: predecir movimiento del target
        aimPos += bestTarget->Velocity * flightTime;

        // Drop: compensar caida de la bala (apuntar mas alto)
        // drop = 0.5 * g * t^2  -->  apuntamos drop metros mas arriba
        float drop = 0.0f;
        if (g_Globals.Silent.Gravity > 0.0f) {
            drop = 0.5f * g_Globals.Silent.Gravity * flightTime * flightTime;
            aimPos.Y += drop;
        }

        // Herencia de velocidad del jugador local (la bala sale con tu velocidad)
        Vector3 localVel = Vector3::Zero();
        auto itLocal = g_Globals.EspConfig.Entities.find(g_Globals.EspConfig.LocalPlayer);
        if (itLocal != g_Globals.EspConfig.Entities.end()) {
            localVel = itLocal->second.Velocity;
        }
        if (IsFiniteVector(localVel)) {
            aimPos += localVel * flightTime;
        }

        Vector3 direction = aimPos - rayOrigin;
        // Normalizar: el juego espera direccion unitaria
        float dirLen = sqrtf(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
        if (dirLen > 0.001f) {
            direction.X /= dirLen; direction.Y /= dirLen; direction.Z /= dirLen;
        }
        if (!IsFiniteVector(direction)) {
            g_valid = false;
            return;
        }

        // MEJORA: Suavizado de dirección basado en HipFireAccuracy
        // Reducimos pequeñas variaciones que causan imprecisión en hip-fire
        static Vector3 lastDirection = Vector3::Zero();
        static bool firstFrame = true;
        if (firstFrame) {
            lastDirection = direction;
            firstFrame = false;
        }
        else {
            // Interpolación basada en la precisión configurada
            float lerpFactor = g_Globals.Silent.HipFireAccuracy * 0.3f; // 0.15-0.30
            direction.X = lastDirection.X + (direction.X - lastDirection.X) * lerpFactor;
            direction.Y = lastDirection.Y + (direction.Y - lastDirection.Y) * lerpFactor;
            direction.Z = lastDirection.Z + (direction.Z - lastDirection.Z) * lerpFactor;
            
            // Renormalizar después de la interpolación
            dirLen = sqrtf(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
            if (dirLen > 0.001f) {
                direction.X /= dirLen; direction.Y /= dirLen; direction.Z /= dirLen;
            }
        }
        lastDirection = direction;

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
    // HILO DE ESCRITURA: escribe la direccion compartida en RayDir de
    // ambas instancias con PGMPhysSimpleWriteGCPhys directo (HookWrite),
    // SIN pasar por la cache ni estructuras compartidas.
    // Optimizado para no robar FPS:
    //  - Sin target (g_valid == false): duerme, NO quema CPU.
    //  - Con target: escrituras en rafaga cada ~20us con espera por QPC
    //    (sin sleep), suficiente cobertura para el instante del disparo.
    //  - Sin timeBeginPeriod: el loop usa QPC, no el timer del sistema.
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
                    if (g_altPhys != 0) {
                        for (int i = 0; i < 60; i++)
                            Mem.HookWrite(Mem.pVMAddr, g_altPhys, (void*)&dir, sizeof(Vector3));
                    }

                    // Espera de ~20 us (spin, sin sleep)
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

} // namespace Aim
