#include <src/Globals.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <Windows.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <src/Fonts/Fonts.hpp>
#include "Minimap.hpp"

// ============================================================================
// ESP RECONSTRUIDO (diseño premium + optimización)
// ----------------------------------------------------------------------------
// Cambios vs la versión anterior:
//  - Una sola llamada a GetBackgroundDrawList() por frame.
//  - W2S: solo 2 proyecciones por entidad (cabeza + pies).
//  - Filtro por rango (DistanceEsp) ANTES de proyectar: entidades lejanas
//    ni se procesan (ahorro de CPU).
//  - Watermark dibujado UNA vez por frame (antes se dibujaba por cada entidad).
//  - Nombre/distancia con outline, sin cajas sólidas ni alocaciones por frame.
//  - Barra de vida con animación suave, glow y color por porcentaje.
//  - Colores de "knocked" en rojo.
// ============================================================================

namespace {

// ----------------------------------------------------------------------------
// UTILIDADES DE COLOR
// ----------------------------------------------------------------------------
ImU32 Col4(const float c[4]) {
	return IM_COL32(
		(int)(c[0] * 255.0f),
		(int)(c[1] * 255.0f),
		(int)(c[2] * 255.0f),
		(int)(c[3] * 255.0f));
}

ImU32 Alpha(ImU32 col, int a) {
	return (col & 0x00FFFFFF) | ((ImU32)ImClamp(a, 0, 255) << 24);
}

ImU32 FadeAlpha(ImU32 col, float f) {
	return Alpha(col, (int)((float)(col >> 24) * ImClamp(f, 0.0f, 1.0f)));
}

bool IsFinitePos(const ImVec2& v) {
	return std::isfinite(v.x) && std::isfinite(v.y);
}

// ----------------------------------------------------------------------------
// BARRA DE VIDA (vertical, animada, con glow)
// ----------------------------------------------------------------------------
struct BarAnim { float v = 1.0f; };
static std::unordered_map<int, BarAnim> g_BarAnims;

ImU32 HealthColor(float p) {
	if (p > 0.6f) return IM_COL32(80, 255, 140, 255);
	if (p > 0.3f) return IM_COL32(255, 210, 70, 255);
	return IM_COL32(255, 80, 80, 255);
}

void HealthBar(int entityId, short hp, short maxHp, ImVec2 pos, float height) {
	if (maxHp <= 0 || height < 4.0f) return;
	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	const float w = 5.0f;
	const float r = 2.0f;
	const float pct = ImClamp((float)hp / (float)maxHp, 0.0f, 1.0f);

	float& anim = g_BarAnims[entityId].v;
	anim = ImLerp(anim, pct, ImMin(1.0f, ImGui::GetIO().DeltaTime * 14.0f));

	dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + height), IM_COL32(10, 16, 12, 210), r);

	const float fh = height * anim;
	if (fh > 1.0f) {
		ImU32 col = HealthColor(pct);
		dl->AddRectFilled(
			ImVec2(pos.x - 2.0f, pos.y + height - fh - 2.0f),
			ImVec2(pos.x + w + 2.0f, pos.y + height + 2.0f), Alpha(col, 26), 3.0f);
		dl->AddRectFilled(
			ImVec2(pos.x, pos.y + height - fh),
			ImVec2(pos.x + w, pos.y + height), col, r);
		dl->AddRectFilled(
			ImVec2(pos.x + 1.0f, pos.y + height - fh + 1.0f),
			ImVec2(pos.x + w - 1.0f, pos.y + height - fh + 3.0f), IM_COL32(255, 255, 255, 170));
	}
	dl->AddRect(pos, ImVec2(pos.x + w, pos.y + height), IM_COL32(255, 255, 255, 60), r, 0, 1.0f);
}

// ----------------------------------------------------------------------------
// CAJA CON ESQUINAS (estilo premium)
// ----------------------------------------------------------------------------
void CornerBox(ImDrawList* dl, float x, float y, float w, float h, ImU32 col, float thick) {
	const float l = w * 0.22f;
	dl->AddLine(ImVec2(x, y), ImVec2(x + l, y), col, thick);
	dl->AddLine(ImVec2(x, y), ImVec2(x, y + l), col, thick);
	dl->AddLine(ImVec2(x + w, y), ImVec2(x + w - l, y), col, thick);
	dl->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + l), col, thick);
	dl->AddLine(ImVec2(x, y + h), ImVec2(x + l, y + h), col, thick);
	dl->AddLine(ImVec2(x, y + h), ImVec2(x, y + h - l), col, thick);
	dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w - l, y + h), col, thick);
	dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - l), col, thick);
}

// ----------------------------------------------------------------------------
// LINEA SNAP con halo
// ----------------------------------------------------------------------------
void SnapLine(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 col) {
	dl->AddLine(from, to, col, 1.0f);
	dl->AddLine(from, to, FadeAlpha(col, 0.35f), 1.7f);
}

// ----------------------------------------------------------------------------
// TEXTO CON OUTLINE (sin alocaciones)
// ----------------------------------------------------------------------------
void TextOutline(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text) {
	const ImU32 oc = IM_COL32(0, 0, 0, 220);
	dl->AddText(font, size, ImVec2(pos.x - 1, pos.y), oc, text);
	dl->AddText(font, size, ImVec2(pos.x + 1, pos.y), oc, text);
	dl->AddText(font, size, ImVec2(pos.x, pos.y - 1), oc, text);
	dl->AddText(font, size, ImVec2(pos.x, pos.y + 1), oc, text);
	dl->AddText(font, size, pos, col, text);
}

void NameAndDist(ImDrawList* dl, const char* name, float dist, float centerX, float topY) {
	ImFont* font = FWork::Fonts::InterSemiBold;
	if (!font) return;

	const float scale = ImClamp(200.0f / ImMax(dist, 1.0f), 0.45f, 0.85f);
	const float fs = font->FontSize * scale;
	const bool showDist = g_Globals.Visuals.Distance;

	char distBuf[16] = "";
	if (showDist) ImFormatString(distBuf, sizeof(distBuf), "[%dm]", (int)dist);

	const ImVec2 ns = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, name);
	const ImVec2 ds = showDist ? font->CalcTextSizeA(fs, FLT_MAX, 0.0f, distBuf) : ImVec2(0, 0);
	const float gap = 5.0f;
	const float total = ns.x + (showDist ? gap + ds.x : 0.0f);
	const float y = topY - fs - 2.0f;
	float x = centerX - total * 0.5f;

	if (g_Globals.Visuals.Name) {
		TextOutline(dl, font, fs, ImVec2(x, y), Col4(g_Globals.Visuals.NameColor), name);
		x += ns.x + gap;
	}
	if (showDist)
		TextOutline(dl, font, fs, ImVec2(x, y), Col4(g_Globals.Visuals.DistColor), distBuf);
}

// ----------------------------------------------------------------------------
// WATERMARK (una vez por frame)
// ----------------------------------------------------------------------------
void Watermark(ImDrawList* dl, int screenW) {
	ImFont* font = FWork::Fonts::InterExtraBold;
	if (!font) return;

	static float t = 0.0f;
	t += ImGui::GetIO().DeltaTime;

	const char* txt = "FREE";
	const float fs = 30.0f;
	const ImVec2 sz = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, txt);
	const ImVec2 p((float)(screenW - (int)sz.x) * 0.5f, 72.0f);
	const ImU32 col = IM_COL32(10, 255, 130, 255);
	const int glowA = 26 + (int)((sinf(t * 2.4f) + 1.0f) * 14.0f);

	for (int dx = -2; dx <= 2; dx++)
		for (int dy = -2; dy <= 2; dy++)
			if (dx != 0 || dy != 0)
				dl->AddText(font, fs, ImVec2(p.x + dx, p.y + dy), Alpha(col, glowA), txt);

	dl->AddText(font, fs, ImVec2(p.x + 1, p.y + 1), IM_COL32(0, 0, 0, 160), txt);
	dl->AddText(font, fs, p, col, txt);
}

} // namespace

// ============================================================================
// RENDER PRINCIPAL DEL ESP
// ============================================================================
namespace ESP {
	void Players() {
		auto& cfg = g_Globals.EspConfig;
		auto& vis = g_Globals.Visuals;

		if (!cfg.Matrix || !vis.Enable) return;

		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		const int W = cfg.Width, H = cfg.Height;
		if (W <= 0 || H <= 0) return;

		const float hw = W * 0.5f, hh = H * 0.5f;
		const float now = (float)GetTickCount64() / 1000.0f;
		const float range = (float)ImMax(vis.DistanceEsp, 50);
		const float fadeBase = 1.30f / range;

		const bool anyVisual = vis.Lines || vis.HealthBar || vis.FilledBox ||
			vis.Box || vis.Name || vis.Distance;

		if (vis.Watermark) Watermark(dl, W);

		if (anyVisual) {
			for (auto& [entityID, p] : cfg.Entities) {
				if (p.IsDead) continue;
				if (p.Head == Vector3::Zero() && p.Hip == Vector3::Zero()) continue;

				const float dist = p.Distance;
				if (dist < 1.0f || dist > range) continue;

				// Predicción de movimiento (si el dato de posición es reciente)
				Vector3 off = Vector3::Zero();
				const float t = now - p.LastUpdateTime;
				if (t > 0.0f && t < 0.1f) off = p.Velocity * t;

				// Proyección: cabeza + pies (solo 2 por entidad)
				const ImVec2 h = W2S::WorldToScreenImVec2(cfg.ViewMatrix, p.Head + off, W, H);
				const Vector3 foot = (p.Root != Vector3::Zero() ? p.Root : p.Hip) + off;
				const ImVec2 f = W2S::WorldToScreenImVec2(cfg.ViewMatrix, foot, W, H);
				if (!IsFinitePos(h) || !IsFinitePos(f) || f.y <= h.y) continue;

				// Geometría de la box
				float bh = f.y - h.y;
				if (bh < 14.0f) bh = 14.0f;
				const float bw = bh * 0.62f;
				const float topPad = bh * (bh > 100.0f ? 0.08f : 0.14f);
				const ImVec2 top(h.x, h.y - topPad);
				const ImVec2 bot(h.x, f.y);
				const float boxX = h.x - bw * 0.5f;

				// Fade por distancia (las entidades lejanas son más tenues)
				const float fade = ImClamp(1.0f - (dist - range * 0.3f) * fadeBase, 0.55f, 1.0f);
				const bool knocked = p.IsKnocked;
				const ImU32 red = IM_COL32(255, 70, 70, 255);

				// Línea snap (hacia arriba o abajo)
				if (vis.Lines) {
					const ImVec2 anchor = (vis.EspLines == 2) ? ImVec2(hw, (float)H) : ImVec2(hw, 0.0f);
					SnapLine(dl, top, anchor, FadeAlpha(knocked ? red : Col4(vis.LinesColor), fade));
				}

				// Relleno
				if (vis.FilledBox)
					dl->AddRectFilled(
						ImVec2(boxX, top.y),
						ImVec2(boxX + bw, bot.y),
						FadeAlpha(Col4(vis.Filledboxcolor), fade));

				// Caja con esquinas + halo
				if (vis.Box) {
					const ImU32 bc = FadeAlpha(knocked ? red : Col4(vis.BoxColor), fade);
					CornerBox(dl, boxX - 1.0f, top.y - 1.0f, bw + 2.0f, bot.y - top.y + 2.0f, Alpha(bc, 22), 1.0f);
					CornerBox(dl, boxX, top.y, bw, bot.y - top.y, bc, 1.2f);
				}

				// Barra de vida (vertical, izquierda)
				if (vis.HealthBar)
					HealthBar(entityID, p.Health, 200, ImVec2(boxX - 8.0f, top.y), bot.y - top.y);

				// Nombre + distancia
				if (vis.Name || vis.Distance) {
					char nb[64];
					int n = 0;
					for (char c : p.Name) {
						if (n >= 63) break;
						if (c >= 32) nb[n++] = c;
					}
					if (n == 0) { nb[0] = '?'; n = 1; }
					nb[n] = 0;
					NameAndDist(dl, nb, dist, h.x, top.y - 3.0f);
				}
			}
		}

		static Minimap mini;
		if (vis.Minimap) mini.Draw();
	}
}
