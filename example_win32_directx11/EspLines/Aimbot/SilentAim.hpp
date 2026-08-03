#pragma once

namespace Aim {
    // Calculo del target y la direccion: hilo principal (Data::Work).
    void SilentAimUpdate();

    // Hilo de escritura a ~1ms (patron TeleKill: std::atomic, sin mutex).
    void SilentAimStart();
    void SilentAimStop();
}
