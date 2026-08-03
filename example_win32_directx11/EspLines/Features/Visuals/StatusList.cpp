#include "StatusList.hpp"
#include "src/Globals.hpp"
#include <imgui.h>
#include <cmath>

void StatusList::Render()
{
    struct FuncaoStatus
    {
        bool ativo;
        const char* nome;
    };

    FuncaoStatus funcoes[] = {
        { g_Globals.AimBot.MemoryAim,              "AIMBOT MEMORY" },
        { g_Globals.AimBot.RageAim,                "AIMBOT RAGE" },
    };

    const int numFuncoes = sizeof(funcoes) / sizeof(funcoes[0]);

    // =========================
    // TAMANHO
    // =========================
    const float largura = 240.0f;
    const float alturaLinha = 30.0f;
    const float alturaHeader = 52.0f;
    const float alturaPadding = 18.0f;
    const float alturaTotal = alturaHeader + (numFuncoes * alturaLinha) + alturaPadding;

    ImGui::SetNextWindowPos(
        ImVec2(
            g_Globals.EspConfig.Width - largura - 25.0f,
            g_Globals.EspConfig.Height - alturaTotal - 25.0f
        ),
        ImGuiCond_Always
    );

    ImGui::SetNextWindowSize(ImVec2(largura, alturaTotal));

    // =========================
    // ESTILO
    // =========================
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

    ImGui::Begin(
        "##LAMAFIA_STATUS",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoBackground
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetWindowPos();
    const ImVec2 s = ImGui::GetWindowSize();
    const float rounding = 14.0f;

    // =========================
    // ANIMAÇÃO GLOBAL
    // =========================
    const float time = ImGui::GetTime();
    const float pulse = (sinf(time * 3.0f) + 1.0f) * 0.5f;

    // =========================
    // CORES BASE (OTIMIZADO)
    // =========================
    const ImColor bgColor(8, 8, 8, 215);
    const ImColor borderGlow(0, 220, 255, 25);

    // =========================
    // FUNDO
    // =========================
    draw->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y), bgColor, rounding);

    // Glow externo (leve)
    for (int i = 0; i < 3; i++)
    {
        draw->AddRect(
            ImVec2(p.x - i, p.y - i),
            ImVec2(p.x + s.x + i, p.y + s.y + i),
            ImColor(0, 220, 255, 25 - i * 8),
            rounding
        );
    }

    // Gradiente topo
    draw->AddRectFilledMultiColor(
        p,
        ImVec2(p.x + s.x, p.y + 4),
        ImColor(0, 220, 255, 200),
        ImColor(0, 120, 255, 200),
        ImColor(0, 120, 255, 200),
        ImColor(0, 220, 255, 200)
    );

    // Brilho interno
    draw->AddRectFilled(
        ImVec2(p.x + 1, p.y + 1),
        ImVec2(p.x + s.x - 1, p.y + 38),
        ImColor(255, 255, 255, 10),
        rounding,
        ImDrawFlags_RoundCornersTop
    );

    // Barra lateral
    draw->AddRectFilled(
        ImVec2(p.x, p.y),
        ImVec2(p.x + 4, p.y + s.y),
        ImColor(0, 220, 255, 180),
        rounding,
        ImDrawFlags_RoundCornersLeft
    );

    // =========================
    // HEADER
    // =========================
    draw->AddText(ImVec2(p.x + 16, p.y + 10), ImColor(255, 255, 255), "ASMODEUS");
    draw->AddText(ImVec2(p.x + 16, p.y + 28), ImColor(140, 140, 140), "STATUS PANEL");

    draw->AddLine(
        ImVec2(p.x + 12, p.y + alturaHeader),
        ImVec2(p.x + s.x - 12, p.y + alturaHeader),
        ImColor(255, 255, 255, 20)
    );

    // =========================
    // LISTA
    // =========================
    float yBase = p.y + alturaHeader + 10.0f;

    for (int i = 0; i < numFuncoes; i++)
    {
        const float y = yBase + (i * alturaLinha);
        const bool ativo = funcoes[i].ativo;

        const ImColor statusColor = ativo
            ? ImColor(0, 255, 140)
            : ImColor(255, 70, 70);

        // highlight em float (correto)
        const float highlight = ativo ? (0.08f + pulse * 0.08f) : 0.03f;

        // Fundo linha
        draw->AddRectFilled(
            ImVec2(p.x + 10, y - 4),
            ImVec2(p.x + s.x - 10, y + 20),
            ImColor(1.0f, 1.0f, 1.0f, highlight),
            8.0f
        );

        // =========================
        // GLOW OTIMIZADO (menos custo)
        // =========================
        const ImVec2 circlePos(p.x + 24, y + 8);

        const int glowAlpha = 40 + (int)(pulse * 40);

        draw->AddCircleFilled(circlePos, 8.0f, ImColor(statusColor.Value.x, statusColor.Value.y, statusColor.Value.z, glowAlpha / 255.0f));
        draw->AddCircleFilled(circlePos, 5.5f, ImColor(statusColor.Value.x, statusColor.Value.y, statusColor.Value.z, 0.25f));

        // Núcleo
        draw->AddCircleFilled(circlePos, 4.0f, statusColor);

        // Nome
        draw->AddText(
            ImVec2(p.x + 38, y),
            ativo ? ImColor(255, 255, 255) : ImColor(155, 155, 155),
            funcoes[i].nome
        );

        // Estado
        const char* estado = ativo ? "ON" : "OFF";
        const ImVec2 estadoSize = ImGui::CalcTextSize(estado);

        draw->AddText(
            ImVec2(p.x + s.x - estadoSize.x - 16, y),
            statusColor,
            estado
        );
    }

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}