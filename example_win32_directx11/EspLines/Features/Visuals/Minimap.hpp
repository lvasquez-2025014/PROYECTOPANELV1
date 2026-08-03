#pragma once
#include <imgui.h>
#include <cmath>
#include <vector>
#include <string>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#pragma once
#include <imgui.h> // Essencial para o ImVec2
#include <cmath>
#include <vector>
#include <src/Globals.hpp>
class Minimap {
private:
    float minimapSize = 180.0f;
    int detectionRange = 250;
    ImVec2 minimapCenter;

    float GetCameraYaw();
    void DrawCompass(ImDrawList* drawList, float yaw);
    void DrawPlayerIndicator(ImDrawList* drawList, float cosYaw, float sinYaw);
    void DrawEntities(ImDrawList* drawList, float cosYaw, float sinYaw);

public:
    Minimap();
    void Draw();
};
