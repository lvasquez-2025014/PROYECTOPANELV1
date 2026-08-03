#pragma once
#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>

namespace Aim {
    class RageAimbot {
    public:
        static void Aimbot();
    private:
        static bool FindTarget(Player& outTarget, float& outCrosshairDist);
    };
}
