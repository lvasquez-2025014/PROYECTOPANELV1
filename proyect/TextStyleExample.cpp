// ===== EXEMPLO DE USO DA BIBLIOTECA TextStyleGenerator =====
// Este arquivo demonstra como usar a biblioteca para gerar nomes estilizados

#include "TextStyleGenerator.hpp"
#include <iostream>
#include <vector>

using namespace TextStyle;

int main() {
    std::string playerName = "GAMER";
    
    std::cout << "=== GERADOR DE NOMES ESTILIZADOS ===" << std::endl;
    std::cout << "Nome Original: " << playerName << std::endl << std::endl;
    
    // ===== EXEMPLO 1: Converter para um único estilo =====
    std::cout << "--- Estilos Individuais ---" << std::endl;
    std::cout << "Bold: " << StyleConverter::convert(playerName, StyleConverter::Style::BOLD) << std::endl;
    std::cout << "Italic: " << StyleConverter::convert(playerName, StyleConverter::Style::ITALIC) << std::endl;
    std::cout << "Bold Italic: " << StyleConverter::convert(playerName, StyleConverter::Style::BOLD_ITALIC) << std::endl;
    std::cout << "Fraktur: " << StyleConverter::convert(playerName, StyleConverter::Style::FRAKTUR) << std::endl;
    std::cout << "Double Struck: " << StyleConverter::convert(playerName, StyleConverter::Style::DOUBLE_STRUCK) << std::endl;
    std::cout << "Bubble: " << StyleConverter::convert(playerName, StyleConverter::Style::BUBBLE) << std::endl;
    std::cout << "Black Bubble: " << StyleConverter::convert(playerName, StyleConverter::Style::BLACK_BUBBLE) << std::endl;
    std::cout << "Monospace: " << StyleConverter::convert(playerName, StyleConverter::Style::MONOSPACE) << std::endl;
    std::cout << "Small Caps: " << StyleConverter::convert(playerName, StyleConverter::Style::SMALL_CAPS) << std::endl;
    std::cout << "Tiny: " << StyleConverter::convert(playerName, StyleConverter::Style::TINY) << std::endl;
    std::cout << "Upside Down: " << StyleConverter::convert(playerName, StyleConverter::Style::UPSIDE_DOWN) << std::endl;
    std::cout << "Fancy Style 1: " << StyleConverter::convert(playerName, StyleConverter::Style::FANCY_STYLE_1) << std::endl;
    std::cout << "Fancy Style 2: " << StyleConverter::convert(playerName, StyleConverter::Style::FANCY_STYLE_2) << std::endl;
    std::cout << "Magic: " << StyleConverter::convert(playerName, StyleConverter::Style::MAGIC) << std::endl;
    std::cout << "Full Width: " << StyleConverter::convert(playerName, StyleConverter::Style::FULL_WIDTH) << std::endl;
    std::cout << "Square: " << StyleConverter::convert(playerName, StyleConverter::Style::SQUARE) << std::endl;
    std::cout << "Strikethrough: " << StyleConverter::convert(playerName, StyleConverter::Style::STRIKETHROUGH) << std::endl;
    std::cout << "Underline: " << StyleConverter::convert(playerName, StyleConverter::Style::UNDERLINE) << std::endl;
    
    // ===== EXEMPLO 2: Converter para múltiplos estilos =====
    std::cout << "\n--- Múltiplos Estilos ---" << std::endl;
    std::vector<StyleConverter::Style> styles = {
        StyleConverter::Style::BOLD,
        StyleConverter::Style::BUBBLE,
        StyleConverter::Style::MAGIC
    };
    
    auto results = StyleConverter::convertMultiple(playerName, styles);
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << StyleConverter::getStyleName(styles[i]) << ": " << results[i] << std::endl;
    }
    
    // ===== EXEMPLO 3: Aplicar diacríticos =====
    std::cout << "\n--- Diacríticos ---" << std::endl;
    std::cout << "Bridge Above: " << DiacriticsApplier::applyDiacritic(playerName, DiacriticsApplier::DiacriticType::BRIDGE_ABOVE) << std::endl;
    std::cout << "Asterisk Below: " << DiacriticsApplier::applyDiacritic(playerName, DiacriticsApplier::DiacriticType::ASTERISK_BELOW) << std::endl;
    std::cout << "X Above/Below: " << DiacriticsApplier::applyDiacritic(playerName, DiacriticsApplier::DiacriticType::X_ABOVE_BELOW) << std::endl;
    std::cout << "Love Hearts: " << DiacriticsApplier::applyDiacritic(playerName, DiacriticsApplier::DiacriticType::LOVE_HEARTS) << std::endl;
    std::cout << "Invisible Ink: " << DiacriticsApplier::applyDiacritic(playerName, DiacriticsApplier::DiacriticType::INVISIBLE_INK) << std::endl;
    
    // ===== EXEMPLO 4: Geradores especiais =====
    std::cout << "\n--- Geradores Especiais ---" << std::endl;
    std::cout << "Com espaços invisíveis: " << SpecialGenerators::addInvisibleSpaces(playerName, 1) << std::endl;
    std::cout << "Com símbolos decorativos: " << SpecialGenerators::addDecorativeSymbols(playerName, "✿") << std::endl;
    std::cout << "Com emojis: " << SpecialGenerators::addRandomEmojis(playerName) << std::endl;
    std::cout << "Com cor FF [FF0000]: " << SpecialGenerators::addFFColor(playerName, "FF0000") << std::endl;
    
    // ===== EXEMPLO 5: Combinar estilos =====
    std::cout << "\n--- Combinações de Estilos ---" << std::endl;
    std::vector<StyleConverter::Style> combinedStyles = {
        StyleConverter::Style::BOLD,
        StyleConverter::Style::FULL_WIDTH
    };
    std::cout << "Bold + Full Width: " << SpecialGenerators::combineStyles(playerName, combinedStyles) << std::endl;
    
    // ===== EXEMPLO 6: Validações =====
    std::cout << "\n--- Validações ---" << std::endl;
    std::cout << "Comprimento original: " << playerName.length() << std::endl;
    std::cout << "É válido para Free Fire (max 50): " << (Utils::isValidFFLength(playerName) ? "Sim" : "Não") << std::endl;
    std::cout << "Comprimento efetivo: " << Utils::getEffectiveLength(playerName) << std::endl;
    
    // ===== EXEMPLO 7: Listar todos os estilos =====
    std::cout << "\n--- Todos os Estilos Disponíveis ---" << std::endl;
    auto allStyles = StyleConverter::getAllStyles();
    for (const auto& style : allStyles) {
        std::cout << StyleConverter::getStyleName(style) << ": " 
                  << StyleConverter::convert(playerName, style) << std::endl;
    }
    
    return 0;
}

// ===== EXEMPLO DE INTEGRAÇO COM ImGui (para a interface do projeto) =====
/*
void RenderTextStyleGenerator() {
    static char inputText[256] = "GAMER";
    static int selectedStyle = 0;
    
    ImGui::InputText("Nome", inputText, sizeof(inputText));
    
    auto allStyles = TextStyle::StyleConverter::getAllStyles();
    std::vector<const char*> styleNames;
    for (const auto& style : allStyles) {
        styleNames.push_back(TextStyle::StyleConverter::getStyleName(style).c_str());
    }
    
    ImGui::Combo("Estilo", &selectedStyle, styleNames.data(), styleNames.size());
    
    std::string styledName = TextStyle::StyleConverter::convert(
        inputText, 
        allStyles[selectedStyle]
    );
    
    ImGui::Text("Resultado: %s", styledName.c_str());
    
    if (ImGui::Button("Copiar")) {
        // Copiar para clipboard
    }
}
*/
