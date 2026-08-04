#include <windows.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <D3DX11tex.h>
#pragma comment(lib, "D3DX11.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "winmm.lib")
#include <thread>
#include <iostream>
#include <atomic> 
#include <string>
#include <vector>
#include <include/MinHook.h>
#include <TlHelp32.h>
#include <src/adb/adb.hpp>
#include <src/Overlay/Overlay.hpp>
#include <src/Overlay/Render.hpp>
#include <src/ui/ui.hpp>
#include <src/Globals.hpp>
#include <EspLines/Data/Data.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Aimbot/SilentAim.hpp>
#include <EspLines/Exploits/TeleKill.hpp>
#include <EspLines/Exploits/WeaponAttributes.hpp>
#include <EspLines/Features/Visuals/Visual.hpp>
#include <src/adb/adb_utils.hpp>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif


void initcmd() {
	AllocConsole();
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	SetConsoleTitleA("Debug Console");
}

void closecmd() {
	FreeConsole();
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
}

HMODULE g_hModule;
FWork::Interface* g_pInterface = nullptr;
static HANDLE g_HiResTimer = nullptr;
static bool g_SystemTimerPeriod = false;

void KillEmulator();
DWORD WINAPI Unload() {
	if (g_pInterface) {
		g_pInterface->ShutDown();
		delete g_pInterface;
		g_pInterface = nullptr;
	}

	if (MemoryUtils::ogPhysRead) {
		if (MH_DisableHook((LPVOID)MemoryUtils::ogPhysRead) != MH_OK) {
			std::cout << "Falha ao desativar o hook de PGMPhysRead!" << std::endl;
		}

		if (MH_RemoveHook((LPVOID)MemoryUtils::ogPhysRead) != MH_OK) {
			std::cout << "Falha ao remover o hook de PGMPhysRead!" << std::endl;
		}
	}

	MH_Uninitialize();

	if (g_hModule) {
		FreeLibraryAndExitThread(g_hModule, 0);
	}
	return 0;
}

bool memoryinit = false;
void Memoryy() {
	auto vmm = GetModuleHandleA("BstkVMM.dll");
	if (vmm == nullptr) {
		return;
	}

	auto readFunc = (MemoryUtils::PGMPhysReadFunc)GetProcAddress(vmm, "PGMPhysRead");
	if (readFunc == nullptr) {
		return;
	}

	MH_Initialize();
	if (MH_CreateHook((LPVOID)readFunc, MemoryUtils::HookedPGMPhysRead, (LPVOID*)&MemoryUtils::ogPhysRead) != MH_OK) {
		return;
	}

	if (MH_EnableHook((LPVOID)readFunc) != MH_OK) {
		return;
	}

	int timeout = 5000;
	int elapsed = 0;

	while (MemoryUtils::vmPtr == nullptr && elapsed < timeout) {
		Sleep(1);
		elapsed++;
	}

	MemoryUtils::ogCPU = (MemoryUtils::VMMGetCpuByIdFunc)GetProcAddress(vmm, "VMMGetCpuById");
	if (MemoryUtils::ogCPU == nullptr) {
		return;
	}

	MemoryUtils::ogCast = (MemoryUtils::PGMPhysGCPtr2GCPhysFunc)GetProcAddress(vmm, "PGMPhysGCPtr2GCPhys");
	if (MemoryUtils::ogCast == nullptr) {
		return;
	}

	MemoryUtils::ogWrite = (MemoryUtils::PGMPhysSimpleWriteGCPhysFunc)GetProcAddress(vmm, "PGMPhysSimpleWriteGCPhys");
	if (MemoryUtils::ogWrite == nullptr) {
		return;
	}

	MemoryUtils::Initialize(MemoryUtils::vmPtr);
	std::cout << "Virt Memory: " << MemoryUtils::pVMAddr << std::endl;

	memoryinit = true;
}

namespace Cheat {
	void Initialize() {
		FWork::Overlay::Setup(Render::FindRenderWindow());
		FWork::Overlay::Initialize();
		Memoryy();
		if (!memoryinit) {
			MessageBox(nullptr, L"Memory initialization failed", L"Error", MB_OK | MB_ICONERROR);
		}

		if (FWork::Overlay::IsInitialized())
		{
			FWork::Interface Interface(FWork::Overlay::GetOverlayWindow(), FWork::Overlay::GetTargetWindow(), FWork::Overlay::dxGetDevice(), FWork::Overlay::dxGetDeviceContext());
			Interface.UpdateStyle();
			FWork::Overlay::SetupWindowProcHook(std::bind(&FWork::Interface::WindowProc, &Interface, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

			MSG Message;
			ZeroMemory(&Message, sizeof(Message));
			auto FrameStart = std::chrono::steady_clock::now();
			while (Message.message != WM_QUIT) {

				if (PeekMessage(&Message, FWork::Overlay::GetOverlayWindow(), NULL, NULL, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				ImGui::GetIO().MouseDrawCursor = false;

				if (Interface.ResizeHeight != 0 || Interface.ResizeWidht != 0) 
				{
					FWork::Overlay::dxCleanupRenderTarget();
					FWork::Overlay::dxGetSwapChain()->ResizeBuffers(0, Interface.ResizeWidht, Interface.ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
					Interface.ResizeHeight = Interface.ResizeWidht = 0;
					FWork::Overlay::dxCreateRenderTarget();
				}

				Interface.HandleMenuKey();
				FWork::Overlay::UpdateWindowPos();

				// DELETE: cierra el panel (overlay) y mata el emulador + juego
				static bool DeleteKeyWasDown = false;
				bool deleteDown = (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;
				if (deleteDown && !DeleteKeyWasDown) {
					g_Globals.General.ShutDown = true;
					std::thread([]() {
						// Espera a que el overlay se cierre limpiamente antes de matar el emulador
						std::this_thread::sleep_for(std::chrono::milliseconds(500));
						KillEmulator();
					}).detach();
				}
				DeleteKeyWasDown = deleteDown;

				static bool CaptureBypassOn = false;
				if (g_Globals.General.Capture != CaptureBypassOn) 
				{
					CaptureBypassOn = g_Globals.General.Capture;
					SetWindowDisplayAffinity(hWindow, CaptureBypassOn ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
				}

				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();

				// ===== INPUT DIRECTO DEL RATON (EVENTOS) =====
				// El juego/emulador captura el raton, por lo que los mensajes no
				// llegan de forma confiable al WndProc del overlay. Se inyecta el
				// estado real del raton como eventos de ImGui (posicion y botones)
				// cada frame; es estable y no depende de los mensajes de Windows.
				// IMPORTANTE: ImGui trabaja en coordenadas de CLIENTE (0,0 = borde
				// superior izquierdo del overlay), igual que ImGui_ImplWin32_NewFrame().
				// Convertir de pantalla a cliente mantiene el hover alineado aunque
				// el overlay no este en (0,0) (emulador en modo ventana).
				{
					ImGuiIO& mIo = ImGui::GetIO();
					HWND overlayHwnd = FWork::Overlay::GetOverlayWindow();
					POINT mp = {};
					RECT wrect;
					bool inside = GetWindowRect(overlayHwnd, &wrect)
						&& GetCursorPos(&mp)
						&& mp.x >= wrect.left && mp.x < wrect.right
						&& mp.y >= wrect.top && mp.y < wrect.bottom;

					if (inside) {
						POINT c = mp;
						ScreenToClient(overlayHwnd, &c);
						mIo.AddMousePosEvent((float)c.x, (float)c.y);
					}

					static bool lastDown[3] = { false, false, false };
					bool down[3] = {
						(GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0,
						(GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0,
						(GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0
					};
					for (int b = 0; b < 3; b++) {
						if (down[b] != lastDown[b]) {
							// Los releases SIEMPRE se entregan (aunque el cursor este
							// fuera del overlay) para que ImGui no se quede con el
							// boton "atascado" pulsado. Los presses solo dentro.
							if (inside || !down[b]) {
								mIo.AddMouseButtonEvent(b, down[b]);
							}
							lastDown[b] = down[b];
						}
					}
				}

				ImGui::NewFrame();
				{
					FWork::Data::Work();
					
					Interface.RenderGui();

					if (g_Globals.Visuals.Enable) {
						ESP::Players();
					}


					if (g_Globals.Misc.ShowAimbotFov) {
						ImColor OutlineColor = ImColor(g_Globals.Misc.AimbotFovColor[0], g_Globals.Misc.AimbotFovColor[1], g_Globals.Misc.AimbotFovColor[2], g_Globals.Misc.AimbotFovColor[3]);
						ImColor Fillcolor = ImColor(
							g_Globals.AimBot.Fillcolor[0],
							g_Globals.AimBot.Fillcolor[1],
							g_Globals.AimBot.Fillcolor[2],
							g_Globals.AimBot.Fillcolor[3]
						);

						const float fovRadius = (float)g_Globals.AimBot.DistanceAim;

						ImGui::GetBackgroundDrawList()->AddCircleFilled(
							ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2),
							fovRadius,
							Fillcolor,
							360
						);

						ImGui::GetBackgroundDrawList()->AddCircle(
							ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2),
							fovRadius,
							OutlineColor,
							360
						);
					}
				}
				ImGui::EndFrame();
				ImGui::Render();
				FWork::Overlay::dxRefresh();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				// Present(1, 0) = vsync: cada vblank muestra un frame completo del overlay.
				// Presentar sin sincronizar (0, 0) a ~120 FPS sobre un emulador a 60 Hz
				// alterna los buffers en momentos arbitrarios del refresco y DWM compone
				// el overlay por-pixel-alfa con buffers viejos/medio dibujados: eso provoca
				// el ghosting y parpadeo de las ESP en pantalla completa.
				FWork::Overlay::dxGetSwapChain()->Present(1, 0);

				if (g_Globals.General.ShutDown) {
					Unload();
					return;
				}

				if (g_Globals.General.Delay > 0) {
					// Retardo manual configurado
					std::this_thread::sleep_for(std::chrono::milliseconds(g_Globals.General.Delay));
				}
				else {
					// Limitador de FPS preciso a 120 (8.33 ms por frame).
					// Usa un waitable timer de alta resolución (preciso a 0.5 ms,
					// sin el redondeo de sleep_for que provoca ~15 ms y baja a ~64 FPS).
					static bool timerReady = false;
					if (!timerReady) {
						g_HiResTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
						if (!g_HiResTimer) {
							// Fallback: resolver el timer del sistema a 1 ms
							timeBeginPeriod(1);
							g_SystemTimerPeriod = true;
						}
						timerReady = true;
					}

					auto Now = std::chrono::steady_clock::now();
					double elapsed = std::chrono::duration<double, std::milli>(Now - FrameStart).count();
					// Limite = refresco real del monitor (60/120/144/240...): un frame por
					// vblank, igual que el Present con vsync. Fallback a 120 si falla.
					static double TargetFrameMs = 0.0;
					if (TargetFrameMs <= 0.0) {
						DEVMODEW dm = {};
						dm.dmSize = sizeof(dm);
						DWORD refresh = 0;
						if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 0)
							refresh = dm.dmDisplayFrequency;
						TargetFrameMs = refresh > 0 ? (1000.0 / (double)refresh) : (1000.0 / 120.0);
					}
					double remain = TargetFrameMs - elapsed;
					if (remain > 0.0) {
						if (g_HiResTimer) {
							LARGE_INTEGER due;
							due.QuadPart = -((LONGLONG)(remain * 10000.0)); // en unidades de 100 ns
							SetWaitableTimer(g_HiResTimer, &due, 0, nullptr, nullptr, FALSE);
							WaitForSingleObject(g_HiResTimer, INFINITE);
						}
						else {
							std::this_thread::sleep_for(std::chrono::milliseconds((int)remain));
						}
					}
					FrameStart = std::chrono::steady_clock::now();
				}
			}
		}
	}
}

void KillEmulator() {
	// Termina el emulador (BlueStacks) y sus procesos relacionados
	const wchar_t* targets[] = { L"HD-Player.exe", L"Bluestacks.exe", L"BluestacksHelper.exe" };
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return;
	PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W) };
	if (Process32FirstW(snapshot, &entry)) {
		do {
			for (const wchar_t* name : targets) {
				if (_wcsicmp(entry.szExeFile, name) == 0) {
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
					if (hProcess) {
						TerminateProcess(hProcess, 1);
						CloseHandle(hProcess);
					}
					break;
				}
			}
		} while (Process32NextW(snapshot, &entry));
	}
	CloseHandle(snapshot);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
#ifdef _DEBUG
	initcmd();
#endif // _DEBUG

    Cheat::Initialize();

	// Hilo del TeleKill (lazy: revisa su propio switch dentro del loop)
	TeleKill::Start();

	while (!g_Globals.General.ShutDown) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// Limpieza del limitador de FPS
	if (g_HiResTimer) {
		CloseHandle(g_HiResTimer);
		g_HiResTimer = nullptr;
	}
	if (g_SystemTimerPeriod) {
		timeEndPeriod(1);
		g_SystemTimerPeriod = false;
	}

	// Detener el hilo del silent aim antes de descargar la DLL
	Aim::SilentAimStop();

	// Detener el hilo del TeleKill (idempotente)
	TeleKill::Stop();

	// Restaurar atributos de arma (fire scale / speed scales) al descargar
	WeaponAttributes::RestoreAll();

	closecmd();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wWinMain, hModule, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
		g_Globals.General.ShutDown = true;
		// Detener el hilo del silent aim (idempotente): sin hilos vivos
		// cuando la DLL se desmapea.
		Aim::SilentAimStop();
		TeleKill::Stop();
        break;
    }
    return TRUE;
}

