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
// PALETA DE COLORES (DISEÑO PREMIUM)
// ============================================================================

#define COL_GREEN       IM_COL32(10, 255, 130, 255)  // Acento principal
#define COL_GREEN_SOFT  IM_COL32(10, 255, 130, 120)  // Verde suave (glow)
#define COL_GREEN_DIM   IM_COL32(10, 255, 130, 60)   // Verde tenue (glow lejano)
#define COL_GREEN_DARK  IM_COL32(6, 60, 32, 255)     // Verde oscuro (fondo)
#define COL_GREEN_BG    IM_COL32(10, 40, 24, 255)    // Verde fondo hover
#define COL_YELLOW      IM_COL32(255, 200, 40, 255)  // Estado de espera
#define COL_RED         IM_COL32(255, 80, 80, 255)   // Estado de error

#define COL_TEXT_MAIN   IM_COL32(235, 240, 237, 255) // Texto principal
#define COL_TEXT_DIM    IM_COL32(130, 145, 137, 255) // Texto secundario
#define COL_TEXT_MUTED  IM_COL32(82, 92, 86, 255)    // Texto tenue

#define COL_BG_WINDOW   IM_COL32(9, 11, 10, 255)     // Fondo de la ventana
#define COL_BG_CARD     IM_COL32(14, 17, 15, 255)    // Fondo de tarjetas
#define COL_BORDER      IM_COL32(30, 36, 32, 255)    // Borde base

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
	case AdbState::Disconnected: return COL_TEXT_MUTED;
	case AdbState::Connecting:   return COL_YELLOW;
	case AdbState::Connected:    return COL_GREEN;
	case AdbState::Failed:       return COL_RED;
	}
	return COL_TEXT_MUTED;
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
// UTILIDADES DE DIBUJO PREMIUM
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

static void DrawShadow(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding)
{
	for (int i = 6; i >= 1; --i) {
		float expand = (float)i * 3.0f;
		int alpha = (int)(28.0f / (float)(i * i));
		dl->AddRect(
			min - ImVec2(expand, expand),
			max + ImVec2(expand, expand),
			IM_COL32(0, 0, 0, alpha), rounding + expand,
			ImDrawFlags_RoundCornersAll, 1.0f);
	}
}

static void DrawGlowText(ImDrawList* dl, const ImVec2& pos, ImU32 color, const char* text)
{
	dl->AddText(pos + ImVec2(1, 1), IM_COL32(0, 0, 0, 220), text);
	const int layers = 2;
	for (int i = layers; i >= 1; --i) {
		float expand = (float)i * 1.2f;
		float alpha = (float)(color >> 24 & 0xFF) * 0.05f;
		ImU32 glow = (color & 0x00FFFFFF) | ((ImU32)alpha << 24);
		dl->AddText(pos + ImVec2(-expand, 0), glow, text);
		dl->AddText(pos + ImVec2(expand, 0), glow, text);
		dl->AddText(pos + ImVec2(0, -expand), glow, text);
		dl->AddText(pos + ImVec2(0, expand), glow, text);
	}
	dl->AddText(pos, color, text);
}

static void DrawPulseDot(ImDrawList* dl, const ImVec2& c, ImU32 color, float pulse)
{
	dl->AddCircleFilled(c, 3.6f, color);
	dl->AddCircle(c, 4.6f, (color & 0x00FFFFFF) | (40u << 24), 24, 1.0f);
	dl->AddCircle(c, 5.5f + pulse * 4.5f, (color & 0x00FFFFFF) | ((ImU32)(int)((1.0f - pulse) * 70.0f) << 24), 24, 1.0f);
}

static void DrawIcon(ImDrawList* dl, const ImVec2& center, float radius, const char* glyph, ImU32 accent, ImFont* font)
{
	dl->AddCircleFilled(center, radius, IM_COL32(9, 13, 11, 255));
	dl->AddCircle(center, radius + 5.0f, (accent & 0x00FFFFFF) | (22u << 24), 32, 1.0f);
	dl->AddCircle(center, radius - 3.5f, (accent & 0x00FFFFFF) | (36u << 24), 32, 1.0f);
	dl->AddCircle(center, radius, accent, 32, 1.0f);
	if (font) ImGui::PushFont(font);
	ImVec2 ts = ImGui::CalcTextSize(glyph);
	dl->AddText(center - ImVec2(ts.x * 0.5f, ts.y * 0.5f), accent, glyph);
	if (font) ImGui::PopFont();
}

static void DrawSweep(ImDrawList* dl, const ImVec2& pos, float width, float t)
{
	const float w = 56.0f;
	float x = ImLerp(-w, width + w, t);
	ImU32 c0 = IM_COL32(255, 255, 255, 0);
	ImU32 c1 = IM_COL32(255, 255, 255, 14);
	dl->AddRectFilledMultiColor(
		ImVec2(pos.x + x - w, pos.y),
		ImVec2(pos.x + x + w, pos.y + 3.0f),
		c0, c1, c1, c0);
}

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

	ImU32 bg = IM_COL32(9, 12, 10, 255);
	if (hov) bg = COL_GREEN_BG;
	if (held) bg = IM_COL32(6, 30, 18, 255);
	dl->AddRectFilled(pos, pos + size, bg, 6.0f);

	float ga = (enabled && hov) ? 1.0f : (enabled ? 0.45f + pulse * 0.35f : 0.55f + pulse * 0.30f);
	dl->AddRect(pos - ImVec2(1.5f, 1.5f), pos + size + ImVec2(1.5f, 1.5f),
		(accent & 0x00FFFFFF) | ((ImU32)(int)(ga * 110.0f) << 24), 7.0f, ImDrawFlags_RoundCornersAll, 1.0f);
	dl->AddRect(pos, pos + size, accent, 6.0f, ImDrawFlags_RoundCornersAll, 1.2f);
	dl->AddRectFilled(pos + ImVec2(2, 0), pos + ImVec2(size.x - 2, 1.5f), (accent & 0x00FFFFFF) | (90u << 24), 1.0f);

	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	ImVec2 ts = ImGui::CalcTextSize(label);
	ImU32 textCol = enabled ? IM_COL32(235, 240, 237, 255) : IM_COL32(150, 165, 155, 255);
	dl->AddText(pos + ImVec2((size.x - ts.x) * 0.5f, (size.y - ts.y) * 0.5f - 1.0f), textCol, label);
	if (Fonts::InterBold12) ImGui::PopFont();

	return pressed;
}

// Switch premium con animacion suave
static bool ToggleSwitch(const char* switchId, ImVec2 pos, ImVec2 size, bool* v, ImU32 accent, float pulse)
{
	static std::map<ImGuiID, float> anims;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiID id = window->GetID(switchId);
	ImRect bb(pos, pos + size);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id)) return false;

	bool hov, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hov, &held);
	if (pressed)
		*v = !*v;

	float& anim = anims[id];
	float target = *v ? 1.0f : 0.0f;
	anim += (target - anim) * ImClamp(ImGui::GetIO().DeltaTime * 12.0f, 0.0f, 1.0f);

	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImU32 bg = LerpColor(IM_COL32(22, 26, 24, 255), IM_COL32(8, 45, 26, 255), anim);
	dl->AddRectFilled(pos, pos + size, bg, 11.0f);
	if (anim > 0.02f) {
		int ga = (int)(anim * (70.0f + pulse * 40.0f));
		dl->AddRect(pos - ImVec2(1.5f, 1.5f), pos + size + ImVec2(1.5f, 1.5f),
			(accent & 0x00FFFFFF) | ((ImU32)ga << 24), 12.0f, ImDrawFlags_RoundCornersAll, 1.0f);
	}
	dl->AddRect(pos, pos + size, LerpColor(IM_COL32(60, 70, 64, 255), accent, anim), 11.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	float knobR = size.y * 0.5f - 3.0f;
	ImVec2 knobC = pos + ImVec2(knobR + 3.0f + anim * (size.x - size.y), size.y * 0.5f);
	dl->AddCircleFilled(knobC, knobR, LerpColor(IM_COL32(150, 160, 154, 255), accent, anim));
	dl->AddCircle(knobC, knobR, (accent & 0x00FFFFFF) | (120u << 24), 32, 1.0f);

	return pressed;
}

// Track de slider (generico para int/float)
template<typename T>
static bool SliderTrack(ImDrawList* dl, const char* sliderId, const ImVec2& pos, float width, T* v, T vmin, T vmax, ImU32 accent, float pulse)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiID id = window->GetID(sliderId);
	ImRect bb(pos, pos + ImVec2(width, 18));
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id)) return false;

	bool hov, held;
	ImGui::ButtonBehavior(bb, id, &hov, &held);

	bool changed = false;
	if (held) {
		float t = ImClamp((ImGui::GetIO().MousePos.x - bb.Min.x) / (bb.Max.x - bb.Min.x), 0.0f, 1.0f);
		*v = (T)(vmin + t * (vmax - vmin));
		changed = true;
	}

	float t = ImClamp((double)(*v - vmin) / (double)(vmax - vmin), 0.0, 1.0);

	dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 24, 22, 255), 9.0f);
	if (t > 0.01f) {
		ImVec2 fillMax(bb.Min.x + (bb.Max.x - bb.Min.x) * (float)t, bb.Max.y);
		dl->AddRectFilled(bb.Min, fillMax, LerpColor(IM_COL32(20, 24, 22, 255), accent, 0.9f), 9.0f);
		dl->AddRectFilled(bb.Min + ImVec2(0, -1.5f), fillMax + ImVec2(0, 1.5f), (accent & 0x00FFFFFF) | (40u << 24), 9.0f);
	}
	dl->AddRect(bb.Min, bb.Max, IM_COL32(60, 70, 64, 255), 9.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	ImVec2 knobC(bb.Min.x + (bb.Max.x - bb.Min.x) * (float)t, bb.Min.y + bb.GetHeight() * 0.5f);
	dl->AddCircleFilled(knobC, 6.5f, accent);
	dl->AddCircle(knobC, 9.5f, (accent & 0x00FFFFFF) | ((ImU32)(int)(50.0f + pulse * 45.0f) << 24), 32, 1.0f);

	return changed;
}

// ============================================================================
// COMPONENTES DE FILAS (dentro de las pestañas)
// ============================================================================

// Titulo de seccion con barra de acento
static void SectionTitle(ImDrawList* dl, const char* title)
{
	ImGui::Dummy(ImVec2(0, 6));
	ImVec2 p = ImGui::GetCursorScreenPos();
	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	dl->AddText(p, COL_TEXT_MUTED, title);
	if (Fonts::InterBold12) ImGui::PopFont();
	dl->AddRectFilled(p + ImVec2(0, 17), p + ImVec2(ImGui::CalcTextSize(title).x + 4, 18), COL_GREEN_DIM, 1.0f);
	ImGui::Dummy(ImVec2(0, 25));
}

// Fila con toggle a la derecha
static bool RowToggle(ImDrawList* dl, const char* switchId, const char* label, const char* sub, bool* v, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
	ImVec2 p0 = p + ImVec2(2, 0), p1 = p + ImVec2(w - 2, 44);

	if (ImGui::IsMouseHoveringRect(p0, p1)) {
		dl->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 5), 6.0f);
	}

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 6), *v ? COL_GREEN : COL_TEXT_MAIN, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	if (sub && Fonts::InterRegular14) {
		ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(12, 26), COL_TEXT_DIM, sub);
		ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, 44));
	return ToggleSwitch(switchId, p + ImVec2(w - 16 - 46, 11), ImVec2(46, 22), v, accent, pulse);
}

// Fila con slider
template<typename T>
static void SliderRow(ImDrawList* dl, const char* sliderId, const char* label, const char* fmt, T* v, T vmin, T vmax, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), COL_TEXT_MAIN, label);
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
// SELECTOR DE TIPO (TOP / BOTTOM) para la posicion de la linea del ESP
// ============================================================================
static void TypeSelector(ImDrawList* dl, const char* id, const char* label, int* v, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), COL_TEXT_MAIN, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	const float h = 30.0f;
	const float sw = (w - 24 - 6) * 0.5f;
	const ImVec2 b0(p.x + 12, p.y + 28);
	const char* opts[2] = { "TOP", "BOTTOM" };

	for (int i = 0; i < 2; i++) {
		ImRect r(b0 + ImVec2(i * (sw + 6), 0), b0 + ImVec2(i * (sw + 6) + sw, h));
		const bool active = (*v == (i ? 2 : 1));
		const bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
		if (hov && !active) dl->AddRectFilled(r.Min, r.Max, IM_COL32(255, 255, 255, 5), 6.0f);
		dl->AddRectFilled(r.Min, r.Max, active ? ((accent & 0x00FFFFFF) | (30u << 24)) : IM_COL32(20, 24, 22, 255), 6.0f);
		dl->AddRect(r.Min, r.Max, active ? accent : IM_COL32(60, 70, 64, 255), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f);

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
		dl->AddText(r.Min + ImVec2((r.GetWidth() - ts.x) * 0.5f, (r.GetHeight() - ts.y) * 0.5f), active ? accent : COL_TEXT_DIM, opts[i]);
		if (Fonts::InterBold12) ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, h + 34));
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

	if (ImGui::IsMouseHoveringRect(p0, p1)) {
		dl->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 5), 6.0f);
	}

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 10), COL_TEXT_MAIN, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	ImVec2 sw = p + ImVec2(w - 16 - 30, 9);
	dl->AddRectFilled(sw, sw + ImVec2(30, 22), Col4(col), 4.0f);
	dl->AddRect(sw, sw + ImVec2(30, 22), IM_COL32(60, 70, 64, 255), 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);

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
	dl->AddText(p + ImVec2(12, 0), COL_GREEN_DIM, group);
	if (Fonts::InterBold12) ImGui::PopFont();
	dl->AddRectFilled(p + ImVec2(12, 15), p + ImVec2(12 + ImGui::CalcTextSize(group).x + 2, 16), COL_GREEN_DIM, 1.0f);
	ImGui::Dummy(ImVec2(0, 22));

	ImGui::PushID(group);
	RowColor(dl, "##L", "Linea", gc->Line, COL_GREEN, pulse, width);
	RowColor(dl, "##B", "Caja", gc->Box, COL_GREEN, pulse, width);
	RowColor(dl, "##F", "Relleno", gc->Fill, COL_GREEN, pulse, width);
	ImGui::PopID();
}

// ============================================================================
// TARJETA DE MODO (cabecera con icono, titulo, descripcion y estado)
// ============================================================================
static void ModeCardHeader(ImDrawList* dl, const char* title, const char* desc, const char* glyph, bool* enabled, float pulse, float width)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	const float h = 64.0f;
	ImU32 accent = *enabled ? COL_GREEN : COL_TEXT_DIM;

	dl->AddRectFilled(p, p + ImVec2(width, h), *enabled ? IM_COL32(10, 26, 17, 255) : COL_BG_CARD, 10.0f);
	dl->AddRect(p, p + ImVec2(width, h), *enabled ? ((COL_GREEN & 0x00FFFFFF) | (80u << 24)) : COL_BORDER, 10.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	if (*enabled) {
		int ga = 120 + (int)(pulse * 60.0f);
		dl->AddRectFilled(p + ImVec2(0, 12), p + ImVec2(3.0f, h - 12), (COL_GREEN & 0x00FFFFFF) | ((ImU32)ga << 24), 1.5f);
	}

	DrawIcon(dl, p + ImVec2(26, h * 0.5f), 12.0f, glyph, accent, Fonts::FontAwesomeSolid);

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(52, 12), *enabled ? COL_TEXT_MAIN : COL_TEXT_DIM, title);
	if (Fonts::InterSemiBold) ImGui::PopFont();
	if (Fonts::InterRegular14) ImGui::PushFont(Fonts::InterRegular14);
	dl->AddText(p + ImVec2(52, 34), COL_TEXT_MUTED, desc);
	if (Fonts::InterRegular14) ImGui::PopFont();

	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	const char* st = *enabled ? "ON" : "OFF";
	ImVec2 ss = ImGui::CalcTextSize(st);
	dl->AddText(p + ImVec2(width - 16 - ss.x, 24), *enabled ? COL_GREEN : COL_TEXT_MUTED, st);
	if (Fonts::InterBold12) ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, h + 10));
}

// ============================================================================
// NOMBRE LEGIBLE DE UNA TECLA VK
// ============================================================================
static const char* KeyName(int vk)
{
	switch (vk) {
	case VK_LBUTTON: return "CLICK IZQ";
	case VK_RBUTTON: return "CLICK DER";
	case VK_MBUTTON: return "RUEDA";
	case VK_INSERT:  return "INSERT";
	case VK_SPACE:   return "SPACE";
	case VK_SHIFT:   return "SHIFT";
	case VK_CONTROL: return "CTRL";
	case VK_MENU:    return "ALT";
	case VK_TAB:     return "TAB";
	case VK_ESCAPE:  return "ESC";
	case VK_RETURN:  return "ENTER";
	case VK_BACK:    return "SUPR";
	case VK_UP:      return "ARRIBA";
	case VK_DOWN:    return "ABAJO";
	case VK_LEFT:    return "IZQUIERDA";
	case VK_RIGHT:   return "DERECHA";
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
// FILA CON CAPTURA DE TECLA (badge + captura por polling)
// ============================================================================
static bool KeyBindRow(ImDrawList* dl, const char* rowId, const char* label, const char* sub, int* v, ImU32 accent, float pulse, float width = 0.0f)
{
	static std::map<ImGuiID, bool> capturing;
	static std::map<ImGuiID, float> captureStart;

	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
	ImVec2 p0 = p + ImVec2(2, 0), p1 = p + ImVec2(w - 2, 44);

	if (ImGui::IsMouseHoveringRect(p0, p1))
		dl->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 5), 6.0f);

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 6), COL_TEXT_MAIN, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	if (sub && Fonts::InterRegular14) {
		ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(12, 26), COL_TEXT_DIM, sub);
		ImGui::PopFont();
	}

	ImGui::PushID(rowId);
	ImGuiID rid = ImGui::GetID("##key");
	const bool cap = capturing[rid];
	float& capT = captureStart[rid];

	const ImVec2 bw = p + ImVec2(w - 16 - 74, 9);
	const ImVec2 bs(74, 26);
	const bool hovBadge = ImGui::IsMouseHoveringRect(bw, bw + bs);
	dl->AddRectFilled(bw, bw + bs, cap ? IM_COL32(10, 26, 17, 255) : IM_COL32(20, 24, 22, 255), 6.0f);
	dl->AddRect(bw, bw + bs, cap ? accent : (hovBadge ? IM_COL32(90, 100, 94, 255) : IM_COL32(60, 70, 64, 255)), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
	const char* kn = cap ? "..." : KeyName(*v);
	ImVec2 ks = ImGui::CalcTextSize(kn);
	dl->AddText(bw + ImVec2((bs.x - ks.x) * 0.5f, (bs.y - ks.y) * 0.5f - 1.0f), cap ? accent : COL_TEXT_MAIN, kn);
	if (Fonts::InterBold12) ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, 44));

	if (cap) {
		// Pulsar el badge otra vez cancela la captura
		if (ImGui::IsMouseClicked(0) && hovBadge) {
			capturing[rid] = false;
		}
		else {
			float now = (float)GetTickCount64() / 1000.0f;
			for (int key = 0x01; key < 0xFE; key++) {
				if (key == VK_LSHIFT || key == VK_RSHIFT || key == VK_LCONTROL ||
					key == VK_RCONTROL || key == VK_LMENU || key == VK_RMENU ||
					key == VK_LWIN || key == VK_RWIN) continue;
				// Botones del raton: ignorar sobre el badge o justo tras activar
				if (key >= VK_LBUTTON && key <= VK_MBUTTON && (hovBadge || now - capT < 0.25f)) continue;
				if (GetAsyncKeyState(key) & 0x8000) {
					*v = key;
					capturing[rid] = false;
					break;
				}
			}
		}
	}
	else if (ImGui::IsMouseClicked(0) && hovBadge) {
		capturing[rid] = true;
		capT = (float)GetTickCount64() / 1000.0f;
	}

	ImGui::PopID();
	return cap;
}

// ============================================================================
// SELECTOR DE HUESO OBJETIVO (HEAD / NECK / HIP)
// ============================================================================
static void BoneSelector(ImDrawList* dl, const char* id, const char* label, Config::Bone* v, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), COL_TEXT_MAIN, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	const float h = 30.0f;
	const float sw = (w - 24 - 12) / 3.0f;
	const ImVec2 b0(p.x + 12, p.y + 28);
	const char* opts[3] = { "HEAD", "NECK", "HIP" };

	for (int i = 0; i < 3; i++) {
		ImRect r(b0 + ImVec2(i * (sw + 6), 0), b0 + ImVec2(i * (sw + 6) + sw, h));
		const bool active = ((int)*v == i);
		const bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
		if (hov && !active) dl->AddRectFilled(r.Min, r.Max, IM_COL32(255, 255, 255, 5), 6.0f);
		dl->AddRectFilled(r.Min, r.Max, active ? ((accent & 0x00FFFFFF) | (30u << 24)) : IM_COL32(20, 24, 22, 255), 6.0f);
		dl->AddRect(r.Min, r.Max, active ? accent : IM_COL32(60, 70, 64, 255), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f);

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
		dl->AddText(r.Min + ImVec2((r.GetWidth() - ts.x) * 0.5f, (r.GetHeight() - ts.y) * 0.5f), active ? accent : COL_TEXT_DIM, opts[i]);
		if (Fonts::InterBold12) ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, h + 34));
}

// ============================================================================
// SELECTOR DE 3 OPCIONES (HEAD / SPINE / ROOT, etc.)
// ============================================================================
static void TriSelector(ImDrawList* dl, const char* id, const char* label, int* v,
    const char* optA, const char* optB, const char* optC, ImU32 accent, float pulse, float width = 0.0f)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(p + ImVec2(12, 8), COL_TEXT_MAIN, label);
	if (Fonts::InterSemiBold) ImGui::PopFont();

	const float h = 30.0f;
	const float sw = (w - 24 - 12) / 3.0f;
	const ImVec2 b0(p.x + 12, p.y + 28);
	const char* opts[3] = { optA, optB, optC };

	for (int i = 0; i < 3; i++) {
		ImRect r(b0 + ImVec2(i * (sw + 6), 0), b0 + ImVec2(i * (sw + 6) + sw, h));
		const bool active = (*v == i);
		const bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
		if (hov && !active) dl->AddRectFilled(r.Min, r.Max, IM_COL32(255, 255, 255, 5), 6.0f);
		dl->AddRectFilled(r.Min, r.Max, active ? ((accent & 0x00FFFFFF) | (30u << 24)) : IM_COL32(20, 24, 22, 255), 6.0f);
		dl->AddRect(r.Min, r.Max, active ? accent : IM_COL32(60, 70, 64, 255), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f);

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
		dl->AddText(r.Min + ImVec2((r.GetWidth() - ts.x) * 0.5f, (r.GetHeight() - ts.y) * 0.5f), active ? accent : COL_TEXT_DIM, opts[i]);
		if (Fonts::InterBold12) ImGui::PopFont();
	}

	ImGui::Dummy(ImVec2(0, h + 34));
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
        dl->AddRectFilled(p0, p1, IM_COL32(255, 255, 255, 5), 6.0f);

    if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
    dl->AddText(p + ImVec2(12, 6), COL_TEXT_MAIN, label);
    if (Fonts::InterSemiBold) ImGui::PopFont();

    if (sub && Fonts::InterRegular14) {
        ImGui::PushFont(Fonts::InterRegular14);
        dl->AddText(p + ImVec2(12, 26), COL_TEXT_DIM, sub);
        ImGui::PopFont();
    }

    const ImVec2 bw = p + ImVec2(w - 16 - 140, 9);
    const ImVec2 bs(140, 26);
    const bool hovBadge = ImGui::IsMouseHoveringRect(bw, bw + bs);
    dl->AddRectFilled(bw, bw + bs, IM_COL32(20, 24, 22, 255), 6.0f);
    dl->AddRect(bw, bw + bs, hovBadge ? accent : IM_COL32(60, 70, 64, 255), 6.0f, ImDrawFlags_RoundCornersAll, 1.0f);

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
    dl->AddText(bw + ImVec2((bs.x - ws.x) * 0.5f, (bs.y - ws.y) * 0.5f - 1.0f), COL_TEXT_MAIN, badge);
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
        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(18, 22, 20, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(20, 24, 22, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(30, 36, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_MAIN);
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
// PESTAÑA BOOST: mejora de armas (fire rate por nivel) + mini uzi speed
// ============================================================================
static void BoostTab(ImDrawList* dl, float pulse)
{
	(void)dl;
	float availW = ImGui::GetContentRegionAvail().x;
	const float colGap = 32.0f;
	const float colW = (availW - colGap) * 0.5f;
	const float availH = ImGui::GetContentRegionAvail().y;

	// ==================== COLUMNA IZQUIERDA: MEJORA DE ARMAS ====================
	ImGui::BeginChild("##BstLeft", ImVec2(colW, availH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* ldl = ImGui::GetWindowDrawList();

		SectionTitle(ldl, "MEJORA DE ARMAS");
		RowToggle(ldl, "##BstWa", "Mejora de Armas", "Fire rate por nivel de arma",
			&g_Globals.Exploits.WeaponAttributes, COL_GREEN, pulse);
		SliderRow(ldl, "##BstWaL", "Nivel de arma", "LV%d",
			&g_Globals.Exploits.WeaponLevel, 0, 3, COL_GREEN, pulse);
		WeaponSelectorRow(ldl, "##BstWaW", "Arma a mejorar", "Click para cambiar",
			&g_Globals.Exploits.BoostWeaponId, COL_GREEN, pulse);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// ==================== COLUMNA DERECHA: MINI UZI ====================
	ImGui::BeginChild("##BstRight", ImVec2(colW, availH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* rdl = ImGui::GetWindowDrawList();

		SectionTitle(rdl, "MINI UZI SPEED");
		RowToggle(rdl, "##BstMu", "Mini Uzi Speed", "Speed hack solo con Mini Uzi",
			&g_Globals.Exploits.MiniUziSpeed, COL_GREEN, pulse);
		SliderRow(rdl, "##BstMuM", "Multiplicador", "%.2fx",
			&g_Globals.Exploits.MiniUziSpeedMultiplier, 1.05f, 1.5f, COL_GREEN, pulse);
	}
	ImGui::EndChild();
}

// ============================================================================
// PESTAÑA EXPLOITS
// ============================================================================
static void AimbotTab(ImDrawList* dl, float pulse)
{
	(void)dl;
	float availW = ImGui::GetContentRegionAvail().x;
	const float colGap = 16.0f;
	const float colW = (availW - colGap * 2.0f) / 3.0f;
	const float colH = ImGui::GetContentRegionAvail().y;

	// ==================== COLUMNA IZQUIERDA: MEMORY AIM ====================
	ImGui::BeginChild("##AimLeft", ImVec2(colW, colH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* ldl = ImGui::GetWindowDrawList();

		ModeCardHeader(ldl, "MEMORY AIM", "Escritura de rotacion en memoria", "\uF05B",
			&g_Globals.AimBot.MemoryAim, pulse, colW);
		RowToggle(ldl, "##AimMa", "Aimbot Memoria", "Rota la camara hacia el enemigo",
			&g_Globals.AimBot.MemoryAim, COL_GREEN, pulse);
		KeyBindRow(ldl, "##AimMaK", "Tecla de activacion", "Mantener pulsada para aimear",
			&g_Globals.AimBot.AimbotBind, COL_GREEN, pulse);
		BoneSelector(ldl, "##AimMaB", "TARGET (BONE)", &g_Globals.AimBot.TargetBone, COL_GREEN, pulse);
		// FOV unico compartido (lo usan los tres modos)
		SliderRow(ldl, "##AimFov", "FOV (todos)", "%d px",
			&g_Globals.AimBot.DistanceAim, 50, 500, COL_GREEN, pulse);
		RowToggle(ldl, "##AimFv", "Mostrar FOV", "Circulo del radio en pantalla",
			&g_Globals.Misc.ShowAimbotFov, COL_GREEN, pulse);
		RowToggle(ldl, "##AimMaEn", "Solo Enemigos", "No aimear a aliados",
			&g_Globals.AimBot.OnlyEnemies, COL_GREEN, pulse);
		RowToggle(ldl, "##AimMaKn", "Ignorar Derribados", "No aimear a jugadores knocked",
			&g_Globals.AimBot.IgnoreKnocked, COL_GREEN, pulse);
		RowToggle(ldl, "##AimMaBt", "Ignorar Bots", "No aimear a bots",
			&g_Globals.AimBot.IgnoreBots, COL_GREEN, pulse);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// ==================== COLUMNA CENTRAL: SILENT AIM ====================
	ImGui::BeginChild("##AimMid", ImVec2(colW, colH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* mdl = ImGui::GetWindowDrawList();

		ModeCardHeader(mdl, "SILENT AIM", "Redirige el proyectil al target", "\uF1D8",
			&g_Globals.Silent.Enabled, pulse, colW);
		RowToggle(mdl, "##AimSi", "Silent Aim", "Activo manteniendo el click izquierdo",
			&g_Globals.Silent.Enabled, COL_GREEN, pulse);
		BoneSelector(mdl, "##AimSiB", "TARGET (BONE)", &g_Globals.Silent.TargetBone, COL_GREEN, pulse);
		SliderRow(mdl, "##AimSiSpd", "Velocidad de Bala", "%.0f m/s",
			&g_Globals.Silent.BulletSpeed, 200.0f, 2000.0f, COL_GREEN, pulse);
		if (Fonts::InterRegular14) ImGui::PushFont(Fonts::InterRegular14);
		ImVec2 cp = ImGui::GetCursorScreenPos();
		mdl->AddText(cp + ImVec2(12, 0), COL_TEXT_MUTED, "Elige el hueso del target. La velocidad de bala predice el movimiento del objetivo");
		if (Fonts::InterRegular14) ImGui::PopFont();
		ImGui::Dummy(ImVec2(0, 24));
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// ==================== COLUMNA DERECHA: RAGE AIM ====================
	ImGui::BeginChild("##AimRight", ImVec2(colW, colH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* rdl = ImGui::GetWindowDrawList();

		ModeCardHeader(rdl, "RAGE AIM", "Fija el collider del enemigo", "\uF140",
			&g_Globals.AimBot.RageAim, pulse, colW);
		RowToggle(rdl, "##AimRa", "Rage Aimbot", "Se activa con CLICK IZQUIERDO",
			&g_Globals.AimBot.RageAim, COL_GREEN, pulse);
	}
	ImGui::EndChild();
}

static void EspTab(ImDrawList* dl, float pulse)
{
	(void)dl;
	float availW = ImGui::GetContentRegionAvail().x;
	const float colGap = 32.0f;
	const float colW = (availW - colGap) * 0.5f;
	const float availH = ImGui::GetContentRegionAvail().y;

	// ==================== COLUMNA IZQUIERDA: SETTINGS (scroll propio) ====================
	ImGui::BeginChild("##EspLeft", ImVec2(colW, availH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* ldl = ImGui::GetWindowDrawList();

		SectionTitle(ldl, "SETTINGS");
		RowToggle(ldl, "##EspGlow", "Glow", "Halo resaltado en el ESP", &g_Globals.Visuals.Glow, COL_GREEN, pulse);
		SliderRow(ldl, "##SlGlow", "Intensidad", "%d %%", &g_Globals.Visuals.GlowIntensity, 0, 100, COL_GREEN, pulse);

		SectionTitle(ldl, "POSICION");
		TypeSelector(ldl, "##TpEsp", "TYPE", &g_Globals.Visuals.EspLines, COL_GREEN, pulse);

		SectionTitle(ldl, "FILTRO");
		RowToggle(ldl, "##EspFb", "Ignorar Bots", "No mostrar bots", &g_Globals.Visuals.IgnoreBots, COL_GREEN, pulse);
		RowToggle(ldl, "##EspFk", "Ignorar Derribados", "Ocultar jugadores knocked", &g_Globals.Visuals.IgnoreKnocked, COL_GREEN, pulse);
		RowToggle(ldl, "##EspFv", "Solo Visibles", "Mostrar solo enemigos visibles", &g_Globals.Visuals.OnlyVisible, COL_GREEN, pulse);

		SectionTitle(ldl, "COLORS");
		GroupColors(ldl, "BOTS", &g_Globals.Visuals.ColBots, pulse, 0.0f);
		GroupColors(ldl, "VISIBLES", &g_Globals.Visuals.ColVisible, pulse, 0.0f);
		GroupColors(ldl, "KNOCKED", &g_Globals.Visuals.ColKnocked, pulse, 0.0f);
		GroupColors(ldl, "NORMAL", &g_Globals.Visuals.ColNormal, pulse, 0.0f);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// ==================== COLUMNA DERECHA: VISUALES (scroll propio) ====================
	ImGui::BeginChild("##EspRight", ImVec2(colW, availH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* rdl = ImGui::GetWindowDrawList();

		SectionTitle(rdl, "VISUALES");
		if (RowToggle(rdl, "##EspLine", "ESP Line", "Linea hacia todos los enemigos", &g_Globals.Visuals.Lines, COL_GREEN, pulse)) {
			g_Globals.Visuals.EspLines = g_Globals.Visuals.Lines ? 1 : 0;
			if (g_Globals.Visuals.Lines) {
				g_Globals.Visuals.Enable = true;
			}
		}
		RowToggle(rdl, "##EspBox", "Caja", "Caja alrededor del enemigo", &g_Globals.Visuals.Box, COL_GREEN, pulse);
		RowToggle(rdl, "##EspFill", "Caja Rellena", "Relleno semi transparente", &g_Globals.Visuals.FilledBox, COL_GREEN, pulse);
		RowToggle(rdl, "##EspHb", "Barra de Vida", "Barra de salud en pantalla", &g_Globals.Visuals.HealthBar, COL_GREEN, pulse);
		RowToggle(rdl, "##EspNm", "Nombre", "Nombre del enemigo", &g_Globals.Visuals.Name, COL_GREEN, pulse);
		RowToggle(rdl, "##EspDs", "Distancia", "Distancia al enemigo", &g_Globals.Visuals.Distance, COL_GREEN, pulse);
		RowToggle(rdl, "##EspMm", "Minimapa", "Radar en pantalla", &g_Globals.Visuals.Minimap, COL_GREEN, pulse);
		RowToggle(rdl, "##EspWm", "Marca de Agua", "Logo ASMODEUS en pantalla", &g_Globals.Visuals.Watermark, COL_GREEN, pulse);
		RowToggle(rdl, "##EspSk", "Esqueleto", "Huesos de los enemigos", &g_Globals.Visuals.Skeleton, COL_GREEN, pulse);
		RowToggle(rdl, "##EspSkL", "Mi Esqueleto", "Huesos de tu personaje", &g_Globals.Visuals.LocalSkeleton, COL_GREEN, pulse);
		RowColor(rdl, "##EspSkC", "Color Esqueleto", g_Globals.Visuals.SkeletonColor, COL_GREEN, pulse);

		SectionTitle(rdl, "RANGO");
		SliderRow(rdl, "##SlEsp", "Alcance del ESP", "%d m", &g_Globals.Visuals.DistanceEsp, 50, 500, COL_GREEN, pulse);
	}
	ImGui::EndChild();
}

static void ExploitsTab(ImDrawList* dl, float pulse)
{
	(void)dl;
	float availW = ImGui::GetContentRegionAvail().x;
	const float colGap = 32.0f;
	const float colW = (availW - colGap) * 0.5f;
	const float availH = ImGui::GetContentRegionAvail().y;

	// ==================== COLUMNA IZQUIERDA: EXPLOITS ====================
	ImGui::BeginChild("##ExpLeft", ImVec2(colW, availH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* ldl = ImGui::GetWindowDrawList();

		SectionTitle(ldl, "EXPLOITS");
		RowToggle(ldl, "##ExFs", "Fast Switch", "Cambio de arma instantaneo",
			&g_Globals.Exploits.FastSwitch, COL_GREEN, pulse);
		RowToggle(ldl, "##ExNr", "Sin Retroceso", "Elimina el retroceso del arma",
			&g_Globals.Exploits.NoRecoil, COL_GREEN, pulse);
		RowToggle(ldl, "##ExNl", "Sin Recarga", "No necesitas recargar el arma",
			&g_Globals.Exploits.NoReload, COL_GREEN, pulse);
		RowToggle(ldl, "##ExSh", "Speed Hack", "Corre mas rapido",
			&g_Globals.Exploits.SpeedHack, COL_GREEN, pulse);
		SliderRow(ldl, "##ExShM", "Velocidad carrera", "%.2fx",
			&g_Globals.Exploits.SpeedHackMultiplier, 1.0f, 2.0f, COL_GREEN, pulse);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// ==================== COLUMNA DERECHA: TELE KILL ====================
	ImGui::BeginChild("##ExpRight", ImVec2(colW, availH), false, ImGuiWindowFlags_NoBackground);
	{
		ImDrawList* rdl = ImGui::GetWindowDrawList();

		ModeCardHeader(rdl, "TELE KILL", "Teleporta al enemigo mas cercano", "\uF1D8",
			&g_Globals.Exploits.TeleKill, pulse, colW);
		RowToggle(rdl, "##ExTk", "Tele Kill", "Acerca al enemigo a tu posicion",
			&g_Globals.Exploits.TeleKill, COL_GREEN, pulse);
		KeyBindRow(rdl, "##ExTkK", "Tecla (toggle)", "Presiona para activar/desactivar",
			&g_Globals.Exploits.TeleKillKey, COL_GREEN, pulse);
		SliderRow(rdl, "##ExTkD", "Distancia de uso", "%0.1f m",
			&g_Globals.Exploits.TeleKillDistance, 1.0f, 50.0f, COL_GREEN, pulse);
		RowToggle(rdl, "##ExSt", "Speed Timer", "Acelera el tiempo y tu velocidad",
			&g_Globals.Exploits.SpeedTimer, COL_GREEN, pulse);
		SliderRow(rdl, "##ExStM", "Velocidad", "%.1fx",
			&g_Globals.Exploits.SpeedMultiplier, 1.0f, 5.0f, COL_GREEN, pulse);
		RowToggle(rdl, "##ExUp", "Under Player", "Hundete 0.7m bajo el suelo",
			&g_Globals.Exploits.UnderPlayer, COL_GREEN, pulse);
		KeyBindRow(rdl, "##ExUpK", "Tecla volver", "Vuelve a tu posicion original",
			&g_Globals.Exploits.UnderPlayerKey, COL_GREEN, pulse);
		RowToggle(rdl, "##ExPp", "Pull Player", "Jala al enemigo al disparar",
			&g_Globals.Exploits.PullPlayer, COL_GREEN, pulse);
		KeyBindRow(rdl, "##ExPpK", "Tecla pull", "Mantener para jalar sin disparar",
			&g_Globals.Exploits.PullPlayerKey, COL_GREEN, pulse);
		TriSelector(rdl, "##ExPpB", "Hueso", &g_Globals.Exploits.PullBone,
			"HEAD", "SPINE", "ROOT", COL_GREEN, pulse);
		SliderRow(rdl, "##ExPpD", "Alcance", "%0.1f m",
			&g_Globals.Exploits.PullDistance, 1.0f, 50.0f, COL_GREEN, pulse);
		SliderRow(rdl, "##ExPpS", "Suavidad", "%.1fx",
			&g_Globals.Exploits.PullSmooth, 0.5f, 4.0f, COL_GREEN, pulse);
	}
	ImGui::EndChild();
}

static void ConfigTab(ImDrawList* dl, float pulse)
{
	SectionTitle(dl, "CONEXION ADB");
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		float w = ImGui::GetContentRegionAvail().x;

		dl->AddRectFilled(p, p + ImVec2(w, 60), COL_BG_CARD, 8.0f);
		dl->AddRect(p, p + ImVec2(w, 60), COL_BORDER, 8.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		ImU32 bar = (COL_GREEN & 0x00FFFFFF) | ((ImU32)(120 + (int)(pulse * 60.0f)) << 24);
		dl->AddRectFilled(p + ImVec2(10, 0), p + ImVec2(56, 2.0f), bar, 1.0f);
		dl->AddRectFilled(p + ImVec2(10, 0), p + ImVec2(56, 4.5f), (COL_GREEN & 0x00FFFFFF) | (26u << 24), 1.0f);

		DrawIcon(dl, p + ImVec2(25, 29), 13.0f, "\uF1E6", COL_GREEN, Fonts::FontAwesomeSolid);
		if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
		dl->AddText(p + ImVec2(48, 8), COL_TEXT_MAIN, "CONEXION ADB");
		if (Fonts::InterSemiBold) ImGui::PopFont();
		if (Fonts::InterRegular14) ImGui::PushFont(Fonts::InterRegular14);
		dl->AddText(p + ImVec2(48, 30), COL_TEXT_DIM, "BlueStacks Emulator");
		if (Fonts::InterRegular14) ImGui::PopFont();

		const char* btnLabel;
		ImU32 btnColor;
		bool btnEnabled;
		switch (g_AdbState) {
		case AdbState::Connected:   btnLabel = "CONECTADO";   btnColor = COL_GREEN;  btnEnabled = false; break;
		case AdbState::Connecting:  btnLabel = "CONECTANDO";  btnColor = COL_YELLOW; btnEnabled = false; break;
		case AdbState::Failed:      btnLabel = "RECONECTAR";  btnColor = COL_RED;    btnEnabled = true;  break;
		default:                    btnLabel = "CONECTAR";    btnColor = COL_GREEN;  btnEnabled = true;  break;
		}
		if (PremiumButton(p + ImVec2(w - 16 - 110, 13), ImVec2(110, 34), btnLabel, btnColor, btnEnabled, pulse)) {
			ConnectAdb();
		}

		ImGui::Dummy(ImVec2(0, 70));
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

	Style->WindowRounding = 12.0f;
	Style->FrameRounding = 6.0f;
	Style->GrabRounding = 6.0f;
	Style->GrabMinSize = 4.0f;
	Style->WindowBorderSize = 0.0f;
	Style->FrameBorderSize = 1.0f;
	Style->WindowPadding = ImVec2(12, 12);
	Style->ItemSpacing = ImVec2(8, 8);
	Style->ScrollbarSize = 2.0f;

	Style->Colors[ImGuiCol_WindowBg] = ImColor(COL_BG_WINDOW);
	Style->Colors[ImGuiCol_ChildBg] = ImColor(COL_BG_CARD);
	Style->Colors[ImGuiCol_Border] = ImColor(COL_BORDER);
	Style->Colors[ImGuiCol_Text] = ImColor(COL_TEXT_MAIN);
	Style->Colors[ImGuiCol_TextSelectedBg] = ImColor(COL_GREEN_SOFT);

	Style->Colors[ImGuiCol_FrameBg] = ImColor(COL_BG_CARD);
	Style->Colors[ImGuiCol_FrameBgHovered] = ImColor(24, 28, 26, 255);
	Style->Colors[ImGuiCol_FrameBgActive] = ImColor(30, 34, 32, 255);
	Style->Colors[ImGuiCol_CheckMark] = ImColor(COL_GREEN);
}

// ============================================================================
// RENDERIZADO PRINCIPAL (4 PESTAÑAS: AIMBOT / ESP / EXPLOITS / CONFIG)
// ============================================================================

static void DrawTabContent(ImDrawList* cdl, int tab, float pulse)
{
	switch (tab) {
	case 0: AimbotTab(cdl, pulse); break;
	case 1: EspTab(cdl, pulse); break;
	case 2: ExploitsTab(cdl, pulse); break;
	case 3: BoostTab(cdl, pulse); break;
	case 4: ConfigTab(cdl, pulse); break;
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

	ImGui::SetNextWindowSize(ImVec2(1000.0f, 650.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_BG_WINDOW);

	ImGui::Begin("##MainWindow", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImVec2 Pos = ImGui::GetWindowPos();
		ImVec2 Size = ImGui::GetWindowSize();

		// ====================================================================
		// SOMBRA EXTERIOR
		// ====================================================================
		DrawShadow(dl, Pos, Pos + Size, 12.0f);

		// ====================================================================
		// MARCO CON GLOW RESPIRADO
		// ====================================================================
		int glowA = 90 + (int)(pulse * 70.0f);
		dl->AddRect(Pos - ImVec2(1, 1), Pos + Size + ImVec2(1, 1), IM_COL32(10, 255, 130, glowA / 3), 12.0f, ImDrawFlags_RoundCornersAll, 1.2f);
		dl->AddRect(Pos - ImVec2(3, 3), Pos + Size + ImVec2(3, 3), IM_COL32(10, 255, 130, glowA / 5), 12.0f, ImDrawFlags_RoundCornersAll, 1.0f);
		dl->AddRect(Pos + ImVec2(1.5f, 1.5f), Pos + Size - ImVec2(1.5f, 1.5f), IM_COL32(40, 50, 44, 255), 10.0f, ImDrawFlags_RoundCornersAll, 1.0f);

		// ====================================================================
		// HEADER
		// ====================================================================
		dl->AddRectFilledMultiColor(Pos, Pos + ImVec2(Size.x, 50),
			IM_COL32(13, 22, 17, 255), IM_COL32(10, 18, 14, 255),
			IM_COL32(9, 11, 10, 255), IM_COL32(9, 11, 10, 255));

		DrawIcon(dl, Pos + ImVec2(24, 24), 11.0f, "\uF0E7", COL_GREEN, Fonts::FontAwesomeSolid);

		ImVec2 titlePos = Pos + ImVec2(44, 9);
		if (Fonts::GeistRegularMedium) ImGui::PushFont(Fonts::GeistRegularMedium);
		DrawGlowText(dl, titlePos, COL_GREEN, "ASMODEUS");
		if (Fonts::GeistRegularMedium) ImGui::PopFont();

		if (Fonts::InterLight) ImGui::PushFont(Fonts::InterLight);
		dl->AddText(Pos + ImVec2(45, 31), COL_TEXT_DIM, "C O N T R O L   P A N E L");
		if (Fonts::InterLight) ImGui::PopFont();

		DrawPulseDot(dl, Pos + ImVec2(Size.x - 26, 25), AdbStateColor(), pulse);

		// ====================================================================
		// PESTAÑAS
		// ====================================================================
		static const char* tabNames[] = { "AIMBOT", "ESP", "EXPLOITS", "BOOST", "CONFIG" };
		static const char* tabIcons[] = { "\uF05B", "\uF06E", "\uF0E7", "\uE059", "\uF013" };
		static int curTab = 0;
		static int targetTab = -1;      // pestana hacia la que se transiciona
		static int slideDir = 1;        // direccion del deslizamiento (-1 izquierda / +1 derecha)
		static float tabSlide = 1.0f;   // 0 = inicio de la transicion, 1 = asentada
		float tabW = (Size.x - 32) / 5.0f;

		for (int i = 0; i < 5; i++) {
			ImVec2 tp = Pos + ImVec2(16 + i * tabW, 52);
			ImRect r(tp, tp + ImVec2(tabW, 30));
			bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
			bool active = (i == curTab);

			if (hov && !active) {
				dl->AddRectFilled(r.Min, r.Max, IM_COL32(255, 255, 255, 4), 6.0f);
			}

			// Icono + nombre de la pestana (centrados como un bloque)
			float iconW = 0.0f;
			if (Fonts::FontAwesomeSolid) {
				ImGui::PushFont(Fonts::FontAwesomeSolid);
				iconW = ImGui::CalcTextSize(tabIcons[i]).x;
				ImGui::PopFont();
			}
			const float gap = 8.0f;
			ImVec2 ls = ImGui::CalcTextSize(tabNames[i]);
			const float total = iconW + gap + ls.x;
			const float tx = tp.x + (tabW - total) * 0.5f;
			const float ty = tp.y + (30.0f - ls.y) * 0.5f - 1.0f;

			if (Fonts::FontAwesomeSolid) {
				ImGui::PushFont(Fonts::FontAwesomeSolid);
				dl->AddText(ImVec2(tx, ty), active ? COL_GREEN : (hov ? COL_TEXT_MAIN : COL_TEXT_DIM), tabIcons[i]);
				ImGui::PopFont();
			}

			if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
			ImVec2 lpos(tx + iconW + gap, ty);
			dl->AddText(lpos, active ? COL_GREEN : (hov ? COL_TEXT_MAIN : COL_TEXT_DIM), tabNames[i]);
			if (Fonts::InterBold12) ImGui::PopFont();

			if (active && !hov) {
				dl->AddText(lpos + ImVec2(1, 1), IM_COL32(0, 0, 0, 120), tabNames[i]);
				if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
				dl->AddText(lpos, COL_GREEN, tabNames[i]);
				if (Fonts::InterBold12) ImGui::PopFont();
			}

			ImGuiID tid = ImGui::GetID(tabNames[i]);
			ImGui::ItemSize(r);
			if (ImGui::ItemAdd(r, tid)) {
				bool thov, theld;
				// Cambia al presionar (no espera al release) y libera el ActiveId
				// inmediatamente para no bloquear el arrastre del panel
				if (ImGui::ButtonBehavior(r, tid, &thov, &theld,
					ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoHoldingActiveId)) {
					if (i != curTab) {
						targetTab = i;
						slideDir = (i > curTab) ? 1 : -1;
					}
				}
			}

			// Badge de conexion en la pestana CONFIG
			if (i == 4 && g_AdbState == AdbState::Connected) {
				DrawPulseDot(dl, ImVec2(r.Max.x - 16, r.Min.y + 15), COL_GREEN, pulse);
			}
		}

		// Subrayado animado de la pestana activa
		float targetCx = Pos.x + 16 + (curTab + 0.5f) * tabW;
		static float undCx = 0.0f;
		if (undCx == 0.0f) undCx = targetCx;
		undCx += (targetCx - undCx) * ImClamp(dt * 12.0f, 0.0f, 1.0f);
		dl->AddRectFilled(ImVec2(undCx - 18, Pos.y + 81), ImVec2(undCx + 18, Pos.y + 83), COL_GREEN, 1.0f);
		dl->AddRectFilled(ImVec2(undCx - 24, Pos.y + 81.5f), ImVec2(undCx + 24, Pos.y + 82.3f), (COL_GREEN & 0x00FFFFFF) | (50u << 24), 1.0f);

		// ====================================================================
		// SEPARADOR CON BARRIDO
		// ====================================================================
		dl->AddRectFilled(Pos + ImVec2(16, 84), Pos + ImVec2(Size.x - 16, 85.2f), IM_COL32(20, 28, 23, 255));
		dl->AddRectFilled(Pos + ImVec2(16, 84), Pos + ImVec2(16 + (Size.x - 32) * 0.55f, 85.2f), IM_COL32(10, 255, 130, 90));
		DrawSweep(dl, Pos + ImVec2(16, 83.4f), Size.x - 32, fmodf(tGlobal * 0.2f, 1.0f));

		// ====================================================================
		// TRANSICION ENTRE PESTAÑAS: deslizamiento suave del contenido
		// ====================================================================
		if (targetTab != -1) {
			curTab = targetTab;          // cambio inmediato de pestana
			tabSlide = 0.0f;             // y entrada animada del nuevo contenido
			targetTab = -1;
		}
		else if (tabSlide < 1.0f) {
			tabSlide = ImMin(tabSlide + dt * 5.5f, 1.0f);   // ~180 ms
		}

		// ====================================================================
		// CONTENIDO DE LA PESTAÑA ACTIVA (con scroll)
		// ====================================================================
		ImGui::SetCursorScreenPos(Pos + ImVec2(16, 90));
		if (ImGui::BeginChild("##Content", ImVec2(Size.x - 32, Size.y - 90 - 38), false, ImGuiWindowFlags_NoBackground)) {
			ImDrawList* cdl = ImGui::GetWindowDrawList();
			if (tabSlide < 1.0f) {
				// Desliza el contenido entrante desde el lado de la pestana anterior
				float ease = 1.0f - powf(1.0f - tabSlide, 3.0f);   // ease-out cubico
				float off = slideDir * 24.0f * (1.0f - ease);
				ImVec2 origin = ImGui::GetWindowPos();
				cdl->PushClipRect(origin, origin + ImGui::GetWindowSize(), true);
				ImGui::SetCursorScreenPos(origin + ImVec2(off, 0));
				DrawTabContent(cdl, curTab, pulse);
				cdl->PopClipRect();
			}
			else {
				DrawTabContent(cdl, curTab, pulse);
			}
		}
		ImGui::EndChild();

		// ====================================================================
		// FOOTER
		// ====================================================================
		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		dl->AddText(Pos + ImVec2(16, Size.y - 26), COL_GREEN_DIM, "ASMODEUS v1.0");
		if (Fonts::InterBold12) ImGui::PopFont();

		if (Fonts::InterLight) ImGui::PushFont(Fonts::InterLight);
		const char* keyHint = "INSERT  para abrir/cerrar";
		ImVec2 kh = ImGui::CalcTextSize(keyHint);
		dl->AddText(Pos + ImVec2(Size.x - 16 - kh.x, Size.y - 26), COL_TEXT_MUTED, keyHint);
		if (Fonts::InterLight) ImGui::PopFont();

		dl->AddRectFilled(Pos + ImVec2(16, Size.y - 4), Pos + ImVec2(Size.x - 16, Size.y - 3.2f), IM_COL32(10, 255, 130, 40));

		// ====================================================================
		// ARRASTRE DEL PANEL DESDE CUALQUIER PARTE
		// ====================================================================
		// Con el raton pulsado dentro del panel, si se supera el umbral de
		// movimiento se inicia el arrastre (aunque se haya pulsado encima de
		// un tab). No secuestra widgets interactivos pulsados (toggles/sliders):
		// esos mantienen su ActiveId y solo arrastran tras soltar y volver a pulsar.
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
