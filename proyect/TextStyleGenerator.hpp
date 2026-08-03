#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace TextStyle {

    // ===== MAPEAMENTOS DE CARACTERES PARA DIFERENTES ESTILOS =====

    class StyleConverter {
    private:
        // Mapeamento para Bold (𝐀𝐁𝐂...)
        static const std::unordered_map<char, std::string> boldMap;
        
        // Mapeamento para Italic (𝘈𝘉𝘊...)
        static const std::unordered_map<char, std::string> italicMap;
        
        // Mapeamento para Bold Italic (𝙖𝙗𝙘...)
        static const std::unordered_map<char, std::string> boldItalicMap;
        
        // Mapeamento para Fraktur (𝔞𝔟𝔠...)
        static const std::unordered_map<char, std::string> frakturMap;
        
        // Mapeamento para Bold Fraktur (𝖆𝖇𝖈...)
        // Nota: Usando frakturMap como base para Bold Fraktur
        
        // Mapeamento para Double Struck (𝕒𝕓𝕔...)
        static const std::unordered_map<char, std::string> doubleStruckMap;
        
        // Mapeamento para Bubble (ⓐⓑⓒ...)
        static const std::unordered_map<char, std::string> bubbleMap;
        
        // Mapeamento para Black Bubble (🅐🅑🅒...)
        static const std::unordered_map<char, std::string> blackBubbleMap;
        
        // Mapeamento para Monospace (𝚊𝚋𝚌...)
        static const std::unordered_map<char, std::string> monospaceMap;
        
        // Mapeamento para Small Caps (ᴀʙᴄ...)
        static const std::unordered_map<char, std::string> smallCapsMap;
        
        // Mapeamento para Tiny (ᴀʙᴄ...)
        static const std::unordered_map<char, std::string> tinyMap;
        
        // Mapeamento para Upside Down (ɐqɔ...)
        static const std::unordered_map<char, std::string> upsideDownMap;
        
        // Mapeamento para Fancy Style 1 (яεsυℓтα∂σ)
        static const std::unordered_map<char, std::string> fancyStyle1Map;
        
        // Mapeamento para Fancy Style 2 (尺乇丂ㄩㄥㄒ卂ᗪㄖ)
        static const std::unordered_map<char, std::string> fancyStyle2Map;
        
        // Mapeamento para Magic (ꮢꭼsuꮮꮖꭺꭰꮎ)
        static const std::unordered_map<char, std::string> magicMap;
        
        // Mapeamento para Full Width (ａｂｃ...)
        static const std::unordered_map<char, std::string> fullWidthMap;
        
        // Mapeamento para Square (🅁🄴🅂...)
        static const std::unordered_map<char, std::string> squareMap;
        
        // Mapeamento para Strikethrough (a̶b̶c̶...)
        static const std::unordered_map<char, std::string> strikethroughMap;
        
        // Mapeamento para Underline (a̲b̲c̲...)
        static const std::unordered_map<char, std::string> underlineMap;

    public:
        // Enum para diferentes estilos
        enum class Style {
            BOLD,
            ITALIC,
            BOLD_ITALIC,
            FRAKTUR,
            BOLD_FRAKTUR,
            DOUBLE_STRUCK,
            BUBBLE,
            BLACK_BUBBLE,
            MONOSPACE,
            SMALL_CAPS,
            TINY,
            UPSIDE_DOWN,
            FANCY_STYLE_1,
            FANCY_STYLE_2,
            MAGIC,
            FULL_WIDTH,
            SQUARE,
            STRIKETHROUGH,
            UNDERLINE,
            NORMAL
        };

        // Converter texto para um estilo específico
        static std::string convert(const std::string& text, Style style);
        
        // Converter texto para múltiplos estilos
        static std::vector<std::string> convertMultiple(const std::string& text, 
                                                        const std::vector<Style>& styles);
        
        // Obter nome do estilo
        static std::string getStyleName(Style style);
        
        // Obter todos os estilos disponíveis
        static std::vector<Style> getAllStyles();
        
        // Converter caractere individual
        static std::string convertChar(char c, Style style);
    };

    // ===== DIACRÍTICOS COMBINANTES =====
    
    class DiacriticsApplier {
    public:
        enum class DiacriticType {
            BRIDGE_ABOVE,      // ͆͆
            ASTERISK_BELOW,    // ͙͙
            PLUS_SIGN_BELOW,   // ̟̟
            X_ABOVE_BELOW,     // ͓͓̽̽
            BRIDGE_BELOW,      // ̺̺
            UPWARD_ARROW_BELOW,// ͎͎
            STRIKETHROUGH,     // ̶̶
            SLASH,             // ̷̷
            DOUBLE_UNDERLINE,  // ̳̳
            LOVE_HEARTS,       // ♥♥
            INVISIBLE_INK      // ҉҉
        };

        // Aplicar diacrítico a um texto
        static std::string applyDiacritic(const std::string& text, DiacriticType type);
        
        // Obter o caractere de diacrítico
        static std::string getDiacriticChar(DiacriticType type);
    };

    // ===== GERADORES ESPECIAIS =====
    
    class SpecialGenerators {
    public:
        // Gerar nome com espaço invisível (ㅤ)
        static std::string addInvisibleSpaces(const std::string& text, int spacing = 1);
        
        // Gerar nome com símbolos decorativos
        static std::string addDecorativeSymbols(const std::string& text, 
                                               const std::string& symbol = "✿");
        
        // Gerar nome com emojis aleatórios
        static std::string addRandomEmojis(const std::string& text);
        
        // Gerar nome com cores (formato Free Fire)
        static std::string addFFColor(const std::string& text, const std::string& colorCode);
        
        // Combinar múltiplos estilos
        static std::string combineStyles(const std::string& text, 
                                        const std::vector<StyleConverter::Style>& styles);
    };

    // ===== UTILITÁRIOS =====
    
    class Utils {
    public:
        // Verificar se é letra maiúscula
        static bool isUpperCase(char c);
        
        // Verificar se é letra minúscula
        static bool isLowerCase(char c);
        
        // Verificar se é número
        static bool isDigit(char c);
        
        // Verificar se é letra
        static bool isLetter(char c);
        
        // Converter para maiúscula
        static char toUpper(char c);
        
        // Converter para minúscula
        static char toLower(char c);
        
        // Limpar espaços extras
        static std::string trimSpaces(const std::string& text);
        
        // Validar comprimento para Free Fire (máximo 50 caracteres)
        static bool isValidFFLength(const std::string& text);
        
        // Obter comprimento efetivo (considerando caracteres especiais)
        static int getEffectiveLength(const std::string& text);
    };

} // namespace TextStyle
