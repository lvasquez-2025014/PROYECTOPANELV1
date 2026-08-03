#include <src/ui/Gui.hpp>
#include <imgui.h>
#include <Windows.h>

static int selectedTab = 0;
bool g_OverlayVisible = true;

static const ImVec4 accentBlue = ImVec4(0.13f, 0.59f, 0.95f, 1.0f);
static const ImVec4 bgDark = ImVec4(0.08f, 0.08f, 0.12f, 0.97f);
static const ImVec4 bgPanel = ImVec4(0.14f, 0.14f, 0.18f, 0.98f);
static const ImVec4 bgSidebar = ImVec4(0.06f, 0.06f, 0.10f, 1.0f);
static const ImVec4 textPrimary = ImVec4(0.92f, 0.92f, 0.95f, 1.0f);
static const ImVec4 textSecondary = ImVec4(0.55f, 0.55f, 0.62f, 1.0f);

static bool DrawSidebarIcon(const char* label, int index) {
	ImVec2 buttonSize(44, 44);

	if (selectedTab == index)
		ImGui::PushStyleColor(ImGuiCol_Button, accentBlue);
	else
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.18f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);

	bool pressed = ImGui::Button(label, buttonSize);

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
	return pressed;
}

static void RenderTabAimbot() {
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgPanel);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

	ImGui::BeginChild("AimbotMain", ImVec2(-1, 50), true);
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
		ImGui::TextColored(textPrimary, "Aimbot");
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

static void RenderTabEsp() {
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgPanel);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

	ImGui::BeginChild("EspMain", ImVec2(-1, 50), true);
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
		ImGui::TextColored(textPrimary, "ESP");
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

static void RenderTabMisc() {
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgPanel);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

	ImGui::BeginChild("MiscMain", ImVec2(-1, 50), true);
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
		ImGui::TextColored(textPrimary, "Misc");
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

static void RenderTabSettings() {
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgPanel);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

	ImGui::BeginChild("SettingsMain", ImVec2(-1, 50), true);
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
		ImGui::TextColored(textPrimary, "Settings");
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

namespace FWork {
	namespace Ui {

		void RenderGui() {
			if (!g_OverlayVisible) return;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, bgDark);

			ImGui::SetNextWindowSize(ImVec2(720, 480), ImGuiCond_FirstUseEver);
			ImGui::Begin("##Overlay", nullptr,
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoResize);

			ImVec2 wPos = ImGui::GetWindowPos();
			ImVec2 wSize = ImGui::GetWindowSize();
			ImDrawList* dl = ImGui::GetWindowDrawList();

			float sidebarW = 52.0f;
			float footerH = 30.0f;

			dl->AddRectFilled(wPos, ImVec2(wPos.x + sidebarW, wPos.y + wSize.y),
				ImGui::GetColorU32(bgSidebar), 12.0f, ImDrawFlags_RoundCornersLeft);

			dl->AddRectFilled(
				ImVec2(wPos.x, wPos.y + wSize.y - footerH),
				ImVec2(wPos.x + wSize.x, wPos.y + wSize.y),
				ImGui::GetColorU32(bgSidebar), 8.0f, ImDrawFlags_RoundCornersBottom);

			dl->AddLine(
				ImVec2(wPos.x + sidebarW, wPos.y + 40),
				ImVec2(wPos.x + sidebarW, wPos.y + wSize.y - footerH),
				ImGui::GetColorU32(ImVec4(0.22f, 0.22f, 0.28f, 0.6f)));

			ImGui::SetCursorScreenPos(ImVec2(wPos.x + 4, wPos.y + 50));
			ImGui::BeginChild("Sidebar", ImVec2(sidebarW - 8, wSize.y - footerH - 60), false);
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6));
				if (DrawSidebarIcon("A", 0)) selectedTab = 0;
				if (DrawSidebarIcon("B", 1)) selectedTab = 1;
				if (DrawSidebarIcon("C", 2)) selectedTab = 2;
				if (DrawSidebarIcon("D", 3)) selectedTab = 3;
				ImGui::PopStyleVar();
			}
			ImGui::EndChild();

			const char* tabNames[] = { "Aimbot", "ESP", "Misc", "Settings" };
			ImGui::SetCursorScreenPos(ImVec2(wPos.x + sidebarW + 14, wPos.y + 14));
			ImGui::TextColored(textPrimary, "%s", tabNames[selectedTab]);

			ImGui::SetCursorScreenPos(ImVec2(wPos.x + sidebarW + 14, wPos.y + 42));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::BeginChild("Content", ImVec2(wSize.x - sidebarW - 28, wSize.y - footerH - 54), false);
			{
				switch (selectedTab) {
				case 0: RenderTabAimbot(); break;
				case 1: RenderTabEsp(); break;
				case 2: RenderTabMisc(); break;
				case 3: RenderTabSettings(); break;
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();

			dl->AddText(
				ImVec2(wPos.x + sidebarW + 14, wPos.y + wSize.y - footerH + 8),
				ImGui::GetColorU32(accentBlue),
				"Developer: Proyect"
			);

			ImGui::End();

			ImGui::PopStyleColor(1);
			ImGui::PopStyleVar(3);
		}

	}
}
