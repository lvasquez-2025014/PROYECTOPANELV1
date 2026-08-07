#include "ui.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <src/Overlay/Overlay.hpp>
#include <src/Globals.hpp>
#include <src/Fonts/Fonts.hpp>
#include <src/adb/adb_utils.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <vector>
#include <cmath>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// SIMBOLOS GLOBALES REQUERIDOS POR ext/ImGui MODIFICADO (imgui.cpp / imgui_widgets.cpp)
// ============================================================================
ImFont* FontAwesomeRegular = nullptr;
ImFont* FontAwesomeSolid = nullptr;
ImFont* FontAwesomeSolid14 = nullptr;
ImFont* FontAwesomeBrands = nullptr;
ImFont* InterBlack = nullptr;
ImFont* InterBold = nullptr;
ImFont* InterBold12 = nullptr;
ImFont* InterExtraBold = nullptr;
ImFont* InterExtraLight = nullptr;
ImFont* InterLight = nullptr;
ImFont* InterMedium = nullptr;
ImFont* InterRegular = nullptr;
ImFont* InterSemiBold = nullptr;
ImFont* InterThin = nullptr;

namespace FWork {

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

// ============================================================================
// PALETA DE COLORES (DISEÑO HESCO-XIT: rojo/naranja sobre fondo oscuro)
// ============================================================================

#define C_MAIN          IM_COL32(255, 0, 0, 255)      // Acento principal rojo
#define C_ACCENT        IM_COL32(255, 69, 0, 255)     // Acento naranja-rojo (checkbox/slider/tab)
#define C_WINDOW_BG     IM_COL32(11, 11, 14, 210)     // Fondo de la ventana
#define C_DARK          IM_COL32(28, 30, 34, 255)     // Fondo oscuro (page active)
#define C_SECOND        IM_COL32(23, 23, 31, 255)     // Fondo secundario (headers/sliders)
#define C_SECOND_HOVER  IM_COL32(30, 30, 38, 255)     // Second hover
#define C_BACKGROUND    IM_COL32(24, 24, 29, 240)     // Fondo de popups
#define C_STROKE        IM_COL32(48, 48, 58, 65)      // Bordes y lineas
#define C_SEP           IM_COL32(45, 45, 50, 255)     // Lineas entre secciones
#define C_CHILD_BG      IM_COL32(13, 13, 16, 210)     // Fondo de cards
#define C_CHILD_STROKE  IM_COL32(45, 45, 50, 255)     // Borde de cards
#define C_PAGE_ACTIVE   IM_COL32(36, 36, 46, 255)     // Segmento activo

#define C_TEXT_ACTIVE   IM_COL32(255, 255, 255, 255)
#define C_TEXT_HOVERED  IM_COL32(200, 200, 200, 255)
#define C_TEXT_DEFAULT  IM_COL32(111, 111, 111, 255)
#define C_DESC_ACTIVE   IM_COL32(200, 200, 200, 102)
#define C_DESC_HOVERED  IM_COL32(200, 200, 200, 63)
#define C_DESC_DEFAULT  IM_COL32(200, 200, 200, 40)

// ============================================================================
// ESTADO DE CONEXION ADB
// ============================================================================

enum class AdbState {
	Disconnected,
	Connecting,
	Connected,
	Failed
};

static std::atomic<AdbState> g_AdbState{ AdbState::Disconnected };

static ImU32 AdbStateColor()
{
	switch (g_AdbState) {
	case AdbState::Disconnected: return C_TEXT_DEFAULT;
	case AdbState::Connecting:   return IM_COL32(255, 200, 40, 255);
	case AdbState::Connected:    return C_ACCENT;
	case AdbState::Failed:       return IM_COL32(255, 80, 80, 255);
	}
	return C_TEXT_DEFAULT;
}

static void ConnectAdb()
{
	if (g_AdbState == AdbState::Connecting || g_AdbState == AdbState::Connected) return;
	g_AdbState = AdbState::Connecting;

	std::thread([]() {
		bool ok = false;
		try {
			ok = ADB::InitializeADB();
		} catch (const std::exception&) {
			ok = false;
		}
		g_AdbState = ok ? AdbState::Connected : AdbState::Failed;
	}).detach();
}

// ============================================================================
// UTILIDADES DE DIBUJO (estilo HESCO)
// ============================================================================

static ImU32 LerpColor(ImU32 a, ImU32 b, float t)
{
	t = ImClamp(t, 0.0f, 1.0f);
	ImVec4 A = ImGui::ColorConvertU32ToFloat4(a);
	ImVec4 B = ImGui::ColorConvertU32ToFloat4(b);
	return ImGui::ColorConvertFloat4ToU32(ImVec4(
		A.x + (B.x - A.x) * t,
		A.y + (B.y - A.y) * t,
		A.z + (B.z - A.z) * t,
		A.w + (B.w - A.w) * t));
}

static float GetAnimSpeed()
{
	return ImClamp(ImGui::GetIO().DeltaTime * 12.0f, 0.0f, 1.0f);
}

static void DrawPulseDot(ImDrawList* dl, const ImVec2& c, ImU32 color, float pulse)
{
	dl->AddCircleFilled(c, 3.6f, color);
	dl->AddCircle(c, 4.6f, (color & 0x00FFFFFF) | (40u << 24), 24, 1.0f);
	dl->AddCircle(c, 5.5f + pulse * 4.5f, (color & 0x00FFFFFF) | ((ImU32)(int)((1.0f - pulse) * 70.0f) << 24), 24, 1.0f);
}

// Titulo arcoiris por caracter (branding estilo HESCO)
static void DrawRainbowText(ImDrawList* dl, const ImVec2& pos, const char* text, float t)
{
	if (Fonts::InterExtraBold) ImGui::PushFont(Fonts::InterExtraBold);
	float x = pos.x;
	for (const char* c = text; *c; c++) {
		if (*c == ' ') {
			ImVec2 sz = ImGui::CalcTextSize(" ");
			x += sz.x;
			continue;
		}
		char ch[2] = { *c, 0 };
		ImVec2 sz = ImGui::CalcTextSize(ch);
		float hue = fmodf(t * 0.12f + (float)(c - text) * 0.055f, 1.0f);
		float r, g, b;
		ImGui::ColorConvertHSVtoRGB(hue, 0.85f, 1.0f, r, g, b);
		ImU32 col = IM_COL32((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f), 255);
		dl->AddText(ImVec2(x + 1, pos.y + 1), IM_COL32(0, 0, 0, 170), ch);
		dl->AddText(ImVec2(x, pos.y), col, ch);
		x += sz.x;
	}
	if (Fonts::InterExtraBold) ImGui::PopFont();
}

// ============================================================================
// PARTICULAS FLOTANTES DE FONDO (estilo HESCO: particle::RenderEffects)
// ============================================================================
struct Particle {
	ImVec2 pos;
	float speed;
	float size;
	float phase;
};

static std::vector<Particle> g_particles;

static void InitParticles()
{
	srand(1337);
	for (int i = 0; i < 36; i++) {
		Particle pt;
		pt.pos = ImVec2((float)(rand() % 2000), (float)(rand() % 2000));
		pt.speed = 0.15f + (float)(rand() % 100) / 100.0f * 0.5f;
		pt.size = 1.5f + (float)(rand() % 100) / 100.0f * 3.5f;
		pt.phase = (float)(rand() % 1000) / 1000.0f * 6.2831f;
		g_particles.push_back(pt);
	}
}

static void RenderParticles(ImDrawList* dl, ImVec2 display, float dt, float t)
{
	if (g_particles.empty()) InitParticles();
	for (auto& pt : g_particles) {
		pt.pos.y -= pt.speed * dt * 120.0f;
		pt.pos.x += sinf(t * 0.8f + pt.phase) * dt * 20.0f;
		if (pt.pos.y < -20.0f) pt.pos.y = display.y + 20.0f;
		if (pt.pos.x < -20.0f) pt.pos.x = display.x + 20.0f;
		if (pt.pos.x > display.x + 20.0f) pt.pos.x = -20.0f;
		float tw = 0.4f + 0.6f * (0.5f + 0.5f * sinf(t * 2.0f + pt.phase));
		ImU32 c = (C_ACCENT & 0x00FFFFFF) | ((ImU32)(int)(14.0f * tw) << 24);
		dl->AddCircleFilled(pt.pos, pt.size, c, 24);
		dl->AddCircle(pt.pos, pt.size * 2.2f, c, 24, 1.0f);
	}
}

// Boton premium (redondeado, borde de acento, glow)
static bool PremiumButton(ImVec2 pos, ImVec2 size, const char* label, ImU32 accent, bool enabled, float pulse)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiID id = window->GetID(label);
	ImRect bb(pos, pos + size);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id)) return false;

	bool hov = false, held = false;
	bool pressed = false;
	if (enabled) {
		pressed = ImGui::ButtonBehavior(bb, id, &hov, &held);
	}

	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImU32 bg = C_SECOND;
	if (hov) bg = C_SECOND_HOVER;
	if (held) bg = C_PAGE_ACTIVE;
	dl->AddRectFilled(pos, pos + size, bg, 4.0f);

	float ga = (enabled && hov) ? 1.0f : (enabled ? 0.45f + pulse * 0.35f : 0.20f);
	dl->AddRect(pos, pos + size, (accent & 0x00FFFFFF) | ((ImU32)(int)(ga * 110.0f) << 24), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
	dl->AddRect(pos + ImVec2(1, 1), pos + size - ImVec2(1, 1), C_STROKE, 3.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	ImVec2 ts = ImGui::CalcTextSize(label);
	ImU32 textCol = enabled ? C_TEXT_ACTIVE : C_TEXT_DEFAULT;
	dl->AddText(pos + ImVec2((size.x - ts.x) * 0.5f, (size.y - ts.y) * 0.5f - 1.0f), textCol, label);
	if (Fonts::InterBold12) ImGui::PopFont();

	return pressed;
}

// ============================================================================
// PESTAÑA VERTICAL DEL SIDEBAR (estilo HESCO custom::Tab)
// ============================================================================
static std::map<ImGuiID, float> g_tabAnims;

static bool HesTab(ImDrawList* dl, const char* label, const char* icon, bool active, float pulse)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiID id = window->GetID(label);
	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(ImGui::GetContentRegionAvail().x, 45));
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id)) return false;

	bool hov, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hov, &held);

	float& anim = g_tabAnims[id];
	anim += ((active ? 1.0f : 0.0f) - anim) * GetAnimSpeed();

	// Marco: gradiente vertical de acento (activo 0.3 / hover 0.15 / nada)
	ImU32 frameCol = LerpColor(IM_COL32(0, 0, 0, 0), (C_ACCENT & 0x00FFFFFF) | (77u << 24), anim);
	if (!active && hov) frameCol = (C_ACCENT & 0x00FFFFFF) | (38u << 24);
	dl->AddRectFilledMultiColor(bb.Min, bb.Max, frameCol, IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), frameCol, 4.0f);

	// Icono a la izquierda
	if (Fonts::FontAwesomeSolid) {
		ImGui::PushFont(Fonts::FontAwesomeSolid);
		ImVec2 is = ImGui::CalcTextSize(icon);
		float ia = active ? 1.0f : (hov ? 0.8f : 0.5f + anim * 0.5f);
		ImU32 ic = (C_ACCENT & 0x00FFFFFF) | ((ImU32)(int)(ia * 255.0f) << 24);
		dl->AddText(ImVec2(bb.Min.x + 10, bb.GetCenter().y - is.y * 0.5f), ic, icon);
		ImGui::PopFont();
	}

	// Nombre
	ImU32 tc = LerpColor(active ? C_TEXT_ACTIVE : C_TEXT_DEFAULT, C_TEXT_ACTIVE, anim);
	if (!active && hov) tc = C_TEXT_HOVERED;
	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	ImVec2 ls = ImGui::CalcTextSize(label);
	dl->AddText(ImVec2(bb.Min.x + 40, bb.GetCenter().y - ls.y * 0.5f), tc, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	return pressed;
}

// ============================================================================
// CARD COLAPSABLE (estilo HESCO custom::Child con cap)
// ============================================================================
static std::map<ImGuiID, bool> g_cardOpen;
static std::map<ImGuiID, float> g_cardAnim;

// contentH = suma exacta de las alturas que avanzaran las filas del contenido
static bool Card(ImDrawList* dl, const char* id, const char* title, const char* desc, const char* icon, float width, float contentH)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiID cid = window->GetID(id);
	bool& open = g_cardOpen[cid];
	float& anim = g_cardAnim[cid];
	anim += ((open ? 1.0f : 0.0f) - anim) * ImClamp(ImGui::GetIO().DeltaTime * 10.0f, 0.0f, 1.0f);

	ImVec2 p = ImGui::GetCursorScreenPos();
	const float h = 55.0f + contentH;

	// Fondo + borde de la card
	dl->AddRectFilled(p, p + ImVec2(width, h), C_CHILD_BG, 5.0f);

	// Cabecera (55px)
	ImRect hdr(p, p + ImVec2(width, 55));
	const bool hhov = ImGui::IsMouseHoveringRect(hdr.Min, hdr.Max);
	dl->AddRectFilled(hdr.Min, hdr.Max, hhov ? C_SECOND_HOVER : C_SECOND, 5.0f, ImDrawFlags_RoundCornersTop);

	// Icono de la card (fade segun estado)
	if (Fonts::FontAwesomeSolid) {
		ImGui::PushFont(Fonts::FontAwesomeSolid);
		ImVec2 is = ImGui::CalcTextSize(icon);
		ImU32 ic = (C_ACCENT & 0x00FFFFFF) | ((ImU32)(int)(90.0f + anim * 165.0f) << 24);
		dl->AddText(p + ImVec2(27.5f - is.x * 0.5f, 27.5f - is.y * 0.5f + 1.0f), ic, icon);
		ImGui::PopFont();
	}

	// Titulo + descripcion
	if (Fonts::InterBold) ImGui::PushFont(Fonts::InterBold);
	dl->AddText(p + ImVec2(55.5f, 7), C_TEXT_ACTIVE, title);
	if (Fonts::InterBold) ImGui::PopFont();
	if (desc && Fonts::InterRegular14) {
		ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(55.5f, 27), C_DESC_ACTIVE, desc);
		ImGui::PopFont();
	}

	// Pill colapsable (derecha de la cabecera)
	ImRect pill(ImVec2(hdr.Max.x - 50, hdr.GetCenter().y - 10), ImVec2(hdr.Max.x - 20, hdr.GetCenter().y + 10));
	dl->AddRectFilled(pill.Min, pill.Max, C_WINDOW_BG, 36.0f);
	dl->AddRectFilled(pill.Min, pill.Max, LerpColor(C_SECOND, (C_ACCENT & 0x00FFFFFF) | (200u << 24), anim), 36.0f);
	float kx = pill.Min.x + 10.0f + (pill.GetSize().x - 20.0f) * anim;
	ImVec2 kc(kx, pill.GetCenter().y);
	dl->AddCircleFilled(kc, 5.0f, LerpColor(IM_COL32(120, 120, 120, 255), IM_COL32(255, 255, 255, 255), anim), 36);
	dl->AddCircle(kc, 5.0f, C_ACCENT, 36, 1.0f);
	dl->AddRect(pill.Min, pill.Max, C_STROKE, 36.0f);

	// Interaccion: click en la cabecera alterna abierto/cerrado
	ImGui::PushID(id);
	ImGuiID hbId = ImGui::GetID("##hdr");
	ImGui::ItemSize(hdr);
	if (ImGui::ItemAdd(hdr, hbId)) {
		bool hhov2, hheld;
		if (ImGui::ButtonBehavior(hdr, hbId, &hhov2, &hheld, ImGuiButtonFlags_PressedOnClick)) {
			open = !open;
		}
	}
	ImGui::PopID();

	ImGui::Dummy(ImVec2(0, 55));
	return open;
}

// ============================================================================
// FILA CON CHECKBOX (estilo HESCO custom::Checkbox)
// ============================================================================
static std::map<ImGuiID, float> g_checkAnims;

static bool RowToggle(ImDrawList* dl, const char* switchId, const char* label, const char* sub, bool* v, ImU32 accent, float pulse, float width = 0.0f)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiID id = window->GetID(switchId);

	float& anim = g_checkAnims[id];
	anim += ((*v ? 1.0f : 0.0f) - anim) * ImClamp(ImGui::GetIO().DeltaTime * 24.0f, 0.0f, 1.0f);

	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
	ImVec2 p0 = p + ImVec2(2, 0), p1 = p + ImVec2(w - 2, 44);

	const bool bhov = ImGui::IsMouseHoveringRect(p0, p1);
	if (bhov) dl->AddRectFilled(p0, p1, IM_COL32(200, 200, 200, 8), 4.0f);

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 7), *v ? C_TEXT_ACTIVE : C_TEXT_DEFAULT, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();
	if (sub && Fonts::InterRegular14) {
		ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(12, 26), C_DESC_ACTIVE, sub);
		ImGui::PopFont();
	}

	// Caja 20x20 a la derecha
	ImRect box(ImVec2(p.x + w - 32, p.y + 12), ImVec2(p.x + w - 12, p.y + 32));
	dl->AddRectFilled(box.Min, box.Max, LerpColor(C_SECOND, (accent & 0x00FFFFFF) | (150u << 24), anim), 5.0f);
	dl->AddRect(box.Min, box.Max, LerpColor(C_STROKE, accent, anim), 5.0f, ImDrawFlags_RoundCornersAll, 1.0f);
	if (anim > 0.05f) {
		int ga = (int)(anim * (46.0f + pulse * 30.0f));
		dl->AddRect(box.Min - ImVec2(2, 2), box.Max + ImVec2(2, 2), (accent & 0x00FFFFFF) | ((ImU32)ga << 24), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f);
	}
	if (anim > 0.02f) {
		float cw = box.GetWidth() * anim;
		dl->PushClipRect(box.Min, box.Min + ImVec2(cw, box.GetHeight()), true);
		dl->PathLineTo(box.Min + ImVec2(5.5f, 10.5f));
		dl->PathLineTo(box.Min + ImVec2(8.5f, 13.5f));
		dl->PathLineTo(box.Min + ImVec2(14.5f, 6.5f));
		dl->PathStroke(IM_COL32(255, 255, 255, 255), 0, 2.0f);
		dl->PopClipRect();
	}

	ImGui::Dummy(ImVec2(0, 44));

	ImGui::PushID(switchId);
	ImGuiID bId = ImGui::GetID("##box");
	ImGui::ItemSize(box);
	bool pressed = false;
	if (ImGui::ItemAdd(box, bId)) {
		bool bh2, bh3;
		pressed = ImGui::ButtonBehavior(box, bId, &bh2, &bh3, ImGuiButtonFlags_PressedOnClick);
		if (pressed) *v = !*v;
	}
	ImGui::PopID();
	return pressed;
}

// ============================================================================
// TRACK DE SLIDER (estilo HESCO custom::SliderFloat)
// ============================================================================
template<typename T>
static bool SliderTrack(ImDrawList* dl, const char* sliderId, const ImVec2& pos, float width, T* v, T vmin, T vmax, ImU32 accent, float pulse)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiID id = window->GetID(sliderId);
	ImRect bb(pos, pos + ImVec2(width, 18));
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id)) return false;

	bool hov, held;
	ImGui::ButtonBehavior(bb, id, &hov, &held, ImGuiButtonFlags_PressedOnClick);

	bool changed = false;
	if (held) {
		float t = ImClamp((ImGui::GetIO().MousePos.x - bb.Min.x) / (bb.Max.x - bb.Min.x), 0.0f, 1.0f);
		*v = (T)(vmin + t * (vmax - vmin));
		changed = true;
	}

	float t = ImClamp((double)(*v - vmin) / (double)(vmax - vmin), 0.0, 1.0);
	const float trackY = bb.Min.y + 6.0f;
	const float trackH = 6.0f;
	ImVec2 tMin(bb.Min.x, trackY), tMax(bb.Max.x, trackY + trackH);

	dl->AddRectFilled(tMin, tMax, C_SECOND, 3.0f);
	if (t > 0.01f) {
		ImVec2 fMax(bb.Min.x + (bb.Max.x - bb.Min.x) * (float)t, tMax.y);
		ImU32 c2 = LerpColor(accent, C_MAIN, 0.35f);
		dl->AddRectFilledMultiColor(tMin, fMax, accent, c2, c2, accent, 3.0f);
		dl->AddRectFilled(tMin + ImVec2(0, -1.5f), fMax + ImVec2(0, 1.5f), (accent & 0x00FFFFFF) | (40u << 24), 3.0f);
	}
	dl->AddRect(tMin, tMax, C_STROKE, 3.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	ImVec2 kc(bb.Min.x + (bb.Max.x - bb.Min.x) * (float)t, bb.Min.y + 9.0f);
	dl->AddCircleFilled(kc, 5.5f, IM_COL32(255, 255, 255, 235), 24);
	dl->AddCircle(kc, 5.5f, accent, 24, 1.2f);
	if (hov || held) {
		dl->AddCircle(kc, 8.5f, (accent & 0x00FFFFFF) | ((ImU32)(int)(40.0f + pulse * 40.0f) << 24), 24, 1.0f);
	}

	return changed;
}

template<typename T>
static void SliderRow(ImDrawList* dl, const char* sliderId, const char* label, const char* fmt, T* v, T vmin, T vmax, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), C_TEXT_ACTIVE, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	char buf[32];
	ImFormatString(buf, sizeof(buf), fmt, *v);
	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	ImVec2 vs = ImGui::CalcTextSize(buf);
	dl->AddText(p + ImVec2(w - 12 - vs.x, 8), accent, buf);
	if (Fonts::InterBold12) ImGui::PopFont();

	SliderTrack(dl, sliderId, p + ImVec2(12, 30), w - 24, v, vmin, vmax, accent, pulse);

	ImGui::Dummy(ImVec2(0, 54));
}

// ============================================================================
// FILA DE TEXTO INFORMATIVO
// ============================================================================
static void InfoRow(ImDrawList* dl, const char* l1, const char* l2 = nullptr)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	if (Fonts::InterRegular14) ImGui::PushFont(Fonts::InterRegular14);
	dl->AddText(p + ImVec2(12, 2), C_DESC_ACTIVE, l1);
	if (l2) dl->AddText(p + ImVec2(12, 18), C_DESC_ACTIVE, l2);
	if (Fonts::InterRegular14) ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, 33));
}

// ============================================================================
// SELECTORES DE SEGMENTOS (estilo HESCO page: fondo oscuro, texto 200/255)
// ============================================================================
static void SegmentRow(ImDrawList* dl, const char* id, const char* label, int* v, const char* const* opts, int n, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), C_TEXT_ACTIVE, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	const float h = 30.0f;
	const float gap = 6.0f;
	const float sw = (w - 24 - gap * (n - 1)) / (float)n;
	const ImVec2 b0(p.x + 12, p.y + 28);

	for (int i = 0; i < n; i++) {
		ImRect r(b0 + ImVec2(i * (sw + gap), 0), b0 + ImVec2(i * (sw + gap) + sw, h));
		const bool active = (*v == i);
		const bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
		dl->AddRectFilled(r.Min, r.Max, active ? C_PAGE_ACTIVE : (hov ? C_SECOND_HOVER : C_SECOND), 4.0f);
		if (active) {
			dl->AddRect(r.Min, r.Max, (accent & 0x00FFFFFF) | (90u << 24), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		} else if (hov) {
			dl->AddRect(r.Min, r.Max, C_STROKE, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		}

		ImGui::PushID(i);
		ImGuiID tid = ImGui::GetID(id);
		ImGui::ItemSize(r);
		if (ImGui::ItemAdd(r, tid)) {
			bool thov, theld;
			if (ImGui::ButtonBehavior(r, tid, &thov, &theld, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoHoldingActiveId)) {
				*v = i;
			}
		}
		ImGui::PopID();

		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		ImVec2 ts = ImGui::CalcTextSize(opts[i]);
		dl->AddText(r.Min + ImVec2((r.GetWidth() - ts.x) * 0.5f, (r.GetHeight() - ts.y) * 0.5f), active ? C_TEXT_ACTIVE : (hov ? C_TEXT_HOVERED : C_TEXT_DEFAULT), opts[i]);
		if (Fonts::InterBold12) ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, h + 34));
}

// Posicion de la linea del ESP (TOP=1 / BOTTOM=2)
static void TypeSelector(ImDrawList* dl, const char* id, const char* label, int* v, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), C_TEXT_ACTIVE, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	const float h = 30.0f;
	const float sw = (w - 24 - 6) * 0.5f;
	const ImVec2 b0(p.x + 12, p.y + 28);
	const char* opts[2] = { "TOP", "BOTTOM" };

	for (int i = 0; i < 2; i++) {
		ImRect r(b0 + ImVec2(i * (sw + 6), 0), b0 + ImVec2(i * (sw + 6) + sw, h));
		const bool active = (*v == (i ? 2 : 1));
		const bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
		dl->AddRectFilled(r.Min, r.Max, active ? C_PAGE_ACTIVE : (hov ? C_SECOND_HOVER : C_SECOND), 4.0f);
		if (active) {
			dl->AddRect(r.Min, r.Max, (accent & 0x00FFFFFF) | (90u << 24), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		} else if (hov) {
			dl->AddRect(r.Min, r.Max, C_STROKE, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		}

		ImGui::PushID(i);
		ImGuiID tid = ImGui::GetID(id);
		ImGui::ItemSize(r);
		if (ImGui::ItemAdd(r, tid)) {
			bool thov, theld;
			if (ImGui::ButtonBehavior(r, tid, &thov, &theld, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoHoldingActiveId)) {
				*v = i ? 2 : 1;
			}
		}
		ImGui::PopID();

		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		ImVec2 ts = ImGui::CalcTextSize(opts[i]);
		dl->AddText(r.Min + ImVec2((r.GetWidth() - ts.x) * 0.5f, (r.GetHeight() - ts.y) * 0.5f), active ? C_TEXT_ACTIVE : (hov ? C_TEXT_HOVERED : C_TEXT_DEFAULT), opts[i]);
		if (Fonts::InterBold12) ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, h + 34));
}

// Selector de hueso (HEAD / NECK / HIP)
static void BoneSelector(ImDrawList* dl, const char* id, const char* label, Config::Bone* v, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), C_TEXT_ACTIVE, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	const float h = 30.0f;
	const float sw = (w - 24 - 12) / 3.0f;
	const ImVec2 b0(p.x + 12, p.y + 28);
	const char* opts[3] = { "HEAD", "NECK", "HIP" };

	for (int i = 0; i < 3; i++) {
		ImRect r(b0 + ImVec2(i * (sw + 6), 0), b0 + ImVec2(i * (sw + 6) + sw, h));
		const bool active = ((int)*v == i);
		const bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
		dl->AddRectFilled(r.Min, r.Max, active ? C_PAGE_ACTIVE : (hov ? C_SECOND_HOVER : C_SECOND), 4.0f);
		if (active) {
			dl->AddRect(r.Min, r.Max, (accent & 0x00FFFFFF) | (90u << 24), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		} else if (hov) {
			dl->AddRect(r.Min, r.Max, C_STROKE, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		}

		ImGui::PushID(i);
		ImGuiID tid = ImGui::GetID(id);
		ImGui::ItemSize(r);
		if (ImGui::ItemAdd(r, tid)) {
			bool thov, theld;
			if (ImGui::ButtonBehavior(r, tid, &thov, &theld, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoHoldingActiveId)) {
				*v = (Config::Bone)i;
			}
		}
		ImGui::PopID();

		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		ImVec2 ts = ImGui::CalcTextSize(opts[i]);
		dl->AddText(r.Min + ImVec2((r.GetWidth() - ts.x) * 0.5f, (r.GetHeight() - ts.y) * 0.5f), active ? C_TEXT_ACTIVE : (hov ? C_TEXT_HOVERED : C_TEXT_DEFAULT), opts[i]);
		if (Fonts::InterBold12) ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, h + 34));
}

// Selector de 3 opciones
static void TriSelector(ImDrawList* dl, const char* id, const char* label, int* v,
	const char* optA, const char* optB, const char* optC, ImU32 accent, float pulse, float width = 0.0f)
{
	const char* opts[3] = { optA, optB, optC };
	SegmentRow(dl, id, label, v, opts, 3, accent, pulse, width);
}

// Selector de 2 opciones
static void DualSelector(ImDrawList* dl, const char* id, const char* label, int* v,
	const char* optA, const char* optB, ImU32 accent, float pulse, float width = 0.0f)
{
	const char* opts[2] = { optA, optB };
	SegmentRow(dl, id, label, v, opts, 2, accent, pulse, width);
}

// ============================================================================
// FILA CON SELECTOR DE COLOR (swatch + popup color picker)
// ============================================================================
static ImU32 Col4(const float c[4])
{
	return IM_COL32(
		(int)(c[0] * 255.0f),
		(int)(c[1] * 255.0f),
		(int)(c[2] * 255.0f),
		(int)(c[3] * 255.0f));
}

static void RowColor(ImDrawList* dl, const char* id, const char* label, float col[4], ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
	ImVec2 p0 = p + ImVec2(2, 0), p1 = p + ImVec2(w - 2, 40);

	const bool hov = ImGui::IsMouseHoveringRect(p0, p1);
	if (hov) dl->AddRectFilled(p0, p1, IM_COL32(200, 200, 200, 8), 4.0f);

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 10), C_TEXT_ACTIVE, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	ImVec2 sw = p + ImVec2(w - 16 - 30, 9);
	dl->AddRectFilled(sw, sw + ImVec2(30, 22), Col4(col), 4.0f);
	dl->AddRect(sw, sw + ImVec2(30, 22), hov ? (accent & 0x00FFFFFF) | (70u << 24) : C_STROKE, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	ImGui::Dummy(ImVec2(0, 40));

	ImGui::PushID(id);
	if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(p0, p1)) {
		ImGui::OpenPopup("##cp");
	}
	if (ImGui::BeginPopup("##cp")) {
		ImGui::ColorPicker4("##colp", col,
			ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs);
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

// Grupo de colores (Linea / Caja / Relleno) para un tipo de jugador
static void GroupColors(ImDrawList* dl, const char* group, Globals::Visuals::EspGroupColors* gc, float pulse, float width)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	dl->AddText(p + ImVec2(12, 0), C_TEXT_HOVERED, group);
	if (Fonts::InterBold12) ImGui::PopFont();
	dl->AddRectFilled(p + ImVec2(12, 15), p + ImVec2(12 + ImGui::CalcTextSize(group).x + 2, 16), C_SEP, 1.0f);
	ImGui::Dummy(ImVec2(0, 22));

	ImGui::PushID(group);
	RowColor(dl, "##L", "Linea", gc->Line, C_ACCENT, pulse, width);
	RowColor(dl, "##B", "Caja", gc->Box, C_ACCENT, pulse, width);
	RowColor(dl, "##F", "Relleno", gc->Fill, C_ACCENT, pulse, width);
	ImGui::PopID();
}

// ============================================================================
// EVITA TECLAS DUPLICADAS: al asignar una tecla nueva a una funcion, se
// limpia de TODOS los otros binds para que una pulsacion no active dos
// funciones a la vez (causa de toggles "que no funcionan bien").
// ============================================================================
static void ClearKeyFromOtherBinds(int key, const int* keepField)
{
	if (key <= 0) return;
	if (keepField != &g_Globals.Exploits.TeleKillKey) if (g_Globals.Exploits.TeleKillKey == key) g_Globals.Exploits.TeleKillKey = 0;
	if (keepField != &g_Globals.Exploits.UnderPlayerKey) if (g_Globals.Exploits.UnderPlayerKey == key) g_Globals.Exploits.UnderPlayerKey = 0;
	if (keepField != &g_Globals.Exploits.FlyKey) if (g_Globals.Exploits.FlyKey == key) g_Globals.Exploits.FlyKey = 0;
	if (keepField != &g_Globals.Exploits.PullPlayerKey) if (g_Globals.Exploits.PullPlayerKey == key) g_Globals.Exploits.PullPlayerKey = 0;
	if (keepField != &g_Globals.Exploits.TeleportKey) if (g_Globals.Exploits.TeleportKey == key) g_Globals.Exploits.TeleportKey = 0;
	if (keepField != &g_Globals.Exploits.GhostLagKey) if (g_Globals.Exploits.GhostLagKey == key) g_Globals.Exploits.GhostLagKey = 0;
	if (keepField != &g_Globals.Exploits.FakeLagKey) if (g_Globals.Exploits.FakeLagKey == key) g_Globals.Exploits.FakeLagKey = 0;
	if (keepField != &g_Globals.Exploits.TeleportLagKey) if (g_Globals.Exploits.TeleportLagKey == key) g_Globals.Exploits.TeleportLagKey = 0;
	if (keepField != &g_Globals.Exploits.TurnEnemiesKey) if (g_Globals.Exploits.TurnEnemiesKey == key) g_Globals.Exploits.TurnEnemiesKey = 0;
	if (keepField != &g_Globals.Exploits.SpinBotKey) if (g_Globals.Exploits.SpinBotKey == key) g_Globals.Exploits.SpinBotKey = 0;
	if (keepField != &g_Globals.Exploits.TpWallKey) if (g_Globals.Exploits.TpWallKey == key) g_Globals.Exploits.TpWallKey = 0;
	if (keepField != &g_Globals.Exploits.FastSwitchKey) if (g_Globals.Exploits.FastSwitchKey == key) g_Globals.Exploits.FastSwitchKey = 0;
	if (keepField != &g_Globals.Exploits.NoRecoilKey) if (g_Globals.Exploits.NoRecoilKey == key) g_Globals.Exploits.NoRecoilKey = 0;
	if (keepField != &g_Globals.Exploits.NoReloadKey) if (g_Globals.Exploits.NoReloadKey == key) g_Globals.Exploits.NoReloadKey = 0;
	if (keepField != &g_Globals.Exploits.UnlimitedAmmoKey) if (g_Globals.Exploits.UnlimitedAmmoKey == key) g_Globals.Exploits.UnlimitedAmmoKey = 0;
	if (keepField != &g_Globals.Exploits.JumpHackKey) if (g_Globals.Exploits.JumpHackKey == key) g_Globals.Exploits.JumpHackKey = 0;
	if (keepField != &g_Globals.Exploits.VisionHackKey) if (g_Globals.Exploits.VisionHackKey == key) g_Globals.Exploits.VisionHackKey = 0;
	if (keepField != &g_Globals.Exploits.FastFallKey) if (g_Globals.Exploits.FastFallKey == key) g_Globals.Exploits.FastFallKey = 0;
	if (keepField != &g_Globals.AimBot.AimbotBind) if (g_Globals.AimBot.AimbotBind == key) g_Globals.AimBot.AimbotBind = 0;
	if (keepField != &g_Globals.General.MenuKey) if (g_Globals.General.MenuKey == key) g_Globals.General.MenuKey = 0;
}

// ============================================================================
// NOMBRE LEGIBLE DE UNA TECLA VK
// ============================================================================
static const char* KeyName(int vk)
{
	if (vk <= 0 || vk == VK_CANCEL) return "NINGUNA";
	switch (vk) {
	case VK_LBUTTON: return "CLICK IZQ";
	case VK_RBUTTON: return "CLICK DER";
	case VK_MBUTTON: return "RUEDA";
	case VK_XBUTTON1: return "MOUSE 4";
	case VK_XBUTTON2: return "MOUSE 5";
	case VK_INSERT:  return "INSERT";
	case VK_SPACE:   return "SPACE";
	case VK_SHIFT:   return "SHIFT";
	case VK_LSHIFT:  return "L-SHIFT";
	case VK_RSHIFT:  return "R-SHIFT";
	case VK_CONTROL: return "CTRL";
	case VK_LCONTROL: return "L-CTRL";
	case VK_RCONTROL: return "R-CTRL";
	case VK_MENU:    return "ALT";
	case VK_LMENU:   return "L-ALT";
	case VK_RMENU:   return "R-ALT";
	case VK_CAPITAL: return "CAPS";
	case VK_NUMLOCK: return "NUM";
	case VK_SCROLL:  return "SCROLL";
	case VK_TAB:     return "TAB";
	case VK_ESCAPE:  return "ESC";
	case VK_RETURN:  return "ENTER";
	case VK_BACK:    return "SUPR";
	case VK_DELETE:  return "DEL";
	case VK_UP:      return "ARRIBA";
	case VK_DOWN:    return "ABAJO";
	case VK_LEFT:    return "IZQUIERDA";
	case VK_RIGHT:   return "DERECHA";
	case VK_HOME:    return "HOME";
	case VK_END:     return "END";
	case VK_PRIOR:   return "PAGE UP";
	case VK_NEXT:    return "PAGE DOWN";
	case VK_SNAPSHOT:return "PRINT";
	case VK_PAUSE:   return "PAUSE";
	default:
		if (vk >= 'A' && vk <= 'Z') {
			static char b[2] = { (char)vk, 0 };
			return b;
		}
		if (vk >= '0' && vk <= '9') {
			static char b[2] = { (char)vk, 0 };
			return b;
		}
		if (vk >= VK_F1 && vk <= VK_F24) {
			static char b[8];
			ImFormatString(b, sizeof(b), "F%d", vk - VK_F1 + 1);
			return b;
		}
		return "TECLA";
	}
}

// ============================================================================
// FILA CON CAPTURA DE TECLA (keybox full-width estilo HESCO custom::Keybind)
// ============================================================================
static std::map<ImGuiID, bool> g_capturing;
static std::map<ImGuiID, float> g_captureStart;

static bool KeyBindRow(ImDrawList* dl, const char* rowId, const char* label, const char* sub, int* v, ImU32 accent, float pulse, float width = 0.0f, bool* state = nullptr)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	ImGui::PushID(rowId);
	ImGuiID rid = ImGui::GetID("##key");
	const bool cap = g_capturing[rid];
	float& capT = g_captureStart[rid];

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 4), C_TEXT_ACTIVE, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();
	if (sub && Fonts::InterRegular14) {
		ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(12, 21), C_DESC_ACTIVE, sub);
		ImGui::PopFont();
	}

	// Keybox full-width (25px)
	ImRect box(p + ImVec2(12, 36), p + ImVec2(w - 12, 61));
	const bool hovBadge = ImGui::IsMouseHoveringRect(box.Min, box.Max);
	ImU32 bg = cap ? LerpColor(C_SECOND, (accent & 0x00FFFFFF) | (80u << 24), 0.5f + pulse * 0.5f) : (hovBadge ? C_SECOND_HOVER : C_SECOND);
	dl->AddRectFilled(box.Min, box.Max, bg, 4.0f);
	ImU32 bc = cap ? accent : (hovBadge ? (accent & 0x00FFFFFF) | (70u << 24) : C_STROKE);
	dl->AddRect(box.Min, box.Max, bc, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	// Texto centrado (el estado ON/OFF se dibuja a la derecha y se descuenta del area central)
	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	const char* kn = cap ? "Click para asignar" : KeyName(*v);
	ImVec2 ks = ImGui::CalcTextSize(kn);
	float cx = (box.GetWidth() - ks.x) * 0.5f;
	if (state) cx = (box.GetWidth() - 48.0f - ks.x) * 0.5f + 6.0f;
	ImU32 kc = cap ? accent : (*v ? C_TEXT_ACTIVE : C_TEXT_DEFAULT);
	dl->AddText(box.Min + ImVec2(cx, (box.GetHeight() - ks.y) * 0.5f), kc, kn);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	// Chip de estado ON/OFF dentro del keybox
	if (state) {
		const bool on = *state;
		ImVec2 cMin(box.Max.x - 40, box.GetCenter().y - 7);
		ImVec2 cs(26, 14);
		ImU32 cb = on ? (accent & 0x00FFFFFF) | (45u << 24) : IM_COL32(40, 40, 46, 255);
		dl->AddRectFilled(cMin, cMin + cs, cb, 7.0f);
		dl->AddCircleFilled(cMin + ImVec2(7, 7), 2.5f, on ? accent : C_TEXT_DEFAULT, 24);
		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		dl->AddText(cMin + ImVec2(13, 2), on ? accent : C_TEXT_DEFAULT, on ? "ON" : "OFF");
		if (Fonts::InterBold12) ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, 60));

	if (cap) {
		// Pulsar el keybox otra vez cancela la captura
		if (ImGui::IsMouseClicked(0) && hovBadge) {
			g_capturing[rid] = false;
		}
		else {
			float now = (float)GetTickCount64() / 1000.0f;
			for (int key = 0x01; key < 0xFE; key++) {
				// ESC desasigna la tecla: la funcion queda en "NINGUNA" y
				// no se captura ESC como tecla asignada.
				if (key == VK_ESCAPE) {
					if (GetAsyncKeyState(key) & 0x8000) {
						*v = 0;
						g_capturing[rid] = false;
						break;
					}
					continue;
				}
				// INSERT esta reservada SIEMPRE para abrir/cerrar el menu:
				// nunca se puede asignar a una funcion de exploit.
				if (key == VK_INSERT) continue;
				// Unico bloqueo: el click IZQUIERDO sobre el keybox o justo
				// tras empezar (0.4s) — asi el click que abre la captura
				// no se asigna a si mismo. El resto de botones del mouse
				// (derecho, rueda, 4 y 5) se capturan SIEMPRE.
				if (key == VK_LBUTTON && (hovBadge || now - capT < 0.4f)) continue;
				if (GetAsyncKeyState(key) & 0x8000) {
					// Condicion anti-duplicado: la tecla se limpia de TODAS
					// las otras funciones para que nunca haya dos binds con
					// la misma tecla (eso hacia que se activaran juntas).
					*v = key;
					ClearKeyFromOtherBinds(key, v);
					g_capturing[rid] = false;
					break;
				}
			}
		}
	}
	else if (ImGui::IsMouseClicked(0) && hovBadge) {
		g_capturing[rid] = true;
		capT = (float)GetTickCount64() / 1000.0f;
	}

	ImGui::PopID();
	return cap;
}

// ============================================================================
// SELECTOR DE ARMA (click abre la lista completa; rueda del raton cicla)
// ============================================================================
struct BoostWeaponEntry {
	int id;
	const char* name;
};

static const BoostWeaponEntry g_weapons[] = {
	{ 0, "TODAS" },
	{ 1, "Punos" },
	{ 2, "M4A1" },
	{ 3, "USP" },
	{ 4, "AWM" },
	{ 5, "M1014" },
	{ 6, "AK47" },
	{ 7, "UMP" },
	{ 8, "MP5" },
	{ 9, "DESERT EAGLE" },
	{ 10, "G18" },
	{ 11, "M14" },
	{ 12, "SCAR" },
	{ 13, "VSS" },
	{ 14, "GROZA" },
	{ 15, "MP40" },
	{ 16, "SARTEN" },
	{ 17, "MACHETE" },
	{ 18, "SKS" },
	{ 19, "M249" },
	{ 20, "M1873" },
	{ 21, "KAR98K" },
	{ 24, "FAMAS" },
	{ 25, "M500" },
	{ 26, "SVD" },
	{ 27, "BATE" },
	{ 28, "XM8" },
	{ 29, "SPAS12" },
	{ 30, "M60" },
	{ 32, "P90" },
	{ 33, "AN94" },
	{ 34, "KATANA" },
	{ 35, "CG15" },
	{ 39, "PLASMA" },
	{ 41, "M1887" },
	{ 43, "THOMPSON" },
	{ 45, "M828" },
	{ 46, "AUG" },
	{ 47, "PARAFAL" },
	{ 48, "WOODPECKER" },
	{ 49, "VECTOR" },
	{ 50, "MAG-7" },
	{ 51, "HOZ" },
	{ 54, "KORD" },
	{ 55, "M1917" },
	{ 56, "USP-2" },
	{ 57, "KINGFISHER" },
	{ 58, "MINI UZI" },
	{ 60, "MP5 1" },
	{ 62, "VSS 1" },
	{ 65, "AWM-Y" },
	{ 67, "FAMAS 1" },
	{ 70, "GROZA-X" },
	{ 71, "M249-X" },
	{ 72, "SVD-Y" },
	{ 73, "G36" },
	{ 75, "M24" },
	{ 78, "RIFLE CURATIVO" },
	{ 80, "M4A1 1" },
	{ 81, "M4A1 2" },
	{ 82, "M4A1 3" },
	{ 86, "CHARGE BUSTER" },
	{ 88, "MAC10" },
	{ 89, "AC80" },
	{ 93, "PISTOLA CURATIVA" },
	{ 99, "ARMA ESCUDO" },
	{ 100, "LANZALLAMAS" },
	{ 119, "M1887-X" },
	{ 120, "MP5 2" },
	{ 121, "MP5 3" },
	{ 124, "VSS 2" },
	{ 125, "VSS 3" },
	{ 130, "FAMAS 2" },
	{ 131, "FAMAS 3" },
	{ 150, "BIZON" },
	{ 178, "SCAR 1" },
	{ 179, "SCAR 2" },
	{ 180, "SCAR 3" },
	{ 181, "TROGON" },
	{ 182, "TROGON - GRANADA" },
	{ 184, "M1014 1" },
	{ 185, "M1014 2" },
	{ 186, "M1014 3" },
	{ 193, "AUG 1" },
	{ 194, "AUG 2" },
	{ 195, "AUG 3" },
	{ 197, "VSK94" },
	{ 228, "MAC10 1" },
	{ 229, "MAC10 2" },
	{ 230, "MAC10 3" },
	{ 602, "GRANADA CEGADORA" },
	{ 608, "GRANADA HIELO" },
	{ 1204, "PARED GLOO" },
	{ 10449, "TRIPLE SHURIKEN" },
	{ 21001, "PISTOLA CURATIVA-Y" },
	{ 21002, "M590" },
	{ 21003, "HEAVY SHURIKEN" },
	{ 21004, "LIGHT SHURIKEN" },
};
static const int g_weaponsCount = (int)(sizeof(g_weapons) / sizeof(g_weapons[0]));

static void WeaponSelectorRow(ImDrawList* dl, const char* rowId, const char* label, const char* sub, int* v, ImU32 accent, float pulse, float width = 0.0f)
{
	int idx = 0;
	for (int i = 0; i < g_weaponsCount; i++) {
		if (g_weapons[i].id == *v) { idx = i; break; }
	}

	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
	ImVec2 p0 = p + ImVec2(2, 0), p1 = p + ImVec2(w - 2, 44);

	if (ImGui::IsMouseHoveringRect(p0, p1))
		dl->AddRectFilled(p0, p1, IM_COL32(200, 200, 200, 8), 4.0f);

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 6), C_TEXT_ACTIVE, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	if (sub && Fonts::InterRegular14) {
		ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(12, 26), C_DESC_ACTIVE, sub);
		ImGui::PopFont();
	}

	const ImVec2 bw = p + ImVec2(w - 16 - 140, 9);
	const ImVec2 bs(140, 26);
	const bool hovBadge = ImGui::IsMouseHoveringRect(bw, bw + bs);
	dl->AddRectFilled(bw, bw + bs, hovBadge ? C_SECOND_HOVER : C_SECOND, 4.0f);
	dl->AddRect(bw, bw + bs, hovBadge ? (accent & 0x00FFFFFF) | (70u << 24) : C_STROKE, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	char badge[40];
	ImFormatString(badge, sizeof(badge), "%s (%d)", g_weapons[idx].name, g_weapons[idx].id);
	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	ImVec2 ws = ImGui::CalcTextSize(badge);
	if (ws.x > bs.x - 12) {
		// recortar el nombre con "..." para que quepa en el badge
		for (int c = 0; badge[c]; c++) {
			badge[c + 3] = 0;
			ImVec2 ts = ImGui::CalcTextSize(badge);
			if (ts.x > bs.x - 12) break;
		}
		ImFormatString(badge, sizeof(badge), "%s... (%d)", g_weapons[idx].name, g_weapons[idx].id);
		// segundo recorte si aun es largo
		while (true) {
			ImVec2 ts = ImGui::CalcTextSize(badge);
			if (ts.x <= bs.x - 12) break;
			size_t ln = strlen(badge);
			if (ln <= 6) break;
			badge[ln - 1] = 0;
		}
	}
	ws = ImGui::CalcTextSize(badge);
	dl->AddText(bw + ImVec2((bs.x - ws.x) * 0.5f, (bs.y - ws.y) * 0.5f - 1.0f), C_TEXT_ACTIVE, badge);
	if (Fonts::InterBold12) ImGui::PopFont();

	ImGui::PushID(rowId);
	ImGui::ItemSize(ImVec2(w, 44));
	if (hovBadge) {
		// Rueda del raton: ciclar entre armas rapidamente
		float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			int dir = wheel > 0.0f ? 1 : -1;
			*v = g_weapons[(idx + dir + g_weaponsCount) % g_weaponsCount].id;
		}
		// Click: abrir la lista completa
		if (ImGui::IsMouseClicked(0)) {
			ImGui::OpenPopup("##BoostWeaponList");
		}
	}
	if (ImGui::BeginPopup("##BoostWeaponList")) {
		ImGui::PushStyleColor(ImGuiCol_PopupBg, C_BACKGROUND);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, C_SECOND);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, C_SECOND_HOVER);
		ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_ACTIVE);
		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		ImGui::Text("ARMA A MEJORAR");
		if (Fonts::InterBold12) ImGui::PopFont();

		if (ImGui::BeginListBox("##wlist", ImVec2(360.0f, 380.0f))) {
			for (int i = 0; i < g_weaponsCount; i++) {
				char item[48];
				ImFormatString(item, sizeof(item), "%s  (%d)", g_weapons[i].name, g_weapons[i].id);
				const bool sel = (i == idx);
				if (ImGui::Selectable(item, sel)) {
					*v = g_weapons[i].id;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::PopStyleColor(4);
		ImGui::EndPopup();
	}
	ImGui::PopID();

	ImGui::Dummy(ImVec2(0, 44));
}

// ============================================================================
// PESTAÑA AIMBOT
// ============================================================================
static void AimbotTab(ImDrawList* dl, float pulse)
{
	float w = ImGui::GetContentRegionAvail().x;

	if (Card(dl, "##CardAimMa", "MEMORY AIM", "Escritura de rotacion en memoria", "\uF05B", w, 470.0f)) {
		RowToggle(dl, "##AimMa", "Aimbot Memoria", "Rota la camara hacia el enemigo",
			&g_Globals.AimBot.MemoryAim, C_ACCENT, pulse);
		KeyBindRow(dl, "##AimMaK", "Tecla de activacion", "Mantener pulsada para aimear",
			&g_Globals.AimBot.AimbotBind, C_ACCENT, pulse);
		BoneSelector(dl, "##AimMaB", "TARGET (BONE)", &g_Globals.AimBot.TargetBone, C_ACCENT, pulse);
		SliderRow(dl, "##AimFov", "FOV (todos)", "%d px",
			&g_Globals.AimBot.DistanceAim, 50, 500, C_ACCENT, pulse);
		RowToggle(dl, "##AimFv", "Mostrar FOV", "Circulo del radio en pantalla",
			&g_Globals.Misc.ShowAimbotFov, C_ACCENT, pulse);
		RowToggle(dl, "##AimMaEn", "Solo Enemigos", "No aimear a aliados",
			&g_Globals.AimBot.OnlyEnemies, C_ACCENT, pulse);
		RowToggle(dl, "##AimMaKn", "Ignorar Derribados", "No aimear a jugadores knocked",
			&g_Globals.AimBot.IgnoreKnocked, C_ACCENT, pulse);
		RowToggle(dl, "##AimMaBt", "Ignorar Bots", "No aimear a bots",
			&g_Globals.AimBot.IgnoreBots, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardAimSi", "SILENT AIM", "Redirige el proyectil al target", "\uF1D8", w, 338.0f)) {
		RowToggle(dl, "##AimSi", "Silent Aim", "Activo manteniendo el click izquierdo",
			&g_Globals.Silent.Enabled, C_ACCENT, pulse);
		RowToggle(dl, "##AimSiV", "Solo Visibles", "Apuntar solo a entidades visibles",
			&g_Globals.Silent.OnlyVisible, C_ACCENT, pulse);
		BoneSelector(dl, "##AimSiB", "TARGET (BONE)", &g_Globals.Silent.TargetBone, C_ACCENT, pulse);
		SliderRow(dl, "##AimSiSpd", "Velocidad de Bala", "%.0f m/s",
			&g_Globals.Silent.BulletSpeed, 200.0f, 2000.0f, C_ACCENT, pulse);
		SliderRow(dl, "##AimSiAcc", "Precision Hip-Fire", "%.2f",
			&g_Globals.Silent.HipFireAccuracy, 0.5f, 1.0f, C_ACCENT, pulse);
		InfoRow(dl, "Elige el hueso del target; la velocidad de bala predice", "el movimiento. 1.0 = maxima precision, 0.5 = permisivo");
	}

	if (Card(dl, "##CardAimRa", "RAGE AIM", "Fija el collider del enemigo", "\uF140", w, 53.0f)) {
		RowToggle(dl, "##AimRa", "Rage Aimbot", "Se activa con CLICK IZQUIERDO",
			&g_Globals.AimBot.RageAim, C_ACCENT, pulse);
	}
}

// ============================================================================
// PESTAÑA ESP
// ============================================================================
static void EspTab(ImDrawList* dl, float pulse)
{
	float w = ImGui::GetContentRegionAvail().x;

	if (Card(dl, "##CardEspSt", "SETTINGS", "Glow y apariencia del ESP", "\uF013", w, 116.0f)) {
		RowToggle(dl, "##EspGlow", "Glow", "Halo resaltado en el ESP", &g_Globals.Visuals.Glow, C_ACCENT, pulse);
		SliderRow(dl, "##SlGlow", "Intensidad", "%d %%", &g_Globals.Visuals.GlowIntensity, 0, 100, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardEspPo", "POSICION", "Posicion de la linea del ESP", "\uF0EC", w, 73.0f)) {
		TypeSelector(dl, "##TpEsp", "TYPE", &g_Globals.Visuals.EspLines, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardEspFi", "FILTRO", "Que jugadores mostrar", "\uF0B0", w, 159.0f)) {
		RowToggle(dl, "##EspFb", "Ignorar Bots", "No mostrar bots", &g_Globals.Visuals.IgnoreBots, C_ACCENT, pulse);
		RowToggle(dl, "##EspFk", "Ignorar Derribados", "Ocultar jugadores knocked", &g_Globals.Visuals.IgnoreKnocked, C_ACCENT, pulse);
		RowToggle(dl, "##EspFv", "Solo Visibles", "Mostrar solo enemigos visibles", &g_Globals.Visuals.OnlyVisible, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardEspCo", "COLORS", "Colores por grupo de jugador", "\uF1FB", w, 676.0f)) {
		GroupColors(dl, "BOTS", &g_Globals.Visuals.ColBots, pulse, 0.0f);
		GroupColors(dl, "VISIBLES", &g_Globals.Visuals.ColVisible, pulse, 0.0f);
		GroupColors(dl, "KNOCKED", &g_Globals.Visuals.ColKnocked, pulse, 0.0f);
		GroupColors(dl, "NORMAL", &g_Globals.Visuals.ColNormal, pulse, 0.0f);
	}

	if (Card(dl, "##CardEspVi", "VISUALES", "Elementos en pantalla", "\uF06E", w, 579.0f)) {
		if (RowToggle(dl, "##EspLine", "ESP Line", "Linea hacia todos los enemigos", &g_Globals.Visuals.Lines, C_ACCENT, pulse)) {
			g_Globals.Visuals.EspLines = g_Globals.Visuals.Lines ? 1 : 0;
			if (g_Globals.Visuals.Lines) {
				g_Globals.Visuals.Enable = true;
			}
		}
		RowToggle(dl, "##EspBox", "Caja", "Caja alrededor del enemigo", &g_Globals.Visuals.Box, C_ACCENT, pulse);
		RowToggle(dl, "##EspFill", "Caja Rellena", "Relleno semi transparente", &g_Globals.Visuals.FilledBox, C_ACCENT, pulse);
		RowToggle(dl, "##EspHb", "Barra de Vida", "Barra de salud en pantalla", &g_Globals.Visuals.HealthBar, C_ACCENT, pulse);
		RowToggle(dl, "##EspNm", "Nombre", "Nombre del enemigo", &g_Globals.Visuals.Name, C_ACCENT, pulse);
		RowToggle(dl, "##EspDs", "Distancia", "Distancia al enemigo", &g_Globals.Visuals.Distance, C_ACCENT, pulse);
		RowToggle(dl, "##EspMm", "Minimapa", "Radar en pantalla", &g_Globals.Visuals.Minimap, C_ACCENT, pulse);
		RowToggle(dl, "##EspWm", "Marca de Agua", "Logo ASMODEUS en pantalla", &g_Globals.Visuals.Watermark, C_ACCENT, pulse);
		RowToggle(dl, "##EspSk", "Esqueleto", "Huesos de los enemigos", &g_Globals.Visuals.Skeleton, C_ACCENT, pulse);
		RowToggle(dl, "##EspSkL", "Mi Esqueleto", "Huesos de tu personaje", &g_Globals.Visuals.LocalSkeleton, C_ACCENT, pulse);
		RowColor(dl, "##EspSkC", "Color Esqueleto", g_Globals.Visuals.SkeletonColor, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardEspRa", "RANGO", "Alcance del ESP", "\uF002", w, 63.0f)) {
		SliderRow(dl, "##SlEsp", "Alcance del ESP", "%d m", &g_Globals.Visuals.DistanceEsp, 50, 500, C_ACCENT, pulse);
	}
}

// ============================================================================
// PESTAÑA EXPLOITS
// ============================================================================
static void ExploitsTab(ImDrawList* dl, float pulse)
{
	float w = ImGui::GetContentRegionAvail().x;

	if (Card(dl, "##CardExEx", "EXPLOITS", "Mejoras de combate por frame", "\uF0E7", w, 1322.0f)) {
		RowToggle(dl, "##ExFs", "Fast Switch", "Cambio de arma instantaneo",
			&g_Globals.Exploits.FastSwitch, C_ACCENT, pulse);
		RowToggle(dl, "##ExNr", "Sin Retroceso", "Elimina el retroceso del arma",
			&g_Globals.Exploits.NoRecoil, C_ACCENT, pulse);
		RowToggle(dl, "##ExNl", "Sin Recarga", "No necesitas recargar el arma",
			&g_Globals.Exploits.NoReload, C_ACCENT, pulse);
		RowToggle(dl, "##ExUa", "Municion Infinita", "Nunca gastas municion",
			&g_Globals.Exploits.UnlimitedAmmo, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExUaK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.UnlimitedAmmoKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.UnlimitedAmmo);
		RowToggle(dl, "##ExSh", "Speed Hack", "Corre mas rapido",
			&g_Globals.Exploits.SpeedHack, C_ACCENT, pulse);
		SliderRow(dl, "##ExShM", "Velocidad carrera", "%.2fx",
			&g_Globals.Exploits.SpeedHackMultiplier, 1.0f, 2.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExJmp", "Jump Hack", "Salto mucho mas alto",
			&g_Globals.Exploits.JumpHack, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExJmpK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.JumpHackKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.JumpHack);
		SliderRow(dl, "##ExJmpM", "Altura salto", "%.1fx",
			&g_Globals.Exploits.JumpHeightMultiplier, 1.0f, 5.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExVsn", "Vision Hack", "Ajusta el FOV de la camara",
			&g_Globals.Exploits.VisionHack, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExVsnK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.VisionHackKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.VisionHack);
		SliderRow(dl, "##ExVsnF", "FOV vision", "%.1f",
			&g_Globals.Exploits.VisionSlider, 0.0f, 10.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExFf", "Caida Rapida", "Cae al piso mucho mas rapido",
			&g_Globals.Exploits.FastFall, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExFfK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.FastFallKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.FastFall);
		SliderRow(dl, "##ExFfS", "Velocidad caida", "%.1f",
			&g_Globals.Exploits.FastFallSpeed, 1.0f, 50.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExSp", "Spin Bot", "Gira TU personaje sobre su eje vertical",
			&g_Globals.Exploits.SpinBot, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExSpK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.SpinBotKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.SpinBot);
		SliderRow(dl, "##ExSpS", "Velocidad de giro", "%.0f deg/s",
			&g_Globals.Exploits.SpinBotSpeed, 30.0f, 1080.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExTw", "Tp Wall", "Funcion independiente. Un clic = 1 paso (se desactiva solo)",
			&g_Globals.Exploits.TpWall, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExTwK", "Tecla (paso)", "Cada pulsacion = activa, 1 paso, se desactiva solo",
			&g_Globals.Exploits.TpWallKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.TpWall);
		SliderRow(dl, "##ExTwD", "Distancia por paso", "%.1f m",
			&g_Globals.Exploits.TpWallDistance, 0.1f, 10.0f, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardExTk", "TELE KILL", "Teleporta al enemigo mas cercano", "\uF0EC", w, 185.0f)) {
		RowToggle(dl, "##ExTk", "Tele Kill", "Acerca al enemigo a tu posicion",
			&g_Globals.Exploits.TeleKill, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExTkK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.TeleKillKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.TeleKill);
		SliderRow(dl, "##ExTkD", "Distancia de uso", "%0.1f m",
			&g_Globals.Exploits.TeleKillDistance, 1.0f, 50.0f, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardExMo", "MOVIMIENTO", "Movimiento y posicion", "\uF135", w, 1051.0f)) {
		RowToggle(dl, "##ExSt", "Speed Timer", "Acelera el tiempo y tu velocidad",
			&g_Globals.Exploits.SpeedTimer, C_ACCENT, pulse);
		SliderRow(dl, "##ExStM", "Velocidad", "%.1fx",
			&g_Globals.Exploits.SpeedMultiplier, 1.0f, 5.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExUp", "Under Player", "Tecla: hundir y volver",
			&g_Globals.Exploits.UnderPlayer, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExUpK", "Tecla activar", "Alterna hundido / visible",
			&g_Globals.Exploits.UnderPlayerKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.UnderPlayer);
		RowToggle(dl, "##ExFy", "Fly", "Sube X metros en Y al activar",
			&g_Globals.Exploits.Fly, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExFyK", "Tecla activar", "Alterna subir / volver",
			&g_Globals.Exploits.FlyKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.Fly);
		SliderRow(dl, "##ExFyH", "Altura", "%0.1f m",
			&g_Globals.Exploits.FlyHeight, 0.1f, 10.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExPp", "Pull Player", "Jala enemigos a tu mira",
			&g_Globals.Exploits.PullPlayer, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExPpK", "Tecla activar", "Alterna jalar al disparar/apuntar",
			&g_Globals.Exploits.PullPlayerKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.PullPlayer);
		TriSelector(dl, "##ExPpB", "Hueso", &g_Globals.Exploits.PullBone,
			"HEAD", "SPINE", "ROOT", C_ACCENT, pulse);
		SliderRow(dl, "##ExPpD", "Distancia", "%0.0f m",
			&g_Globals.Exploits.PullDis, 10.0f, 300.0f, C_ACCENT, pulse);
		SliderRow(dl, "##ExPpF", "FOV", "%0.0f px",
			&g_Globals.Exploits.PullFov, 30.0f, 300.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExTp", "Teleport", "Clava tu posicion en la del enemigo mas cercano",
			&g_Globals.Exploits.Teleport, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExTpK", "Tecla activar", "Alterna clavado / normal",
			&g_Globals.Exploits.TeleportKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.Teleport);
		SliderRow(dl, "##ExTpD", "Distancia", "%0.0f m",
			&g_Globals.Exploits.TeleportDistance, 10.0f, 300.0f, C_ACCENT, pulse);
		RowToggle(dl, "##ExTe", "Turn 180", "Flip 180 vertical: cabeza <-> pies en los enemigos",
			&g_Globals.Exploits.TurnEnemies, C_ACCENT, pulse);
		KeyBindRow(dl, "##ExTeK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.TurnEnemiesKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.TurnEnemies);
	}
}

// ============================================================================
// PESTAÑA BOOST
// ============================================================================
static void BoostTab(ImDrawList* dl, float pulse)
{
	float w = ImGui::GetContentRegionAvail().x;

	if (Card(dl, "##CardBsWa", "MEJORA DE ARMAS", "Fire rate por nivel de arma", "\uF0AD", w, 169.0f)) {
		RowToggle(dl, "##BstWa", "Mejora de Armas", "Fire rate por nivel de arma",
			&g_Globals.Exploits.WeaponAttributes, C_ACCENT, pulse);
		SliderRow(dl, "##BstWaL", "Nivel de arma", "LV%d",
			&g_Globals.Exploits.WeaponLevel, 0, 3, C_ACCENT, pulse);
		WeaponSelectorRow(dl, "##BstWaW", "Arma a mejorar", "Click para cambiar",
			&g_Globals.Exploits.BoostWeaponId, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardBsMu", "MINI UZI SPEED", "Speed hack solo con Mini Uzi", "\uF0D0", w, 116.0f)) {
		RowToggle(dl, "##BstMu", "Mini Uzi Speed", "Speed hack solo con Mini Uzi",
			&g_Globals.Exploits.MiniUziSpeed, C_ACCENT, pulse);
		SliderRow(dl, "##BstMuM", "Multiplicador", "%.2fx",
			&g_Globals.Exploits.MiniUziSpeedMultiplier, 1.05f, 1.5f, C_ACCENT, pulse);
	}
}

// ============================================================================
// PESTAÑA CONFIG
// ============================================================================
static void ConfigTab(ImDrawList* dl, float pulse)
{
	float w = ImGui::GetContentRegionAvail().x;

	if (Card(dl, "##CardCfAd", "CONEXION ADB", "BlueStacks Emulator", "\uF1E6", w, 86.0f)) {
		ImVec2 p = ImGui::GetCursorScreenPos();

		dl->AddRectFilled(p, p + ImVec2(w, 76), C_CHILD_BG, 4.0f);
		dl->AddRect(p, p + ImVec2(w, 76), C_CHILD_STROKE, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);

		if (Fonts::FontAwesomeSolid) {
			ImGui::PushFont(Fonts::FontAwesomeSolid);
			ImVec2 is = ImGui::CalcTextSize("\uF1E6");
			dl->AddText(p + ImVec2(20 - is.x * 0.5f, 20 - is.y * 0.5f), C_ACCENT, "\uF1E6");
			ImGui::PopFont();
		}

		if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
		dl->AddText(p + ImVec2(44, 8), C_TEXT_ACTIVE, "CONEXION ADB");
		if (Fonts::InterSemiBold) ImGui::PopFont();
		if (Fonts::InterRegular14) ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(44, 28), C_DESC_ACTIVE, "BlueStacks Emulator");
		if (Fonts::InterRegular14) ImGui::PopFont();

		const char* btnLabel;
		ImU32 btnColor;
		bool btnEnabled;
		switch (g_AdbState) {
		case AdbState::Connected:   btnLabel = "CONECTADO";   btnColor = C_ACCENT; btnEnabled = false; break;
		case AdbState::Connecting:  btnLabel = "CONECTANDO";  btnColor = IM_COL32(255, 200, 40, 255); btnEnabled = false; break;
		case AdbState::Failed:      btnLabel = "RECONECTAR";  btnColor = IM_COL32(255, 80, 80, 255); btnEnabled = true;  break;
		default:                    btnLabel = "CONECTAR";    btnColor = C_ACCENT; btnEnabled = true;  break;
		}
		if (PremiumButton(p + ImVec2(w - 16 - 100, 23), ImVec2(100, 30), btnLabel, btnColor, btnEnabled, pulse)) {
			ConnectAdb();
		}

		ImGui::Dummy(ImVec2(0, 86));
	}
}

// ============================================================================
// PESTAÑA MISC
// ============================================================================
static void MiscTab(ImDrawList* dl, float pulse)
{
	float w = ImGui::GetContentRegionAvail().x;

	if (Card(dl, "##CardMsGh", "GHOST LAG", "El enemigo te ve congelado", "\uF012", w, 185.0f)) {
		RowToggle(dl, "##MscGh", "Ghost Lag", "El enemigo te ve congelado, tu danas igual",
			&g_Globals.Exploits.GhostLag, C_ACCENT, pulse);
		KeyBindRow(dl, "##MscGhK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.GhostLagKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.GhostLag);
		SliderRow(dl, "##MscGhD", "Retardo", "%d ms",
			&g_Globals.Exploits.GhostLagDelay, 50, 5000, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardMsFk", "FAKE LAG", "Congela a los enemigos", "\uF070", w, 185.0f)) {
		RowToggle(dl, "##MscFk", "Fake Lag", "Congela a los enemigos; al desactivar les cuenta el danno",
			&g_Globals.Exploits.FakeLag, C_ACCENT, pulse);
		KeyBindRow(dl, "##MscFkK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.FakeLagKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.FakeLag);
		SliderRow(dl, "##MscFkD", "Retencion", "%d ms",
			&g_Globals.Exploits.FakeLagDelay, 0, 5000, C_ACCENT, pulse);
	}

	if (Card(dl, "##CardMsTp", "TELEPORT", "Teleport por red", "\uF0EC", w, 195.0f)) {
		RowToggle(dl, "##MscTp", "Teleport", "Te ven congelado en A; al desactivar solo te ven en B (no vuelve)",
			&g_Globals.Exploits.TeleportLag, C_ACCENT, pulse);
		KeyBindRow(dl, "##MscTpK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.TeleportLagKey, C_ACCENT, pulse, 0.0f, &g_Globals.Exploits.TeleportLag);
		DualSelector(dl, "##MscTpM", "Modo", &g_Globals.Exploits.TeleportLagMode,
			"SOLO SALIENTE", "AMBOS SENTIDOS", C_ACCENT, pulse);
	}
}

// ============================================================================
// INICIALIZACION Y CONFIGURACION
// ============================================================================

void Interface::Initialize(HWND Window, HWND TargetWindow, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext) {
	hWindow = Window;
	hTargetWindow = TargetWindow;
	IDevice = Device;
	g_pd3dDevice = Device;
	g_pd3dDeviceContext = DeviceContext;

	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWindow);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	Fonts::Initialize(IDevice);

	InitializeMenu();
}

void Interface::InitializeMenu() {
	bIsMenuOpen = true;
	// Sin WS_EX_TRANSPARENT: la ventana debe recibir los clicks del raton
	SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
	SetForegroundWindow(hWindow);
	SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

void Interface::UpdateStyle() {
	ImGuiStyle* Style = &ImGui::GetStyle();

	Style->AntiAliasedLines = true;
	Style->AntiAliasedLinesUseTex = true;
	Style->AntiAliasedFill = true;

	Style->WindowRounding = 8.0f;
	Style->FrameRounding = 4.0f;
	Style->GrabRounding = 4.0f;
	Style->GrabMinSize = 4.0f;
	Style->WindowBorderSize = 0.0f;
	Style->FrameBorderSize = 0.0f;
	Style->WindowPadding = ImVec2(13, 13);
	Style->ItemSpacing = ImVec2(13, 9);
	Style->ScrollbarSize = 3.0f;
	Style->ScrollbarRounding = 2.0f;
	Style->PopupRounding = 6.0f;

	Style->Colors[ImGuiCol_WindowBg] = ImColor(C_WINDOW_BG);
	Style->Colors[ImGuiCol_ChildBg] = ImColor(0, 0, 0, 0);
	Style->Colors[ImGuiCol_Border] = ImColor(C_STROKE);
	Style->Colors[ImGuiCol_Text] = ImColor(C_TEXT_ACTIVE);
	Style->Colors[ImGuiCol_TextDisabled] = ImColor(C_TEXT_DEFAULT);
	Style->Colors[ImGuiCol_TextSelectedBg] = ImColor((C_ACCENT & 0x00FFFFFF) | (50u << 24));
	Style->Colors[ImGuiCol_FrameBg] = ImColor(C_SECOND);
	Style->Colors[ImGuiCol_FrameBgHovered] = ImColor(C_SECOND_HOVER);
	Style->Colors[ImGuiCol_FrameBgActive] = ImColor(C_PAGE_ACTIVE);
	Style->Colors[ImGuiCol_CheckMark] = ImColor(C_ACCENT);
	Style->Colors[ImGuiCol_PopupBg] = ImColor(C_BACKGROUND);
	Style->Colors[ImGuiCol_ScrollbarBg] = ImColor(0, 0, 0, 0);
	Style->Colors[ImGuiCol_ScrollbarGrab] = ImColor(C_STROKE);
	Style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor((C_ACCENT & 0x00FFFFFF) | (80u << 24));
	Style->Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(C_ACCENT);
	Style->Colors[ImGuiCol_SliderGrab] = ImColor(C_ACCENT);
	Style->Colors[ImGuiCol_SliderGrabActive] = ImColor(C_ACCENT);
	Style->Colors[ImGuiCol_Header] = ImColor(C_SECOND);
	Style->Colors[ImGuiCol_HeaderHovered] = ImColor(C_SECOND_HOVER);
	Style->Colors[ImGuiCol_HeaderActive] = ImColor(C_PAGE_ACTIVE);
}

// ============================================================================
// RENDERIZADO PRINCIPAL (6 PESTAÑAS: AIMBOT / ESP / EXPLOITS / BOOST / CONFIG / MISC)
// ============================================================================

static void DrawTabContent(ImDrawList* cdl, int tab, float pulse)
{
	switch (tab) {
	case 0: AimbotTab(cdl, pulse); break;
	case 1: EspTab(cdl, pulse); break;
	case 2: ExploitsTab(cdl, pulse); break;
	case 3: BoostTab(cdl, pulse); break;
	case 4: ConfigTab(cdl, pulse); break;
	case 5: MiscTab(cdl, pulse); break;
	}
}

void Interface::RenderGui()
{
	if (!bIsMenuOpen) return;

	ImGuiIO& io = ImGui::GetIO();
	const float dt = ImMin(io.DeltaTime, 0.1f);

	static float tGlobal = 0.0f;
	tGlobal += dt;
	const float pulse = (sinf(tGlobal * 2.4f) + 1.0f) * 0.5f;

	ImGui::SetNextWindowSize(ImVec2(560.0f, 624.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));

	ImGui::Begin("##MainWindow", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImVec2 Pos = ImGui::GetWindowPos();
		ImVec2 Size = ImGui::GetWindowSize();

		// ====================================================================
		// FONDO: particulas + ventana + sidebar + header (estilo HESCO)
		// ====================================================================
		ImDrawList* bg = ImGui::GetBackgroundDrawList();
		bg->PushClipRect(ImVec2(0, 0), ImVec2(4000, 4000), false);
		RenderParticles(bg, io.DisplaySize, dt, tGlobal);

		bg->AddRectFilled(Pos, Pos + Size, C_WINDOW_BG, 8.0f);
		bg->AddRectFilled(ImVec2(Pos.x - 10, Pos.y - 10), Pos + ImVec2(Size.x + 10, Size.y + 10), C_WINDOW_BG, 8.0f);

		int glowA = 16 + (int)(pulse * 10.0f);
		bg->AddRect(Pos - ImVec2(1, 1), Pos + Size + ImVec2(1, 1), (C_ACCENT & 0x00FFFFFF) | ((ImU32)glowA << 24), 8.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		bg->AddRect(Pos + ImVec2(1, 1), Pos + Size - ImVec2(1, 1), C_STROKE, 7.0f, ImDrawFlags_RoundCornersAll, 1.0f);

		// Sidebar (izquierda) y header (arriba a la derecha)
		bg->AddRectFilled(Pos, Pos + ImVec2(170, Size.y), C_CHILD_BG, 8.0f, ImDrawFlags_RoundCornersLeft);
		bg->AddRectFilled(Pos + ImVec2(160, 0), Pos + ImVec2(Size.x, 75), LerpColor(C_CHILD_BG, IM_COL32(0, 0, 0, 0), 0.5f), 8.0f, ImDrawFlags_RoundCornersTop);
		bg->AddRectFilled(Pos + ImVec2(0, 75), Pos + ImVec2(Size.x, 76), C_SEP);
		bg->AddRectFilled(Pos + ImVec2(170, 75), Pos + ImVec2(171, Size.y), C_SEP);
		bg->PopClipRect();

		// ====================================================================
		// BRANDING: titulo arcoiris en el sidebar (estilo HESCO "Joyst")
		// ====================================================================
		if (Fonts::InterExtraBold) {
			ImGui::PushFont(Fonts::InterExtraBold);
			ImVec2 bs = ImGui::CalcTextSize("ASMODEUS");
			ImGui::PopFont();
			DrawRainbowText(dl, Pos + ImVec2((160.0f - bs.x) * 0.5f, 14), "ASMODEUS", tGlobal);
		}
		if (Fonts::InterLight) {
			ImGui::PushFont(Fonts::InterLight);
			ImVec2 sub = ImGui::CalcTextSize("C O N T R O L   P A N E L");
			dl->AddText(Pos + ImVec2((160.0f - sub.x) * 0.5f, 44), C_DESC_ACTIVE, "C O N T R O L   P A N E L");
			ImGui::PopFont();
		}

		// Estado ADB en el header
		DrawPulseDot(dl, Pos + ImVec2(Size.x - 26, 37), AdbStateColor(), pulse);

		// Version al pie del sidebar
		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		dl->AddText(Pos + ImVec2(12, Size.y - 24), C_DESC_DEFAULT, "ASMODEUS v1.0");
		if (Fonts::InterBold12) ImGui::PopFont();

		// ====================================================================
		// PESTAÑAS DEL SIDEBAR con animacion de deslizamiento del contenido
		// ====================================================================
		static const char* tabNames[] = { "AIMBOT", "ESP", "EXPLOITS", "BOOST", "CONFIG", "MISC" };
		static const char* tabIcons[] = { "\uF05B", "\uF06E", "\uF0E7", "\uF0AD", "\uF013", "\uF0C2" };
		static const char* tabDesc[] = {
			"Targeting & FOV settings",
			"Player ESP settings",
			"Combat & movement exploits",
			"Weapon upgrade & speed",
			"Connection & settings",
			"Network lag tools"
		};
		static int curTab = 0;
		static int iTabTarget = 0;
		static bool bTabState = false;
		static float fTabOffset = 0.f;

		fTabOffset = ImLerp(fTabOffset, bTabState ? 700.f : 0.f, GetAnimSpeed());
		if (fTabOffset > 695.f && bTabState) {
			curTab = iTabTarget;
			bTabState = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 8));
		ImGui::SetCursorPos(ImVec2(12.5f, 87.5f));
		ImGui::BeginChild("##Tabs", ImVec2(160, 540), false, ImGuiWindowFlags_NoBackground);
		{
			ImDrawList* tdl = ImGui::GetWindowDrawList();
			for (int i = 0; i < 6; i++) {
				if (HesTab(tdl, tabNames[i], tabIcons[i], (i == curTab), pulse)) {
					if (i != curTab) {
						iTabTarget = i;
						bTabState = true;
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		// ====================================================================
		// SUBTAB / PILL DEL TAB ACTIVO (estilo HESCO SubTab)
		// ====================================================================
		ImGui::SetCursorPos(ImVec2(180.0f, 17.5f));
		ImGui::BeginChild("##SubTabs", ImVec2(370, 50), false, ImGuiWindowFlags_NoBackground);
		{
			ImDrawList* sdl = ImGui::GetWindowDrawList();
			ImVec2 p = ImGui::GetCursorScreenPos();

			ImRect pill(p + ImVec2(4, 4), p + ImVec2(148, 46));
			sdl->AddRectFilled(pill.Min, pill.Max, C_SECOND, 4.0f);
			sdl->AddRect(pill.Min, pill.Max, (C_ACCENT & 0x00FFFFFF) | (70u << 24), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
			sdl->AddRectFilled(pill.Min, pill.Min + ImVec2(3, pill.GetHeight()), (C_ACCENT & 0x00FFFFFF) | (120u << 24), 1.0f);

			if (Fonts::FontAwesomeSolid) {
				ImGui::PushFont(Fonts::FontAwesomeSolid);
				ImVec2 is = ImGui::CalcTextSize(tabIcons[curTab]);
				sdl->AddText(p + ImVec2(20 - is.x * 0.5f, 25 - is.y * 0.5f), C_ACCENT, tabIcons[curTab]);
				ImGui::PopFont();
			}
			if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
			ImVec2 ns = ImGui::CalcTextSize(tabNames[curTab]);
			sdl->AddText(p + ImVec2(36, 25 - ns.y * 0.5f), C_TEXT_ACTIVE, tabNames[curTab]);
			if (Fonts::InterSemiBold) ImGui::PopFont();

			if (Fonts::InterLight) ImGui::PushFont(Fonts::InterLight);
			ImVec2 ds = ImGui::CalcTextSize(tabDesc[curTab]);
			sdl->AddText(p + ImVec2(160, 25 - ds.y * 0.5f), C_DESC_ACTIVE, tabDesc[curTab]);
			if (Fonts::InterLight) ImGui::PopFont();

			// Hint de tecla del menu (derecha)
			if (Fonts::InterLight) ImGui::PushFont(Fonts::InterLight);
			const char* hint = "INSERT para abrir/cerrar";
			ImVec2 hs = ImGui::CalcTextSize(hint);
			sdl->AddText(p + ImVec2(370 - 4 - hs.x, 25 - hs.y * 0.5f), C_DESC_DEFAULT, hint);
			if (Fonts::InterLight) ImGui::PopFont();
		}
		ImGui::EndChild();

		// ====================================================================
		// CONTENIDO DEL TAB ACTIVO (con deslizamiento tipo HESCO: 0 -> 700)
		// ====================================================================
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(13, 9));
		ImGui::SetCursorPos(ImVec2(180, 85 + fTabOffset));
		{
			char childName[32];
			ImFormatString(childName, sizeof(childName), "##Content%d", curTab);
			if (ImGui::BeginChild(childName, ImVec2(370, 540), false, ImGuiWindowFlags_NoScrollbar)) {
				ImDrawList* cdl = ImGui::GetWindowDrawList();
				DrawTabContent(cdl, curTab, pulse);
			}
			ImGui::EndChild();
		}
		ImGui::PopStyleVar();

		// ====================================================================
		// ARRASTRE DEL PANEL DESDE CUALQUIER PARTE
		// ====================================================================
		ImGuiWindow* win = ImGui::GetCurrentWindow();
		ImGuiIO& gio = ImGui::GetIO();
		if (gio.MouseDown[0] && ImGui::IsMouseHoveringRect(Pos, Pos + Size)) {
			ImGuiID activeId = ImGui::GetActiveID();
			bool interactiveHeld = (activeId != 0 && activeId != win->MoveId);
			if (!interactiveHeld && ImGui::IsMouseDragging(0, 8.0f)) {
				ImGui::StartMouseMovingWindow(win);
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);
}

// ============================================================================
// GESTION DE EVENTOS Y VENTANAS
// ============================================================================

void Interface::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED) {
			ResizeWidht = (UINT)LOWORD(lParam);
			ResizeHeight = (UINT)HIWORD(lParam);
		}
		break;
	}

	if (bIsMenuOpen) {
		// El raton se maneja por polling directo (el juego/emulador captura el
		// raton y los mensajes no llegan de forma confiable). Solo se pasan a
		// ImGui las teclas y mensajes restantes para evitar doble entrada.
		switch (uMsg) {
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
			break;
		default:
			ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
			break;
		}
	}
}

void Interface::HandleMenuKey()
{
	static bool MenuKeyDown = false;
	if (GetAsyncKeyState(g_Globals.General.MenuKey) & 0x8000)
	{
		if (!MenuKeyDown)
		{
			MenuKeyDown = true;
			bIsMenuOpen = !bIsMenuOpen;

			if (bIsMenuOpen) {
				// Sin WS_EX_TRANSPARENT para poder interactuar con el menu
				SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
				SetForegroundWindow(hWindow);
			}
			else {
				// Con WS_EX_TRANSPARENT: los clicks pasan al juego (solo se dibuja el ESP)
				SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE);
				SetForegroundWindow(hTargetWindow);
			}
			SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		}
	}
	else {
		MenuKeyDown = false;
	}
}

void Interface::ShutDown() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	Overlay::ShutDown();
}

} // namespace FWork
