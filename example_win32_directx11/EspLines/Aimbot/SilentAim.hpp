#pragma once

// SILENT AIM BRUTAL (port del BrutalSilentAim.cs):
// Sobrescribe la direccion del ray (RayDir) del arma con el delta
// (Head + 0.1) - StartPosition sin normalizar, cada vez que el flag
// real de disparo del juego (IsFiring) esta activo. La camara nunca se
// mueve. No lead, no drop, sin fisica de bala: puro brute force.
namespace Aim {
    void SilentAimStart();         // arranca el hilo de escritura (lazy)
    void SilentAimStop();          // detiene el hilo (idempotente)
    void SilentAimUpdate();        // se llama 1 vez por frame (Data::Work)
}