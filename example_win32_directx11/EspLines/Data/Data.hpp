#pragma once
#include <cstdint>
#include <set>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>

// ============================================================================
// CONTEXTO DE LA PARTIDA ACTUAL
// ============================================================================
struct GameContext {
    uint32_t currentGame;   // Direccion del objeto del juego
    uint32_t currentMatch;  // Direccion de la partida en curso
    uint32_t localPlayer;   // Direccion del jugador local
};

namespace FWork {
    class Data {
    public:
        // Punto de entrada: procesa los datos de la partida (se llama cada frame)
        static void Work();

    private:
        // Limpia todas las entidades acumuladas
        static void Reset();

        // Obtiene la direccion del juego actual desde la base de Il2Cpp
        static uint32_t GetCurrentGame();

        // Obtiene la direccion de la partida actual a partir del juego
        static uint32_t GetCurrentMatch(uint32_t currentGame);

        // Configura el jugador local y la camara (posicion + view matrix)
        static bool SetupLocalPlayerAndCamera(uint32_t currentMatch);

        // Recorre el diccionario de entidades y actualiza la lista de jugadores
        static void ProcessEntities(const GameContext& ctx);
    };
}
