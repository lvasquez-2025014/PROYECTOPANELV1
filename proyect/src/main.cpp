#include <Windows.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <src/Overlay/Overlay.hpp>
#include <src/Overlay/Render.hpp>
#include <src/Fonts/Fonts.hpp>
#include <src/ui/Gui.hpp>

extern bool g_OverlayVisible;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void HookWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
}

static DWORD WINAPI OverlayThread(LPVOID lpParam) {
	HWND hTarget = Render::FindRenderWindow();
	if (!hTarget) {
		return 1;
	}

	FWork::Overlay::Setup(hTarget);
	FWork::Overlay::Initialize();

	if (!FWork::Overlay::IsInitialized()) {
		return 1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();

	if (!ImGui_ImplWin32_Init(FWork::Overlay::GetOverlayWindow())) {
		return 1;
	}
	if (!ImGui_ImplDX11_Init(FWork::Overlay::dxGetDevice(), FWork::Overlay::dxGetDeviceContext())) {
		return 1;
	}

	FWork::Fonts::Initialize(FWork::Overlay::dxGetDevice());
	FWork::Overlay::SetupWindowProcHook(HookWindowProc);

	MSG msg = {};
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		FWork::Overlay::UpdateWindowPos();

		if (GetAsyncKeyState(VK_INSERT) & 1)
			g_OverlayVisible = !g_OverlayVisible;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		FWork::Ui::RenderGui();

		ImGui::Render();

		FWork::Overlay::dxRefresh();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if (FAILED(FWork::Overlay::dxGetSwapChain()->Present(1, 0)))
			break;

		Sleep(1);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	FWork::Overlay::ShutDown();

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
	}
	return TRUE;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	OverlayThread(nullptr);
	return 0;
}
