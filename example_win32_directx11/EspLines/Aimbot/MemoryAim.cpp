#include "MemoryAim.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/AimB.hpp>
#define NOMINMAX
#include <Windows.h>
#undef min
#include <algorithm>
#include <cmath>

namespace Aim {
	void MemoryAimWork() {
		if (!g_Globals.AimBot.MemoryAim) return;

		// Verificações básicas de segurança
		if (!g_Globals.EspConfig.Matrix || g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0) {
			return;
		}

		// === ATIVAÇÃO INSTANTÂNEA (sin humanización, sin delays) ===
		bool isButtonPressed = (GetAsyncKeyState(g_Globals.AimBot.AimbotBind) & 0x8000) != 0;
		if (!isButtonPressed) return;

		// === SELEÇÃO DE ALVO (Closest to Crosshair) ===
		Player* bestTarget = nullptr;
		float closestDistance = FLT_MAX;
		Vector2 screenCenter((float)g_Globals.EspConfig.Width / 2.0f, (float)g_Globals.EspConfig.Height / 2.0f);

		for (auto& pair : g_Globals.EspConfig.Entities) {
			Player* entity = &pair.second;

			// Filtros configurados: solo actuan si el usuario los activa.
			// Por defecto el aimbot funciona con CUALQUIER entidad viva.
			if (entity->IsDead || (g_Globals.AimBot.IgnoreKnocked && entity->IsKnocked)) continue;
			if (g_Globals.AimBot.IgnoreBots && entity->IsBot) continue;
			if (g_Globals.AimBot.OnlyEnemies && entity->IsTeam == Player::Bool3::True) continue;
			if (entity->Head == Vector3::Zero()) continue;

			// Projeção na tela
			ImVec2 target2D = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, entity->Head, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

			// Verifica se está na tela (com margem de segurança)
			if (target2D.x < 5 || target2D.y < 5 || target2D.x > g_Globals.EspConfig.Width - 5 || target2D.y > g_Globals.EspConfig.Height - 5)
				continue;

			float crosshairDist = Vector2::Distance(screenCenter, Vector2(target2D.x, target2D.y));

			// Seleciona o alvo mais próximo dentro do FOV
			if (crosshairDist < closestDistance && crosshairDist <= (float)g_Globals.AimBot.DistanceAim) {
				closestDistance = crosshairDist;
				bestTarget = entity;
			}
		}

		// === EXECUÇÃO DO AIMBOT (LITE & INSTANTÂNEO) ===
		if (bestTarget && bestTarget->Head != Vector3::Zero()) {

			// 1. Definir o osso alvo (HEAD / NECK / HIP)
			Vector3 aimPosition;
			switch (g_Globals.AimBot.TargetBone) {
			case Config::Bone::Neck:
				aimPosition = bestTarget->Neck != Vector3::Zero() ? bestTarget->Neck : bestTarget->Head;
				break;
			case Config::Bone::Hip:
				aimPosition = bestTarget->Hip != Vector3::Zero() ? bestTarget->Hip : bestTarget->Head;
				break;
			case Config::Bone::Head:
			default:
				aimPosition = bestTarget->Head;
				break;
			}
			if (aimPosition == Vector3::Zero()) return;

			// Offset apenas no HEAD para garantir o tiro no crânio (Rage mode)
			if (g_Globals.AimBot.TargetBone == Config::Bone::Head)
				aimPosition.Y += 0.12f;

			// 2. Calcular rotação necessária DIRETA para o alvo
			// Remove-se qualquer bias ou suavização interna da função GetRotationToLocation
			Quaternion targetRotation = AimB::GetRotationToLocation(aimPosition, 0.0f, g_Globals.EspConfig.MainCamera);

			// 3. Escritura DIRETA e INSTANTÂNEA na memória do jogador
			// Sem Slerp, sem interpolação, sem predição de velocidade.
			// Corre cada frame mientras la tecla esta pulsada: mira fijada al target.
			Mem.Write<Quaternion>(g_Globals.EspConfig.LocalPlayer + Offsets::AimRotation, targetRotation);
		}
	}
}
