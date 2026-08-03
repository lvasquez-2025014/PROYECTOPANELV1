#pragma once
#include "Math/Vector/Vector3.hpp"
#include <string>
#include <cstdint>

#ifndef BOOL3_H
#define BOOL3_H

class Player {
public:
    enum class Bool3 { True, False, Unknown };
    bool IsBot = false;
    bool IsKnown = false;
    bool IsDead = false;
    bool IsKnocked = false;
    bool IsVisible = false;

    float Distance = 0.0f;
    short Health = 100;
    short WeaponID = 0;
    short Pose = 0;
    Bool3 IsTeam = Bool3::Unknown;
    uint32_t Address = 0;
    std::string Name;

    // ===== ESTRUTURA OTIMIZADA (APENAS O ESSENCIAL) =====
    // Removido skeleton completo para economizar CPU e Memória
    Vector3 Head;          // Cabeça (Essencial para Aimbot e Topo da Box)
    Vector3 Neck;          // Pescoço (Target opcional do aimbot)
    Vector3 Hip;           // Quadril (Essencial para Base da Box e fallback)
    Vector3 Root;          // Raiz (Base do corpo)

    // ===== ESQUELETO COMPLETO (solo se lee si el ESP skeleton esta activo) =====
    struct SkeletonBones {
        Vector3 Head;
        Vector3 Neck;
        Vector3 Spine;
        Vector3 Pelvis;    // centro de la cadera
        Vector3 LeftShoulder, LeftElbow, LeftHand;
        Vector3 RightShoulder, RightElbow, RightHand;
        Vector3 LeftAnkle, LeftFoot;
        Vector3 RightAnkle, RightFoot;
    };
    SkeletonBones Skeleton;

    // ===== CACHE DE POSIÇÃO ANTERIOR =====
    Vector3 LastHead;      // Última posição da cabeça
    Vector3 LastHip;       // Última posição do quadril

    // ===== PREDIÇÃO DE MOVIMENTO =====
    Vector3 Velocity;      // Velocidade atual do jogador
    Vector3 LastRoot;      // Última posição conhecida para cálculo de velocidade
    float LastUpdateTime = 0.0f;  // Tempo da última atualização de posição
};

#endif
