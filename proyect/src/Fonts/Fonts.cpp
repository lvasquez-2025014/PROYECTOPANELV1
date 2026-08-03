#include "Fonts.hpp"
#include "FontAwesome.hpp"
#include "FontsTwo.hpp"
#include "FontInter.hpp"


namespace FWork {
    void Fonts::Initialize(ID3D11Device* Device) {
        ImGuiIO& io = ImGui::GetIO();
        
        // ===== CONFIGURAÇO DE SUPORTE A UNICODE MÁXIMO =====
        // Suporta TODOS os caracteres possíveis: acentos, kanji, chinês, árabe, cirílico, símbolos, etc.
        
        ImFontConfig FontAwesomeConfig;
        FontAwesomeConfig.GlyphMinAdvanceX = 25.f * (2.0f / 3.0f);

        static const ImWchar IconRanges[] = {
            ICON_MIN_FA, ICON_MAX_FA, 0
        };

        // ===== CONFIGURAÇO PARA SUPORTE A UNICODE COMPLETO E MÁXIMO =====
        ImFontConfig unicodeFontConfig;
        unicodeFontConfig.OversampleH = 4;  // Aumentado para melhor qualidade
        unicodeFontConfig.OversampleV = 4;  // Aumentado para melhor qualidade
        unicodeFontConfig.PixelSnapH = false;
        unicodeFontConfig.RasterizerMultiply = 1.0f;
        
        // ===== RANGES DE CARACTERES UNICODE MÁXIMOS =====
        // Cobre TODOS os caracteres Unicode possíveis
        static const ImWchar unicodeRanges[] = {
            // ASCII e Latin
            0x0020, 0x00FF,  // ASCII básico e Latin-1 (acentos)
            0x0100, 0x017F,  // Latin Extended-A
            0x0180, 0x024F,  // Latin Extended-B
            0x0250, 0x02AF,  // IPA Extensions
            0x1E00, 0x1EFF,  // Latin Extended Additional
            
            // Grego
            0x0370, 0x03FF,  // Grego
            0x1F00, 0x1FFF,  // Grego Extended
            
            // Cirílico
            0x0400, 0x04FF,  // Cirílico
            0x0500, 0x052F,  // Cirílico Suplementar
            0x1C80, 0x1C8F,  // Cirílico Estendido-C
            0xA640, 0xA69F,  // Cirílico Estendido-B
            
            // Hebraico
            0x0590, 0x05FF,  // Hebraico
            
            // Árabe
            0x0600, 0x06FF,  // Árabe
            0x0750, 0x077F,  // Árabe Suplementar
            0xFB50, 0xFDFF,  // Árabe Apresentação-A
            0xFE70, 0xFEFF,  // Árabe Apresentação-B
            
            // Síraco
            0x0700, 0x074F,  // Síraco
            
            // Thaana
            0x0780, 0x07BF,  // Thaana
            
            // Devanagari (Hindi)
            0x0900, 0x097F,  // Devanagari
            
            // Bengali
            0x0980, 0x09FF,  // Bengali
            
            // Gurmukhi (Punjabi)
            0x0A00, 0x0A7F,  // Gurmukhi
            
            // Gujarati
            0x0A80, 0x0AFF,  // Gujarati
            
            // Oriya
            0x0B00, 0x0B7F,  // Oriya
            
            // Tamil
            0x0B80, 0x0BFF,  // Tamil
            
            // Telugu
            0x0C00, 0x0C7F,  // Telugu
            
            // Kannada
            0x0C80, 0x0CFF,  // Kannada
            
            // Malayalam
            0x0D00, 0x0D7F,  // Malayalam
            
            // Sinhala
            0x0D80, 0x0DFF,  // Sinhala
            
            // Thai
            0x0E00, 0x0E7F,  // Thai
            
            // Lao
            0x0E80, 0x0EFF,  // Lao
            
            // Tibetano
            0x0F00, 0x0FFF,  // Tibetano
            
            // Myanmar
            0x1000, 0x109F,  // Myanmar
            
            // Georgiano
            0x10A0, 0x10FF,  // Georgiano
            
            // Hangul (Coreano)
            0x1100, 0x11FF,  // Hangul Jamo
            0x3130, 0x318F,  // Hangul Compatibility Jamo
            0xA960, 0xA97F,  // Hangul Jamo Extended-A
            0xD7B0, 0xD7FF,  // Hangul Jamo Extended-B
            0xAC00, 0xD7AF,  // Hangul Syllables
            
            // Etíope
            0x1200, 0x137F,  // Etíope
            0x1380, 0x139F,  // Etíope Suplementar
            
            // Cherokee
            0x13A0, 0x13FF,  // Cherokee
            
            // Silábico Unificado Canadense
            0x1400, 0x167F,  // Silábico Unificado Canadense
            
            // Ogham
            0x1680, 0x169F,  // Ogham
            
            // Rúnico
            0x16A0, 0x16FF,  // Rúnico
            
            // Tagalog
            0x1700, 0x171F,  // Tagalog
            
            // Hanunoo
            0x1720, 0x173F,  // Hanunoo
            
            // Buhid
            0x1740, 0x175F,  // Buhid
            
            // Tagbanwa
            0x1760, 0x177F,  // Tagbanwa
            
            // Khmer
            0x1780, 0x17FF,  // Khmer
            0x19E0, 0x19FF,  // Khmer Symbols
            
            // Mongol
            0x1800, 0x18AF,  // Mongol
            
            // Limbu
            0x1900, 0x194F,  // Limbu
            
            // Tai Le
            0x1950, 0x197F,  // Tai Le
            
            // Nova Tai Lue
            0x1980, 0x19DF,  // Nova Tai Lue
            
            // Buginese
            0x1A00, 0x1A1F,  // Buginese
            
            // Balinese
            0x1B00, 0x1B7F,  // Balinese
            
            // Sundanese
            0x1B80, 0x1BBF,  // Sundanese
            
            // Lepcha
            0x1C00, 0x1C4F,  // Lepcha
            
            // Ol Chiki
            0x1C50, 0x1C7F,  // Ol Chiki
            
            // Modificadores Fonéticos
            0x1D00, 0x1D7F,  // Modificadores Fonéticos
            0x1D80, 0x1DBF,  // Modificadores Fonéticos Suplementares
            0x1D00, 0x1D7F,  // Mathematical Alphanumeric Symbols (letras em estilos diferentes)
            0x1D80, 0x1DBF,  // Mathematical Alphanumeric Symbols Extended
            
            // Caracteres Invisíveis e Zero-Width
            0x200B, 0x200F,  // Zero-Width Characters (espaços invisíveis)
            0x2060, 0x2064,  // Invisible Characters
            0xFEFF, 0xFEFF,  // Zero-Width No-Break Space
            
            // Pontuação Geral
            0x2000, 0x206F,  // Pontuação Geral (inclui espaços especiais)
            
            // Sobrescrito e Subscrito
            0x2070, 0x209F,  // Sobrescrito e Subscrito
            
            // Símbolos de Moeda
            0x20A0, 0x20CF,  // Símbolos de Moeda
            
            // Combinando Diacríticos para Símbolos
            0x20D0, 0x20FF,  // Combinando Diacríticos para Símbolos
            
            // Letras-Símbolo
            0x2100, 0x214F,  // Letras-Símbolo
            
            // Numerais Romanos
            0x2150, 0x218F,  // Numerais Romanos
            
            // Setas e Símbolos Matemáticos
            0x2190, 0x27FF,  // Setas e Símbolos Matemáticos
            
            // Braille
            0x2800, 0x28FF,  // Braille
            
            // Setas Suplementares
            0x2900, 0x297F,  // Setas Suplementares
            
            // Símbolos Diversos
            0x2B00, 0x2E7F,  // Símbolos Diversos
            
            // CJK (Chinês, Japonês, Coreano)
            0x2E80, 0x2EFF,  // Radical CJK
            0x2F00, 0x2FDF,  // Kangxi Radicals
            0x2FF0, 0x2FFF,  // Ideographic Description Characters
            0x3000, 0x303F,  // CJK Symbols and Punctuation
            0x3040, 0x309F,  // Hiragana (Japonês)
            0x30A0, 0x30FF,  // Katakana (Japonês)
            0x3100, 0x312F,  // Bopomofo (Chinês)
            0x3190, 0x319F,  // Kanbun
            0x31A0, 0x31BF,  // Bopomofo Extended
            0x31C0, 0x31EF,  // CJK Strokes
            0x31F0, 0x31FF,  // Katakana Phonetic Extensions
            0x3200, 0x32FF,  // Enclosed CJK Letters and Months
            0x3300, 0x33FF,  // CJK Compatibility
            0x3400, 0x4DBF,  // CJK Unified Ideographs Extension A
            0x4DC0, 0x4DFF,  // Yijing Hexagram Symbols
            0x4E00, 0x9FFF,  // CJK Unified Ideographs (Kanji/Hanzi)
            0xF900, 0xFAFF,  // CJK Compatibility Ideographs
            
            // Yi
            0xA000, 0xA48F,  // Yi Syllables
            0xA490, 0xA4CF,  // Yi Radicals
            
            // Lisu
            0xA4D0, 0xA4FF,  // Lisu
            
            // Vai
            0xA500, 0xA63F,  // Vai
            
            // Bamum
            0xA6A0, 0xA6FF,  // Bamum
            
            // Ligaduras Alfabéticas
            0xFB00, 0xFB4F,  // Ligaduras Alfabéticas
            
            // Variação de Seletores
            0xFE00, 0xFE0F,  // Variação de Seletores
            
            // Formas Verticais
            0xFE10, 0xFE1F,  // Formas Verticais
            
            // Combinando Meio Diacríticos
            0xFE20, 0xFE2F,  // Combinando Meio Diacríticos
            
            // Compatibilidade CJK
            0xFE30, 0xFE4F,  // Compatibilidade CJK
            
            // Formas Pequenas
            0xFE50, 0xFE6F,  // Formas Pequenas
            
            // Meia Largura e Largura Completa
            0xFF00, 0xFFEF,  // Meia Largura e Largura Completa
            
            // Caracteres Suplementares (Emojis e símbolos avançados)
            0x1F000, 0x1F02F,  // Símbolos Mahjong
            0x1F030, 0x1F09F,  // Símbolos Dominó
            0x1F0A0, 0x1F0FF,  // Emojis Diversos
            0x1F100, 0x1F64F,  // Emojis Variados
            0x1F680, 0x1F6FF,  // Transporte e Símbolos de Mapa
            0x1F700, 0x1F77F,  // Símbolos Alquímicos
            0x1F780, 0x1F7FF,  // Símbolos Geométricos Estendidos
            0x1F800, 0x1F8FF,  // Setas Suplementares-C
            0x1F900, 0x1F9FF,  // Emojis Suplementares
            
            // Caracteres Especiais Avançados
            0x1D400, 0x1D7FF,  // Mathematical Alphanumeric Symbols (letras em estilos: bold, italic, serif, etc.)
            0x1D800, 0x1DAAF,  // Sutton SignWriting
            0x1E000, 0x1E02F,  // Glagolitic Supplement
            0x1E100, 0x1E14F,  // Nyiakeng Puachue Hmong
            0x1E800, 0x1E8DF,  // Mende Kikakui
            0x1E900, 0x1E95F,  // Adlam
            
            // Mais Caracteres Especiais
            0xFFF0, 0xFFFF,  // Specials (caracteres especiais diversos)
            
            0x0000, 0x0000   // Terminator
        };
        
        // ===== USAR O ARRAY COMPLETO DE UNICODE =====
        // IMPORTANTE: Usar o array definido, não apenas GetGlyphRangesDefault()
        unicodeFontConfig.GlyphRanges = unicodeRanges;

        // Carrega as fontes Inter com suporte MÁXIMO a Unicode
        InterBlack = io.Fonts->AddFontFromMemoryCompressedTTF(InterBlack_compressed_data, InterBlack_compressed_size, 14, &unicodeFontConfig);
        InterBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 17, &unicodeFontConfig);
        InterBold12 = io.Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 15, &unicodeFontConfig);
        InterExtraBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterExtraBold_compressed_data, InterExtraBold_compressed_size, 13, &unicodeFontConfig);
        InterExtraLight = io.Fonts->AddFontFromMemoryCompressedTTF(InterExtraLight_compressed_data, InterExtraLight_compressed_size, 14, &unicodeFontConfig);
        InterLight = io.Fonts->AddFontFromMemoryCompressedTTF(InterLight_compressed_data, InterLight_compressed_size, 12, &unicodeFontConfig);
        InterMedium = io.Fonts->AddFontFromMemoryCompressedTTF(InterMedium_compressed_data, InterMedium_compressed_size, 17, &unicodeFontConfig);
        InterRegular = io.Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, 17, &unicodeFontConfig);
        InterRegular14 = io.Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, 15, &unicodeFontConfig);
        InterSemiBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterSemiBold_compressed_data, InterSemiBold_compressed_size, 16, &unicodeFontConfig);
        InterThin = io.Fonts->AddFontFromMemoryCompressedTTF(InterThin_compressed_data, InterThin_compressed_size, 14, &unicodeFontConfig);

        GeistRegular = io.Fonts->AddFontFromMemoryCompressedTTF(GeistRegular_compressed_data, GeistRegular_compressed_size, 16, &unicodeFontConfig);
        GeistRegularMedium = io.Fonts->AddFontFromMemoryCompressedTTF(GeistRegular_compressed_data, GeistRegular_compressed_size, 18, &unicodeFontConfig);
        GeistMedium = io.Fonts->AddFontFromMemoryCompressedTTF(GeistRegular_compressed_data, GeistRegular_compressed_size, 14, &unicodeFontConfig);
        GeistBold = io.Fonts->AddFontFromMemoryCompressedTTF(GeistBold_compressed_data, GeistBold_compressed_size, 36, &unicodeFontConfig);
        GeistBoldMedium = io.Fonts->AddFontFromMemoryCompressedTTF(GeistBold_compressed_data, GeistBold_compressed_size, 16, &unicodeFontConfig);

        ImFontConfig customFontConfig;
        customFontConfig.MergeMode = true;
        customFontConfig.OversampleH = 1;
        customFontConfig.OversampleV = 1;
        customFontConfig.PixelSnapH = true;

        static const ImWchar customRanges[] = { 0xe000, 0xe204, 0x00 };
        IconWeapon = io.Fonts->AddFontFromMemoryCompressedTTF(weapon_compressed_data, weapon_compressed_size, 41.0f, &customFontConfig, customRanges);


        FontAwesomeRegular = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeRegular_compressed_data, FontAwesomeRegular_compressed_size, 25.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);
        FontAwesomeSolid = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 27.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);
        FontAwesomeSolid18 = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 18.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);
        FontAwesomeSolidBig = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 30.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);

        // ===== COMPILAR ATLAS DE FONTES COM SUPORTE MÁXIMO A UNICODE =====
        // Isso garante que TODOS os caracteres Unicode sejam renderizados corretamente
        io.Fonts->Build();

        //D3DX11CreateShaderResourceViewFromMemory(Device, LogoCirco, sizeof(LogoCirco), NULL, NULL, &Logo, NULL);

        BindImGuiGlobals();
    }
}
