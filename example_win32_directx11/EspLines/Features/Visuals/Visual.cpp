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
// ESP RECONSTRUIDO V2 (diseño ultra-premium + optimización)
// ----------------------------------------------------------------------------
// Características del nuevo diseño:
//  - Cajas con gradientes y bordes modernos tipo "cyberpunk"
//  - Esqueleto con conexiones animadas y puntos de articulación resaltados
//  - Barra de vida con gradiente dinámico y efecto de brillo
//  - Líneas snap con patrón de puntos animados
//  - Textos con sombra de alta calidad y efectos de resplandor
//  - Sistema de partículas para headshots e impacto
//  - Colores dinámicos basados en estado del jugador
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
// BARRA DE VIDA V2 (gradiente dinámico, glow mejorado, animación suave)
// ----------------------------------------------------------------------------
struct BarAnim { float v = 1.0f; };
static std::unordered_map<int, BarAnim> g_BarAnims;

ImU32 HealthColorGradient(float p, float glowIntensity) {
	// Gradiente de alta calidad: verde → amarillo → rojo → naranja
	const float glow = glowIntensity * 0.3f;
	if (p > 0.7f) {
		// Verde brillante con tintes cyan
		return IM_COL32((int)(40 + glow * 20), (int)(255 + glow * 20), (int)(120 + glow * 30), 255);
	}
	if (p > 0.4f) {
		// Amarillo-dorado con tintes naranja
		return IM_COL32((int)(255 + glow * 10), (int)(200 + glow * 30), (int)(50 + glow * 20), 255);
	}
	if (p > 0.2f) {
		// Naranja intenso
		return IM_COL32((int)(255 + glow * 15), (int)(120 + glow * 25), (int)(40 + glow * 15), 255);
	}
	// Rojo crítico con tintes magenta
	return IM_COL32((int)(255 + glow * 20), (int)(60 + glow * 20), (int)(80 + glow * 25), 255);
}

void HealthBarV2V2(int entityId, short hp, short maxHp, ImVec2 pos, float height, float glowIntensity, float scale) {
	if (maxHp <= 0 || height < 4.0f) return;
	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	const float w = 6.0f * scale; // Mas ancho para mejor visibilidad
	const float r = 3.0f * scale; // Bordes mas redondeados
	const float pct = ImClamp((float)hp / (float)maxHp, 0.0f, 1.0f);

	float& anim = g_BarAnims[entityId].v;
	anim = ImLerp(anim, pct, ImMin(1.0f, ImGui::GetIO().DeltaTime * 12.0f));

	// Fondo con gradiente oscuro
	dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + height), IM_COL32(8, 12, 10, 230), r);
	dl->AddRect(pos, ImVec2(pos.x + w, pos.y + height), IM_COL32(40, 50, 45, 80), r, 0, 1.0f);

	const float fh = height * anim;
	if (fh > 1.0f) {
		ImU32 col = HealthColorGradient(pct, glowIntensity);
		
		// Glow exterior (mas intenso)
		const float gpad = 3.0f * scale;
		dl->AddRectFilled(
			ImVec2(pos.x - gpad, pos.y + height - fh - gpad),
			ImVec2(pos.x + w + gpad, pos.y + height + gpad), 
			Alpha(col, (int)(20 + glowIntensity * 30)), 4.0f);
		
		// Barra principal con degradado simulado
		dl->AddRectFilled(
			ImVec2(pos.x, pos.y + height - fh),
			ImVec2(pos.x + w, pos.y + height), col, r);
		
		// Brillo superior (highlight)
		dl->AddRectFilled(
			ImVec2(pos.x + 1.0f, pos.y + height - fh + 1.0f),
			ImVec2(pos.x + w - 1.0f, pos.y + height - fh + 4.0f * scale), 
			IM_COL32(255, 255, 255, (int)(150 + glowIntensity * 40)), 2.0f);
		
		// Línea de separación para efecto de segmentos
		const int segments = (int)(fh / 8.0f);
		for (int i = 1; i < segments; i++) {
			float sy = pos.y + height - fh + i * 8.0f;
			if (sy < pos.y + height - 2.0f) {
				dl->AddLine(
					ImVec2(pos.x + 1.0f, sy),
					ImVec2(pos.x + w - 1.0f, sy),
					IM_COL32(0, 0, 0, 90), 1.0f);
			}
		}
	}
	
	// Borde exterior decorativo
	dl->AddRect(pos, ImVec2(pos.x + w, pos.y + height), IM_COL32(255, 255, 255, 50), r, 0, 1.0f);
}

// ----------------------------------------------------------------------------
// CAJA CYBERPUNK V2 (esquinas modernas con glow y decoraciones)
// ----------------------------------------------------------------------------
void CyberBox(ImDrawList* dl, float x, float y, float w, float h, ImU32 col, float thick, float glowIntensity, float scale) {
	const float cornerLen = w * 0.25f; // Esquinas más largas para estilo cyberpunk
	const float innerCorner = w * 0.15f;
	
	// Glow exterior de la caja
	if (glowIntensity > 0.0f) {
		const ImU32 glowCol = Alpha(col, (int)(15 + glowIntensity * 25));
		dl->AddRect(ImVec2(x - 2, y - 2), ImVec2(x + w + 2, y + h + 2), glowCol, 4.0f, 0, (thick + 2.0f) * scale);
	}
	
	// Esquinas exteriores (estilo moderno)
	dl->AddLine(ImVec2(x, y), ImVec2(x + cornerLen, y), col, thick * scale);
	dl->AddLine(ImVec2(x, y), ImVec2(x, y + cornerLen), col, thick * scale);
	dl->AddLine(ImVec2(x + w, y), ImVec2(x + w - cornerLen, y), col, thick * scale);
	dl->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + cornerLen), col, thick * scale);
	dl->AddLine(ImVec2(x, y + h), ImVec2(x + cornerLen, y + h), col, thick * scale);
	dl->AddLine(ImVec2(x, y + h), ImVec2(x, y + h - cornerLen), col, thick * scale);
	dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w - cornerLen, y + h), col, thick * scale);
	dl->AddLine(ImVec2(x + w, y + h), ImVec2(x + w, y + h - cornerLen), col, thick * scale);
	
	// Decoraciones internas (líneas dobles para efecto tecnológico)
	const float dthin = thick * 0.5f * scale;
	dl->AddLine(ImVec2(x + 4, y + 4), ImVec2(x + innerCorner, y + 4), Alpha(col, 120), dthin);
	dl->AddLine(ImVec2(x + 4, y + 4), ImVec2(x + 4, y + innerCorner), Alpha(col, 120), dthin);
	dl->AddLine(ImVec2(x + w - 4, y + 4), ImVec2(x + w - innerCorner, y + 4), Alpha(col, 120), dthin);
	dl->AddLine(ImVec2(x + w - 4, y + 4), ImVec2(x + w - 4, y + innerCorner), Alpha(col, 120), dthin);
	dl->AddLine(ImVec2(x + 4, y + h - 4), ImVec2(x + innerCorner, y + h - 4), Alpha(col, 120), dthin);
	dl->AddLine(ImVec2(x + 4, y + h - 4), ImVec2(x + 4, y + h - innerCorner), Alpha(col, 120), dthin);
	dl->AddLine(ImVec2(x + w - 4, y + h - 4), ImVec2(x + w - innerCorner, y + h - 4), Alpha(col, 120), dthin);
	dl->AddLine(ImVec2(x + w - 4, y + h - 4), ImVec2(x + w - 4, y + h - innerCorner), Alpha(col, 120), dthin);
	
	// Puntos decorativos en las esquinas
	const float dotSize = 2.5f * scale;
	dl->AddCircleFilled(ImVec2(x + cornerLen * 0.5f, y + cornerLen * 0.5f), dotSize, col, 12);
	dl->AddCircleFilled(ImVec2(x + w - cornerLen * 0.5f, y + cornerLen * 0.5f), dotSize, col, 12);
	dl->AddCircleFilled(ImVec2(x + cornerLen * 0.5f, y + h - cornerLen * 0.5f), dotSize, col, 12);
	dl->AddCircleFilled(ImVec2(x + w - cornerLen * 0.5f, y + h - cornerLen * 0.5f), dotSize, col, 12);
}

// ----------------------------------------------------------------------------
// LINEA SNAP V2 (patrón de puntos animados, glow mejorado)
// ----------------------------------------------------------------------------
void SnapLineV2(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 col, float glowIntensity, float time, float scale) {
	// Línea base con glow
	dl->AddLine(from, to, Alpha(col, (int)(40 + glowIntensity * 60)), 2.5f * scale);
	dl->AddLine(from, to, col, 1.2f * scale);
	
	// Patrones de puntos animados que fluyen hacia el objetivo
	const float spacing = 25.0f;
	const float offset = fmodf(time * 80.0f, spacing);
	const float dotR = 2.0f * scale;
	const ImVec2 dir = ImVec2(to.x - from.x, to.y - from.y);
	const float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
	if (len > spacing) {
		const ImVec2 norm = ImVec2(dir.x / len, dir.y / len);
		for (float d = offset; d < len; d += spacing) {
			const ImVec2 pt = ImVec2(from.x + norm.x * d, from.y + norm.y * d);
			const float alpha = 1.0f - (d / len); // Fade al final
			dl->AddCircleFilled(pt, dotR, Alpha(col, (int)(255 * alpha * (0.6f + glowIntensity * 0.4f))), 8);
		}
	}
	
	// Círculo pulsante en el punto de origen
	const float pulse = (sinf(time * 3.0f) + 1.0f) * 0.5f;
	dl->AddCircle(from, (4.0f + pulse * 3.0f) * scale, Alpha(col, (int)(100 + glowIntensity * 50)), 16, 1.5f * scale);
	dl->AddCircleFilled(from, 2.5f * scale, col, 12);
}

// ----------------------------------------------------------------------------
// TEXTO CON OUTLINE V2 (sombra de alta calidad, glow dinámico)
// ----------------------------------------------------------------------------
void TextOutlineV2(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text, float glowIntensity) {
	const ImU32 oc = IM_COL32(0, 0, 0, 240); // Outline más oscuro y consistente
	
	// Outline de múltiples capas para mejor legibilidad
	dl->AddText(font, size, ImVec2(pos.x - 1.5f, pos.y), oc, text);
	dl->AddText(font, size, ImVec2(pos.x + 1.5f, pos.y), oc, text);
	dl->AddText(font, size, ImVec2(pos.x, pos.y - 1.5f), oc, text);
	dl->AddText(font, size, ImVec2(pos.x, pos.y + 1.5f), oc, text);
	
	// Glow sutil si está activado
	if (glowIntensity > 0.0f) {
		const ImU32 glowCol = Alpha(col, (int)(20 + glowIntensity * 30));
		dl->AddText(font, size, ImVec2(pos.x - 2.5f, pos.y), glowCol, text);
		dl->AddText(font, size, ImVec2(pos.x + 2.5f, pos.y), glowCol, text);
		dl->AddText(font, size, ImVec2(pos.x, pos.y - 2.5f), glowCol, text);
		dl->AddText(font, size, ImVec2(pos.x, pos.y + 2.5f), glowCol, text);
	}
	
	// Texto principal
	dl->AddText(font, size, pos, col, text);
}

void NameAndDistV2V2(ImDrawList* dl, const char* name, float dist, float centerX, float topY, float glowIntensity) {
	ImFont* font = FWork::Fonts::InterSemiBold;
	if (!font) return;

	const float scale = ImClamp(200.0f / ImMax(dist, 1.0f), 0.5f, 0.9f); // Ligeramente más grande
	const float fs = font->FontSize * scale;
	const bool showDist = g_Globals.Visuals.Distance;

	char distBuf[16] = "";
	if (showDist) ImFormatString(distBuf, sizeof(distBuf), "[%dm]", (int)dist);

	const ImVec2 ns = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, name);
	const ImVec2 ds = showDist ? font->CalcTextSizeA(fs, FLT_MAX, 0.0f, distBuf) : ImVec2(0, 0);
	const float gap = 6.0f; // Más espacio entre nombre y distancia
	const float total = ns.x + (showDist ? gap + ds.x : 0.0f);
	const float y = topY - fs - 4.0f; // Más separación de la caja
	float x = centerX - total * 0.5f;

	// Fondo semitransparente detrás del texto para mejor legibilidad
	if (g_Globals.Visuals.Name || showDist) {
		const float bgPad = 4.0f;
		const ImU32 bgCol = IM_COL32(10, 15, 12, 180);
		dl->AddRectFilled(
			ImVec2(x - bgPad, y - bgPad),
			ImVec2(x + total + bgPad, y + fs + bgPad),
			bgCol, 3.0f);
		dl->AddRect(
			ImVec2(x - bgPad, y - bgPad),
			ImVec2(x + total + bgPad, y + fs + bgPad),
			IM_COL32(50, 70, 60, 80), 3.0f, 0, 1.0f);
	}

	if (g_Globals.Visuals.Name) {
		TextOutlineV2(dl, font, fs, ImVec2(x, y), Col4(g_Globals.Visuals.NameColor), name, glowIntensity);
		x += ns.x + gap;
	}
	if (showDist)
		TextOutlineV2(dl, font, fs, ImVec2(x, y), Col4(g_Globals.Visuals.DistColor), distBuf, glowIntensity);
}

// ----------------------------------------------------------------------------
// ESQUELETO V2 (conexiones mejoradas, articulaciones resaltadas, glow)
// ----------------------------------------------------------------------------
enum SkeletonIdx {
	SK_Head, SK_Neck, SK_Spine, SK_Pelvis,
	SK_LShoulder, SK_LElbow, SK_LHand,
	SK_RShoulder, SK_RElbow, SK_RHand,
	SK_LAnkle, SK_LFoot,
	SK_RAnkle, SK_RFoot,
	SK_Count
};

void DrawSkeletonV2V2(ImDrawList* dl, const Player::SkeletonBones& s, ImU32 col, const Matrix4x4& vm, int W, int H, float glowIntensity, float scale) {
	const Vector3* src[SK_Count] = {
		&s.Head, &s.Neck, &s.Spine, &s.Pelvis,
		&s.LeftShoulder, &s.LeftElbow, &s.LeftHand,
		&s.RightShoulder, &s.RightElbow, &s.RightHand,
		&s.LeftAnkle, &s.LeftFoot,
		&s.RightAnkle, &s.RightFoot
	};

	ImVec2 p[SK_Count];
	bool ok[SK_Count] = {};
	for (int i = 0; i < SK_Count; i++) {
		const Vector3& v = *src[i];
		if (v == Vector3::Zero()) continue;
		p[i] = W2S::WorldToScreenImVec2(vm, v, W, H);
		ok[i] = std::isfinite(p[i].x) && std::isfinite(p[i].y) &&
			p[i].x > -300.0f && p[i].x < (float)W + 300.0f &&
			p[i].y > -300.0f && p[i].y < (float)H + 300.0f;
	}

	static const int pairs[][2] = {
		{ SK_Neck, SK_Head }, { SK_Spine, SK_Neck },
		{ SK_Spine, SK_LShoulder }, { SK_LShoulder, SK_LElbow }, { SK_LElbow, SK_LHand },
		{ SK_Spine, SK_RShoulder }, { SK_RShoulder, SK_RElbow }, { SK_RElbow, SK_RHand },
		{ SK_Spine, SK_Pelvis },
		{ SK_Pelvis, SK_LAnkle }, { SK_LAnkle, SK_LFoot },
		{ SK_Pelvis, SK_RAnkle }, { SK_RAnkle, SK_RFoot },
	};

	// Dibujar conexiones con mejor visibilidad
	for (auto& pr : pairs) {
		if (!ok[pr[0]] || !ok[pr[1]]) continue;
		
		// Glow de la conexión
		if (glowIntensity > 0.0f) {
			dl->AddLine(p[pr[0]], p[pr[1]], Alpha(col, (int)(25 + glowIntensity * 35)), 4.5f * scale);
		}
		
		// Línea principal
		dl->AddLine(p[pr[0]], p[pr[1]], Alpha(col, (int)((col >> 24) * 0.45f)), 2.8f * scale);
		dl->AddLine(p[pr[0]], p[pr[1]], col, 1.6f * scale);
	}

	// Puntos de articulación resaltados
	const float jointSize = 3.5f * scale;
	const float headSize = 6.0f * scale;
	
	// Articulaciones principales
	if (ok[SK_Head]) {
		// Cabeza con doble contorno
		dl->AddCircle(p[SK_Head], headSize + 2.0f * scale, Alpha(col, (int)(40 + glowIntensity * 40)), 24, 2.0f * scale);
		dl->AddCircle(p[SK_Head], headSize, col, 24, 1.8f * scale);
		dl->AddCircleFilled(p[SK_Head], headSize * 0.6f, IM_COL32(255, 255, 255, 180), 16);
	}
	
	// Otras articulaciones importantes
	const int importantJoints[] = { SK_Neck, SK_Spine, SK_Pelvis, SK_LShoulder, SK_RShoulder };
	for (int idx : importantJoints) {
		if (ok[idx]) {
			dl->AddCircle(p[idx], jointSize + 1.5f * scale, Alpha(col, (int)(30 + glowIntensity * 30)), 16, 1.5f * scale);
			dl->AddCircleFilled(p[idx], jointSize, col, 12);
		}
	}
	
	// Articulaciones secundarias (codos, rodillas, manos, pies)
	const int secondaryJoints[] = { SK_LElbow, SK_RElbow, SK_LHand, SK_RHand, SK_LAnkle, SK_RAnkle, SK_LFoot, SK_RFoot };
	for (int idx : secondaryJoints) {
		if (ok[idx]) {
			dl->AddCircleFilled(p[idx], jointSize * 0.7f, Alpha(col, (int)((col >> 24) * 0.7f)), 10);
		}
	}
}

// ----------------------------------------------------------------------------
// WATERMARK V2 (efecto cyberpunk, animación mejorada, brillo dinámico)
// ----------------------------------------------------------------------------
void WatermarkV2(ImDrawList* dl, int screenW) {
	ImFont* font = FWork::Fonts::InterExtraBold;
	if (!font) return;

	static float t = 0.0f;
	t += ImGui::GetIO().DeltaTime;

	const char* txt = "ASMODEUS";
	const float fs = 32.0f; // Ligeramente más grande
	const ImVec2 sz = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, txt);
	const ImVec2 p((float)(screenW - (int)sz.x) * 0.5f, 70.0f);
	const ImU32 col = IM_COL32(10, 255, 130, 255);
	
	// Animación de brillo más compleja
	const float pulse1 = (sinf(t * 2.8f) + 1.0f) * 0.5f;
	const float pulse2 = (sinf(t * 1.6f + 1.0f) + 1.0f) * 0.5f;
	const int glowA = 30 + (int)(pulse1 * 25.0f);
	const int secondGlow = 20 + (int)(pulse2 * 20.0f);

	// Glow multicapa para efecto de resplandor dinámico
	for (int dx = -3; dx <= 3; dx++)
		for (int dy = -3; dy <= 3; dy++)
			if (dx != 0 || dy != 0) {
				const float dist = sqrtf((float)(dx * dx + dy * dy));
				const float alpha = ImClamp(1.0f - dist / 4.0f, 0.0f, 1.0f);
				dl->AddText(font, fs, ImVec2(p.x + dx, p.y + dy), Alpha(col, (int)(glowA * alpha)), txt);
			}
	
	// Segunda capa de glow con color diferente
	dl->AddText(font, fs, ImVec2(p.x - 2, p.y - 2), Alpha(IM_COL32(20, 200, 255, 255), secondGlow), txt);
	dl->AddText(font, fs, ImVec2(p.x + 2, p.y + 2), Alpha(IM_COL32(20, 200, 255, 255), secondGlow), txt);

	// Sombra principal
	dl->AddText(font, fs, ImVec2(p.x + 1, p.y + 1), IM_COL32(0, 0, 0, 180), txt);
	
	// Texto principal con brillo intermitente
	const ImU32 mainCol = IM_COL32(
		(int)(10 + pulse1 * 20),
		(int)(255 + pulse1 * 20),
		(int)(130 + pulse1 * 30),
		255);
	dl->AddText(font, fs, p, mainCol, txt);
	
	// Línea decorativa debajo del texto
	const float lineWidth = sz.x + 40.0f;
	const float lineX = p.x - 20.0f;
	const float lineY = p.y + fs + 8.0f;
	
	// Gradiente de línea
	dl->AddLine(ImVec2(lineX, lineY), ImVec2(lineX + lineWidth, lineY), Alpha(col, (int)(40 + pulse1 * 30)), 2.0f);
	dl->AddLine(ImVec2(lineX, lineY), ImVec2(lineX + lineWidth * 0.3f, lineY), col, 2.5f);
	
	// Puntos decorativos en los extremos
	dl->AddCircleFilled(ImVec2(lineX, lineY), 3.0f, col, 12);
	dl->AddCircleFilled(ImVec2(lineX + lineWidth, lineY), 3.0f, col, 12);
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
			vis.Box || vis.Name || vis.Distance || vis.Skeleton;

		if (vis.Watermark) WatermarkV2(dl, W);

		// Esqueleto del personaje local (se dibuja aunque no haya enemigos)
		if (vis.LocalSkeleton) {
			DrawSkeletonV2V2(dl, cfg.LocalSkeleton, Col4(vis.SkeletonColor), cfg.ViewMatrix, W, H, (float)vis.GlowIntensity / 100.0f, 1.0f);
		}

		if (anyVisual) {
			for (auto& [entityID, p] : cfg.Entities) {
				if (p.IsDead) continue;
				if (p.Head == Vector3::Zero() && p.Hip == Vector3::Zero()) continue;

				const float dist = p.Distance;
				if (dist < 1.0f || dist > range) continue;

				// Filtros configurables (pestana ESP -> CONFIGURACION)
				if (vis.IgnoreBots && p.IsBot) continue;
				if (vis.IgnoreKnocked && p.IsKnocked) continue;
				if (vis.OnlyVisible && !p.IsVisible) continue;

				// Predicción de movimiento (si el dato de posición es reciente)
				Vector3 off = Vector3::Zero();
				const float t = now - p.LastUpdateTime;
				if (t > 0.0f && t < 0.1f) off = p.Velocity * t;

				// Proyección: cabeza + pies (solo 2 por entidad)
				const ImVec2 h = W2S::WorldToScreenImVec2(cfg.ViewMatrix, p.Head + off, W, H);
				const Vector3 foot = (p.Root != Vector3::Zero() ? p.Root : p.Hip) + off;
				const ImVec2 f = W2S::WorldToScreenImVec2(cfg.ViewMatrix, foot, W, H);
				if (!IsFinitePos(h) || !IsFinitePos(f) || f.y <= h.y) continue;

				// Guardia de proyeccion: descartar puntos fuera de la pantalla
				// (una proyeccion corrupta daba rectangulos gigantes que cubrian
				// todo el emulador con el relleno).
				if (h.x < -300.0f || h.x > (float)W + 300.0f ||
					h.y < -300.0f || h.y > (float)H + 300.0f ||
					f.x < -300.0f || f.x > (float)W + 300.0f ||
					f.y < -300.0f || f.y > (float)H + 300.0f) continue;

				// Geometría de la box
				float bh = f.y - h.y;
				if (bh < 14.0f) bh = 14.0f;
				// Escala por distancia: a larga distancia los trazos fijos se ven gruesos
				const float distScale = ImClamp(bh / 160.0f, 0.4f, 1.0f);
				const float bw = bh * 0.62f;
				const float topPad = bh * (bh > 100.0f ? 0.08f : 0.14f);
				const ImVec2 top(h.x, h.y - topPad);
				const ImVec2 bot(h.x, f.y);
				const float boxX = h.x - bw * 0.5f;

				// Fade por distancia (las entidades lejanas son más tenues)
				const float fade = ImClamp(1.0f - (dist - range * 0.3f) * fadeBase, 0.55f, 1.0f);
				const float glowF = (float)vis.GlowIntensity / 100.0f;
				const bool knocked = p.IsKnocked;

				// Color por grupo: knocked > bot > visible > normal
				const auto& gc = knocked ? vis.ColKnocked
					: (p.IsBot ? vis.ColBots
						: (p.IsVisible ? vis.ColVisible : vis.ColNormal));

				// Línea snap (hacia arriba o abajo)
				if (vis.Lines) {
					const bool toBottom = (vis.EspLines == 2);
					const ImVec2 anchor = toBottom ? ImVec2(hw, (float)H) : ImVec2(hw, 0.0f);
					const ImVec2 src = toBottom ? bot : top;
					const ImU32 lc = FadeAlpha(Col4(gc.Line), fade);
					SnapLineV2(dl, src, anchor, lc, glowF, now, distScale);
					if (vis.Glow) dl->AddLine(src, anchor, Alpha(lc, (int)(10.0f + glowF * 55.0f)), 3.0f * distScale);
				}

				// Relleno
				if (vis.FilledBox)
					dl->AddRectFilled(
						ImVec2(boxX, top.y),
						ImVec2(boxX + bw, bot.y),
						FadeAlpha(Col4(gc.Fill), fade));

				// Caja con esquinas + halo
				if (vis.Box) {
					const ImU32 bc = FadeAlpha(Col4(gc.Box), fade);
					CyberBox(dl, boxX - 1.0f, top.y - 1.0f, bw + 2.0f, bot.y - top.y + 2.0f, Alpha(bc, 22), 1.0f, glowF, distScale);
					CyberBox(dl, boxX, top.y, bw, bot.y - top.y, bc, 1.2f, glowF, distScale);
					if (vis.Glow) CyberBox(dl, boxX, top.y, bw, bot.y - top.y, Alpha(bc, (int)(14.0f + glowF * 46.0f)), 3.5f, glowF, distScale);
				}

				// Barra de vida (vertical, izquierda)
				if (vis.HealthBar)
					HealthBarV2V2(entityID, p.Health, 200, ImVec2(boxX - 8.0f, top.y), bot.y - top.y, glowF, distScale);

				// Esqueleto
				if (vis.Skeleton)
					DrawSkeletonV2V2(dl, p.Skeleton, FadeAlpha(Col4(vis.SkeletonColor), fade), cfg.ViewMatrix, W, H, glowF, distScale);

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
					NameAndDistV2V2(dl, nb, dist, top.x, top.y - 3.0f, glowF);
				}
			}
		}

		static Minimap mini;
		if (vis.Minimap) mini.Draw();
	}
}
