#include <EspLines\Aimbot\RageAimbot.hpp>
#include <EspLines\Math\WordToScreen.hpp>
#include <EspLines\Math\Vector\Vector2.hpp>
#include <EspLines\Offsets.hpp>
#include <cmath>

namespace Aim {

bool RageAimbot::FindTarget(Player& outTarget, float& outCrosshairDist) {
    if (!g_Globals.EspConfig.Matrix) return false;

    float cx = g_Globals.EspConfig.Width * 0.5f;
    float cy = g_Globals.EspConfig.Height * 0.5f;
    float fovRadius = (float)g_Globals.AimBot.DistanceAim;
    float bestDist = fovRadius;
    bool found = false;

    for (auto& pair : g_Globals.EspConfig.Entities) {
        Player& entity = pair.second;
        if (entity.IsDead || entity.Address == 0) continue;
        if (g_Globals.AimBot.IgnoreKnocked && entity.IsKnocked) continue;
        if (g_Globals.AimBot.IgnoreBots && entity.IsBot) continue;

        float dist3D = Vector3::Distance(g_Globals.EspConfig.MainCamera, entity.Head);
        if (dist3D > 100000.0f) continue;

        Vector3 hitBoxPos = entity.Head;
        if (hitBoxPos == Vector3::Zero()) continue;

        Vector2 screen = W2S::WorldToScreen(g_Globals.EspConfig.ViewMatrix, hitBoxPos, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        if (screen.X < -5000 || screen.Y < -5000) continue;

        float dx = screen.X - cx;
        float dy = screen.Y - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            outTarget = entity;
            outCrosshairDist = dist;
            found = true;
        }
    }
    return found;
}

void RageAimbot::Aimbot() {
    if (!g_Globals.AimBot.RageAim) return;
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;
    if (!g_Globals.EspConfig.Matrix) return;

    Player target;
    float crosshairDist;
    if (!FindTarget(target, crosshairDist)) return;
    if (target.Address == 0) return;

    uint32_t headCollider = 0;
    if (!Mem.Read(target.Address + Offsets::HeadCollider, headCollider) || headCollider == 0) return;

    for (int i = 0; i < 10; i++)
        Mem.Write<uint32_t>(target.Address + Offsets::LockedAimingCollider, headCollider);
}

} // namespace Aim
