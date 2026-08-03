#pragma once
#include <string>
#include <cstring>
#include <algorithm>
#include <array>

namespace StringObfuscation {
    // ===== CHAVE DE CRIPTOGRAFIA FIXA =====
    // Mude isso para uma chave única do seu projeto
    constexpr unsigned char ENCRYPTION_KEY = 0xA7;
    
    // ===== CLASSE DE OFUSCAÇO DE STRINGS =====
    // Criptografa strings em tempo de compilação
    template<size_t N>
    class ObfuscatedString {
    private:
        std::array<unsigned char, N> encrypted_data;
        size_t length;
        
    public:
        // Construtor que criptografa a string em tempo de compilação
        constexpr ObfuscatedString(const char (&str)[N]) : length(N - 1) {
            // Copia e criptografa cada caractere
            for (size_t i = 0; i < N - 1; ++i) {
                encrypted_data[i] = static_cast<unsigned char>(str[i]) ^ ENCRYPTION_KEY;
            }
            encrypted_data[N - 1] = 0; // Null terminator
        }
        
        // Descriptografa a string em tempo de execução
        std::string decrypt() const {
            std::string result;
            result.reserve(length);
            
            for (size_t i = 0; i < length; ++i) {
                result += static_cast<char>(encrypted_data[i] ^ ENCRYPTION_KEY);
            }
            
            return result;
        }
        
        // Retorna a string descriptografada
        operator std::string() const {
            return decrypt();
        }
        
        // Comparação com string normal
        bool operator==(const std::string& other) const {
            return decrypt() == other;
        }
        
        bool operator!=(const std::string& other) const {
            return decrypt() != other;
        }
    };
    
    // ===== MACRO PARA FACILITAR USO =====
    // Uso: auto encrypted = OBFUSCATE("minha string sensível");
    // Depois: std::string decrypted = encrypted.decrypt();
    #define OBFUSCATE(str) StringObfuscation::ObfuscatedString<sizeof(str)>(str)
    
    // ===== FUNÇO DE CRIPTOGRAFIA EM TEMPO DE EXECUÇO =====
    // Para strings dinâmicas que não podem ser compiladas
    inline std::string EncryptString(const std::string& plaintext) {
        std::string encrypted;
        encrypted.reserve(plaintext.length());
        
        for (char c : plaintext) {
            encrypted += static_cast<char>(static_cast<unsigned char>(c) ^ ENCRYPTION_KEY);
        }
        
        return encrypted;
    }
    
    // ===== FUNÇO DE DESCRIPTOGRAFIA EM TEMPO DE EXECUÇO =====
    inline std::string DecryptString(const std::string& encrypted) {
        std::string plaintext;
        plaintext.reserve(encrypted.length());
        
        for (char c : encrypted) {
            plaintext += static_cast<char>(static_cast<unsigned char>(c) ^ ENCRYPTION_KEY);
        }
        
        return plaintext;
    }
    
    // ===== CLASSE PARA ARMAZENAR STRINGS SENSÍVEIS =====
    // Mantém strings criptografadas na memória
    class SecureString {
    private:
        std::string encrypted_value;
        
    public:
        // Construtor com string criptografada
        SecureString(const std::string& encrypted) : encrypted_value(encrypted) {}
        
        // Construtor que criptografa a string
        static SecureString Create(const std::string& plaintext) {
            return SecureString(EncryptString(plaintext));
        }
        
        // Obtém a string descriptografada (apenas quando necessário)
        std::string Get() const {
            return DecryptString(encrypted_value);
        }
        
        // Comparação segura
        bool Equals(const std::string& other) const {
            return Get() == other;
        }
        
        // Limpa a memória (sobrescreve com zeros)
        void Clear() {
            std::fill(encrypted_value.begin(), encrypted_value.end(), 0);
            encrypted_value.clear();
        }
        
        // Destrutor que limpa a memória
        ~SecureString() {
            Clear();
        }
    };
    
    // ===== CLASSE PARA MÚLTIPLAS CHAVES =====
    // Oferece maior segurança com chaves diferentes por string
    template<unsigned char KEY>
    class ObfuscatedStringWithKey {
    private:
        std::string encrypted_data;
        
    public:
        // Construtor que criptografa com chave específica
        ObfuscatedStringWithKey(const std::string& str) {
            encrypted_data.reserve(str.length());
            for (char c : str) {
                encrypted_data += static_cast<char>(static_cast<unsigned char>(c) ^ KEY);
            }
        }
        
        // Descriptografa com a mesma chave
        std::string decrypt() const {
            std::string result;
            result.reserve(encrypted_data.length());
            
            for (char c : encrypted_data) {
                result += static_cast<char>(static_cast<unsigned char>(c) ^ KEY);
            }
            
            return result;
        }
        
        operator std::string() const {
            return decrypt();
        }
    };
    
    // ===== GERADOR DE CHAVES SIMPLES =====
    // Cria chaves únicas baseadas em strings
    inline unsigned char GenerateKey(const std::string& seed) {
        unsigned char key = 0;
        for (char c : seed) {
            key ^= static_cast<unsigned char>(c);
            key = (key << 1) | (key >> 7); // Rotação
        }
        return key;
    }
}

// ===== EXEMPLO DE USO =====
/*
// Strings ofuscadas em tempo de compilação
auto api_url = OBFUSCATE("https://keyauth.win/api/1.2/");
auto app_name = OBFUSCATE("C4532167a's Application");
auto owner_id = OBFUSCATE("qVnXIaYMIE");
auto secret = OBFUSCATE("7d1b68cb6f3020555b9382f17257cf1a010c6cfcb4f1029403d3b96385478cae");

// Usar as strings
std::string url = api_url.decrypt();
std::string name = app_name.decrypt();

// Ou usar diretamente
std::string owner = owner_id;  // Converte automaticamente

// Para strings dinâmicas
std::string user_input = "algum valor";
auto encrypted = StringObfuscation::SecureString::Create(user_input);
std::string decrypted = encrypted.Get();  // Descriptografa quando necessário
*/
