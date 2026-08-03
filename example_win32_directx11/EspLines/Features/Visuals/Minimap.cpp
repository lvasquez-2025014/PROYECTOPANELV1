#include "Minimap.hpp"
#include <src/Globals.hpp>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// MINIMAP REDISEÑADO (tema verde premium, coherente con la GUI)
// ----------------------------------------------------------------------------
// - Posición recalculada cada frame (soporta cambios de resolución).
// - Rango ligado al slider "Alcance del ESP" de la GUI.
// - Estilo verde oscuro con glow, igual que el menú.
// ============================================================================

namespace {

ImU32 Alpha(ImU32 col, int a) {
	return (col & 0x00FFFFFF) | ((ImU32)ImClamp(a, 0, 255) << 24);
}

void TextOutline(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text) {
	const ImU32 oc = IM_COL32(0, 0, 0, 200);
	dl->AddText(ImVec2(pos.x - 1, pos.y), oc, text);
	dl->AddText(ImVec2(pos.x + 1, pos.y), oc, text);
	dl->AddText(ImVec2(pos.x, pos.y - 1), oc, text);
	dl->AddText(ImVec2(pos.x, pos.y + 1), oc, text);
	dl->AddText(pos, col, text);
}

} // namespace

Minimap::Minimap() {
	// La posición se recalcula en Draw() (soporta cambios de resolución)
	minimapCenter = ImVec2(140.0f, 500.0f);
}

float Minimap::GetCameraYaw() {
	return atan2f(g_Globals.EspConfig.ViewMatrix.m02, g_Globals.EspConfig.ViewMatrix.m22);
}

void Minimap::Draw() {
	if (!g_Globals.Visuals.Minimap) return;

	const int H = g_Globals.EspConfig.Height;
	minimapSize = 170.0f;
	minimapCenter = ImVec2(140.0f, (float)H - 140.0f);
	detectionRange = ImMax(100, g_Globals.Visuals.DistanceEsp);

	const float cameraYaw = -GetCameraYaw();
	const float cosYaw = cosf(cameraYaw);
	const float sinYaw = sinf(cameraYaw);

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	const ImU32 base = IM_COL32(10, 255, 130, 255);
	const float R = minimapSize * 0.5f;

	// Fondo
	dl->AddCircleFilled(minimapCenter, R, IM_COL32(7, 15, 11, 225), 96);

	// Grid concéntrico
	for (float k = 0.25f; k <= 1.0f; k += 0.25f)
		dl->AddCircle(minimapCenter, R * k, Alpha(base, 40), 96, 1.0f);

	// Borde con glow
	dl->AddCircle(minimapCenter, R, Alpha(base, 60), 96, 1.0f);
	dl->AddCircle(minimapCenter, R + 2.0f, Alpha(base, 26), 96, 1.0f);

	// Cruz de mira
	dl->AddLine(ImVec2(minimapCenter.x - 8, minimapCenter.y), ImVec2(minimapCenter.x + 8, minimapCenter.y), Alpha(base, 90), 1.0f);
	dl->AddLine(ImVec2(minimapCenter.x, minimapCenter.y - 8), ImVec2(minimapCenter.x, minimapCenter.y + 8), Alpha(base, 90), 1.0f);

	DrawCompass(dl, cameraYaw);
	DrawPlayerIndicator(dl, cosYaw, sinYaw);
	DrawEntities(dl, cosYaw, sinYaw);
}

void Minimap::DrawCompass(ImDrawList* drawList, float yaw) {
	const ImU32 compassColor = IM_COL32(10, 255, 130, 210);
	const char* directions[] = { "N", "E", "S", "W" };

	for (int i = 0; i < 4; i++) {
		const float angle = yaw + i * (float)(M_PI / 2.0);
		ImVec2 dirPos(
			minimapCenter.x + cosf(angle) * (minimapSize * 0.5f - 12.0f),
			minimapCenter.y - sinf(angle) * (minimapSize * 0.5f - 12.0f));
		TextOutline(drawList, ImVec2(dirPos.x - 5, dirPos.y - 5), compassColor, directions[i]);
	}
}

void Minimap::DrawPlayerIndicator(ImDrawList* drawList, float cosYaw, float sinYaw) {
	ImVec2 triangle[3] = { { 0, -8 }, { -6, 6 }, { 6, 6 } };
	ImVec2 rotated[3];

	for (int i = 0; i < 3; i++) {
		rotated[i].x = minimapCenter.x + (triangle[i].x * cosYaw - triangle[i].y * sinYaw);
		rotated[i].y = minimapCenter.y + (triangle[i].x * sinYaw + triangle[i].y * cosYaw);
	}

	drawList->AddTriangleFilled(rotated[0], rotated[1], rotated[2], IM_COL32(10, 255, 130, 255));

	// Halo
	for (int i = 1; i <= 2; i++) {
		ImVec2 glow[3];
		for (int j = 0; j < 3; j++) {
			const float dx = rotated[j].x - minimapCenter.x;
			const float dy = rotated[j].y - minimapCenter.y;
			const float scale = 1.0f + (float)i * 0.25f;
			glow[j] = ImVec2(minimapCenter.x + dx * scale, minimapCenter.y + dy * scale);
		}
		drawList->AddTriangle(glow[0], glow[1], glow[2], Alpha(IM_COL32(10, 255, 130, 255), 255 / (i * 2)), 1.0f);
	}
}

void Minimap::DrawEntities(ImDrawList* drawList, float cosYaw, float sinYaw) {
	const Vector3 localPos = g_Globals.EspConfig.MainCamera;
	const float R = minimapSize * 0.5f;

	for (auto const& [id, entity] : g_Globals.EspConfig.Entities) {
		if (entity.IsDead) continue;

		const float dist = Vector3::Distance(localPos, entity.Head);
		if (dist > detectionRange) continue;

		const Vector3 rel = entity.Head - localPos;
		const float scale = minimapSize / (float)detectionRange;
		const float rotX = rel.X * cosYaw - rel.Z * sinYaw;
		const float rotY = rel.X * sinYaw + rel.Z * cosYaw;
		ImVec2 pos(minimapCenter.x + rotX * scale, minimapCenter.y - rotY * scale);

		const float dx = pos.x - minimapCenter.x;
		const float dy = pos.y - minimapCenter.y;
		if (dx * dx + dy * dy > R * R) continue;

		ImU32 col;
		if (entity.IsKnown) col = entity.IsKnocked ? IM_COL32(255, 210, 70, 255) : IM_COL32(255, 80, 80, 255);
		else col = IM_COL32(90, 160, 255, 255);

		drawList->AddCircleFilled(ImVec2(pos.x + 1, pos.y + 1), 4.0f, IM_COL32(0, 0, 0, 200), 20);
		drawList->AddCircleFilled(pos, 3.5f, col, 20);
		drawList->AddCircle(pos, 4.5f, Alpha(col, 90), 20, 0.6f);

		if (g_Globals.Visuals.Distance) {
			char buf[16];
			ImFormatString(buf, sizeof(buf), "%dm", (int)dist);
			TextOutline(drawList, ImVec2(pos.x + 8, pos.y - 8), IM_COL32(255, 255, 255, 210), buf);
		}
	}
}
