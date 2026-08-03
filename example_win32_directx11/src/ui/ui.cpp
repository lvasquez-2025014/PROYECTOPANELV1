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
static bool RowToggle(ImDrawList* dl, const char* switchId, const char* label, const char* sub, bool* v, ImU32 accent, float pulse)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = ImGui::GetContentRegionAvail().x;
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
static void SliderRow(ImDrawList* dl, const char* sliderId, const char* label, const char* fmt, T* v, T vmin, T vmax, ImU32 accent, float pulse)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = ImGui::GetContentRegionAvail().x;

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
// PESTAÑAS
// ============================================================================

// Panel de "proximamente" para tabs vacios
static void ComingSoonTab(ImDrawList* dl, float pulse)
{
	ImVec2 p = ImGui::GetCursorScreenPos();
	float w = ImGui::GetContentRegionAvail().x;

	dl->AddRectFilled(p + ImVec2(2, 10), p + ImVec2(w - 2, 180), COL_BG_CARD, 8.0f);
	dl->AddRect(p + ImVec2(2, 10), p + ImVec2(w - 2, 180), COL_BORDER, 8.0f, ImDrawFlags_RoundCornersAll, 1.0f);

	ImVec2 c = p + ImVec2(w * 0.5f, 70);
	dl->AddCircleFilled(c, 20.0f, IM_COL32(9, 13, 11, 255));
	dl->AddCircle(c, 20.0f, (COL_GREEN & 0x00FFFFFF) | ((ImU32)(int)(40.0f + pulse * 30.0f) << 24), 32, 1.0f);
	if (Fonts::FontAwesomeSolid) ImGui::PushFont(Fonts::FontAwesomeSolid);
	dl->AddText(c - ImVec2(9, 9), COL_GREEN_DIM, "\uF4B7");
	if (Fonts::FontAwesomeSolid) ImGui::PopFont();

	if (Fonts::InterSemiBold) ImGui::PushFont(Fonts::InterSemiBold);
	dl->AddText(ImVec2(c.x - ImGui::CalcTextSize("PROXIMAMENTE").x * 0.5f, c.y + 32), COL_TEXT_DIM, "PROXIMAMENTE");
	if (Fonts::InterSemiBold) ImGui::PopFont();

	if (Fonts::InterRegular14) ImGui::PushFont(Fonts::InterRegular14);
	ImVec2 ss = ImGui::CalcTextSize("Esta seccion estara disponible en una proxima actualizacion");
	dl->AddText(ImVec2(c.x - ss.x * 0.5f, c.y + 56), COL_TEXT_MUTED, "Esta seccion estara disponible en una proxima actualizacion");
	if (Fonts::InterRegular14) ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, 190));
}

static void AimbotTab(ImDrawList* dl, float pulse)
{
	ComingSoonTab(dl, pulse);
}

static void EspTab(ImDrawList* dl, float pulse)
{
	SectionTitle(dl, "VISUALES");
	if (RowToggle(dl, "##EspLine", "ESP Line", "Linea hacia todos los enemigos", &g_Globals.Visuals.Lines, COL_GREEN, pulse)) {
		g_Globals.Visuals.EspLines = g_Globals.Visuals.Lines ? 1 : 0;
		if (g_Globals.Visuals.Lines) {
			g_Globals.Visuals.Enable = true;
		}
	}
	RowToggle(dl, "##EspBox", "Caja", "Caja alrededor del enemigo", &g_Globals.Visuals.Box, COL_GREEN, pulse);
	RowToggle(dl, "##EspFill", "Caja Rellena", "Relleno semi transparente", &g_Globals.Visuals.FilledBox, COL_GREEN, pulse);
	RowToggle(dl, "##EspHb", "Barra de Vida", "Barra de salud en pantalla", &g_Globals.Visuals.HealthBar, COL_GREEN, pulse);
	RowToggle(dl, "##EspNm", "Nombre", "Nombre del enemigo", &g_Globals.Visuals.Name, COL_GREEN, pulse);
	RowToggle(dl, "##EspDs", "Distancia", "Distancia al enemigo", &g_Globals.Visuals.Distance, COL_GREEN, pulse);
	RowToggle(dl, "##EspMm", "Minimapa", "Radar en pantalla", &g_Globals.Visuals.Minimap, COL_GREEN, pulse);
	RowToggle(dl, "##EspWm", "Marca de Agua", "Logo FREE en pantalla", &g_Globals.Visuals.Watermark, COL_GREEN, pulse);

	SectionTitle(dl, "RANGO");
	SliderRow(dl, "##SlEsp", "Alcance del ESP", "%d m", &g_Globals.Visuals.DistanceEsp, 50, 500, COL_GREEN, pulse);
}

static void ExploitsTab(ImDrawList* dl, float pulse)
{
	ComingSoonTab(dl, pulse);
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

void Interface::RenderGui()
{
	if (!bIsMenuOpen) return;

	ImGuiIO& io = ImGui::GetIO();
	const float dt = ImMin(io.DeltaTime, 0.1f);

	static float tGlobal = 0.0f;
	tGlobal += dt;
	const float pulse = (sinf(tGlobal * 2.4f) + 1.0f) * 0.5f;

	ImGui::SetNextWindowSize(ImVec2(460.0f, 400.0f));

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
		DrawGlowText(dl, titlePos, COL_GREEN, "FREE");
		if (Fonts::GeistRegularMedium) ImGui::PopFont();

		if (Fonts::InterLight) ImGui::PushFont(Fonts::InterLight);
		dl->AddText(Pos + ImVec2(45, 31), COL_TEXT_DIM, "C O N T R O L   P A N E L");
		if (Fonts::InterLight) ImGui::PopFont();

		DrawPulseDot(dl, Pos + ImVec2(Size.x - 26, 25), AdbStateColor(), pulse);

		// ====================================================================
		// PESTAÑAS
		// ====================================================================
		static const char* tabNames[] = { "AIMBOT", "ESP", "EXPLOITS", "CONFIG" };
		static int curTab = 0;
		float tabW = (Size.x - 32) / 4.0f;

		for (int i = 0; i < 4; i++) {
			ImVec2 tp = Pos + ImVec2(16 + i * tabW, 52);
			ImRect r(tp, tp + ImVec2(tabW, 30));
			bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max);
			bool active = (i == curTab);

			if (hov && !active) {
				dl->AddRectFilled(r.Min, r.Max, IM_COL32(255, 255, 255, 4), 6.0f);
			}

			if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
			ImVec2 ls = ImGui::CalcTextSize(tabNames[i]);
			ImVec2 lpos = tp + ImVec2((tabW - ls.x) * 0.5f, (30 - ls.y) * 0.5f - 1.0f);
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
					curTab = i;
				}
			}

			// Badge de conexion en la pestana CONFIG
			if (i == 3 && g_AdbState == AdbState::Connected) {
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
		// CONTENIDO DE LA PESTAÑA ACTIVA (con scroll)
		// ====================================================================
		ImGui::SetCursorScreenPos(Pos + ImVec2(16, 90));
		if (ImGui::BeginChild("##Content", ImVec2(Size.x - 32, Size.y - 90 - 38), false, ImGuiWindowFlags_NoBackground)) {
			ImDrawList* cdl = ImGui::GetWindowDrawList();
			switch (curTab) {
			case 0: AimbotTab(cdl, pulse); break;
			case 1: EspTab(cdl, pulse); break;
			case 2: ExploitsTab(cdl, pulse); break;
			case 3: ConfigTab(cdl, pulse); break;
			}
		}
		ImGui::EndChild();

		// ====================================================================
		// FOOTER
		// ====================================================================
		if (Fonts::InterBold12) ImGui::PushFont(Fonts::InterBold12);
		dl->AddText(Pos + ImVec2(16, Size.y - 26), COL_GREEN_DIM, "FREE v1.0");
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
