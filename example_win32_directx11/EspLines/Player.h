#pragma once
#include "Math/Vector/Vector3.hpp"
#include <string>
#include <cstdint>

#ifndef BOOL3_H
#define BOOL3_H

class Player {
public:
    enum class Bool3 { True, False, Unknown };
    bool IsBot;
    bool IsKnown;
    bool IsDead;
    bool IsKnocked;
    bool IsVisible;
    
    float Distance;
    short Health;
    short WeaponID;
    short Pose;
    Bool3 IsTeam;
    uint32_t Address;
    std::string Name;

    // ===== ESTRUTURA OTIMIZADA (APENAS O ESSENCIAL) =====
    // Removido skeleton completo para economizar CPU e Memória
    Vector3 Head;          // Cabeça (Essencial para Aimbot e Topo da Box)
    Vector3 Hip;           // Quadril (Essencial para Base da Box e fallback)
    Vector3 Root;          // Raiz (Base do corpo)

    // ===== CACHE DE POSIÇÃO ANTERIOR =====
    Vector3 LastHead;      // Última posição da cabeça
    Vector3 LastHip;       // Última posição do quadril

    // ===== PREDIÇÃO DE MOVIMENTO =====
    Vector3 Velocity;      // Velocidade atual do jogador
    Vector3 LastRoot;      // Última posição conhecida para cálculo de velocidade
    float LastUpdateTime;  // Tempo da última atualização de posição
};

#endif
