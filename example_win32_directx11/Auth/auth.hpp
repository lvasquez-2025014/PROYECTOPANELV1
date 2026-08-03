#pragma once
#define CURL_STATICLIB
#include "Auth\Curl\curl.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <atlsecurity.h>
#include "json.hpp"
#include "StringObfuscator.hpp"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Wldap32.lib")

using json = nlohmann::json;

class c_api {
private:
    // STRINGS SENSIVEIS OFUSCADAS
    static constexpr auto keyauth_api_obfuscated = OBFUSCATE("https://keyauth.win/api/1.2/");
    static constexpr auto name_obfuscated = OBFUSCATE("Application");
    static constexpr auto ownerid_obfuscated = OBFUSCATE("ownerid");
    static constexpr auto secret_obfuscated = OBFUSCATE("secret");
    static constexpr auto version_obfuscated = OBFUSCATE("1.0");
    
    std::string GetKeyAuthApi() const {
        return keyauth_api_obfuscated.decrypt();
    }
    
    std::string GetName() const {
        return name_obfuscated.decrypt();
    }
    
    std::string GetOwnerId() const {
        return ownerid_obfuscated.decrypt();
    }
    
    std::string GetSecret() const {
        return secret_obfuscated.decrypt();
    }
    
    std::string GetVersion() const {
        return version_obfuscated.decrypt();
    }

    std::string sessionid;

    static inline auto write_callback(void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static std::string get_hwid() {
        std::string hwid_str = "";
        ATL::CAccessToken accessToken;
        ATL::CSid currentUserSid;
        if (accessToken.GetProcessToken(TOKEN_READ | TOKEN_QUERY) && accessToken.GetUser(&currentUserSid)) {
            hwid_str = std::string(CT2A(currentUserSid.Sid()));
        }
        DWORD volSerial = 0;
        if (GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0)) {
            hwid_str += "-" + std::to_string(volSerial);
        }
        if (hwid_str.length() < 20) {
            hwid_str += "-LAMAFIA-AUTH-SECURE-ID-FIX";
        }
        return hwid_str;
    }

    std::string perform_request(const std::string& postfields) {
        std::string response;
        CURL* hnd = curl_easy_init();
        if (!hnd) {
            return "failed to start connection.";
        }
        std::string url = GetKeyAuthApi();
        curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, ("POST"));
        curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Content-Type: application/x-www-form-urlencoded"));
        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, postfields.c_str());
        CURLcode ret = curl_easy_perform(hnd);
        curl_slist_free_all(headers);
        curl_easy_cleanup(hnd);
        if (ret != CURLE_OK) {
            return ("failed to make request to server: ") + std::string(curl_easy_strerror(ret));
        }
        return response;
    }

public:
    c_api() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~c_api() {
        curl_global_cleanup();
    }

    struct stc_client {
        std::string username;
        std::string password;
        std::string hwid;
        struct stc_sub_type {
            bool active;
            std::string expire_date;
        } sub_type;
    } client;

    struct response_t {
        bool success;
        std::string message;
    } response;

    inline auto setup() -> std::string {
        std::string version = GetVersion();
        std::string name = GetName();
        std::string ownerid = GetOwnerId();
        
        std::string postfields = ("type=init&ver=") + version +
            ("&name=") + name +
            ("&ownerid=") + ownerid;

        std::string api_response = perform_request(postfields);

        try {
            json response_json = json::parse(api_response);
            if (response_json["success"].get<bool>()) {
                sessionid = response_json["sessionid"].get<std::string>();
                response = { true, "success" };
                return "success";
            }
            response = { false, response_json["message"].get<std::string>() };
            return response_json["message"].get<std::string>();
        }
        catch (json::parse_error&) {
            response = { false, "failed to parse server response." };
            return "failed to parse server response.";
        }
    }

    inline auto Login(const std::string username, const std::string password) -> std::string {
        if (sessionid.empty()) {
            response = { false, "Session not initialized. Call setup() first." };
            return "Session not initialized. Call setup() first.";
        }

        std::string name = GetName();
        std::string ownerid = GetOwnerId();
        
        std::string hwid = get_hwid();
        std::string postfields = ("type=login&username=") + username +
            ("&pass=") + password +
            ("&hwid=") + hwid +
            ("&sessionid=") + sessionid +
            ("&name=") + name +
            ("&ownerid=") + ownerid;

        std::string api_response = perform_request(postfields);

        try {
            json response_json = json::parse(api_response);
            if (response_json["success"].get<bool>()) {
                client.username = response_json["info"]["username"].get<std::string>();
                client.hwid = hwid;

                if (response_json["info"].contains("subscriptions")) {
                    auto sub = response_json["info"]["subscriptions"][0];
                    client.sub_type.expire_date = sub["expiry"].get<std::string>();
                    client.sub_type.active = true;
                }
                response = { true, "success" };
                return "success";
            }
            response = { false, response_json["message"].get<std::string>() };
            return response_json["message"].get<std::string>();
        }
        catch (json::parse_error& e) {
            response = { false, "Erro ao processar resposta: " + std::string(e.what()) };
            return "Erro ao processar resposta: " + std::string(e.what());
        }
    }

    inline auto Register_key(const std::string username, const std::string password, const std::string& key) -> std::string {
        if (sessionid.empty()) {
            response = { false, "Session not initialized. Call setup() first." };
            return "Session not initialized. Call setup() first.";
        }

        std::string name = GetName();
        std::string ownerid = GetOwnerId();
        
        std::string hwid = get_hwid();
        std::string postfields = ("type=register&username=") + username +
            ("&pass=") + password +
            ("&key=") + key +
            ("&hwid=") + hwid +
            ("&sessionid=") + sessionid +
            ("&name=") + name +
            ("&ownerid=") + ownerid;

        std::string api_response = perform_request(postfields);

        try {
            json response_json = json::parse(api_response);
            if (response_json["success"].get<bool>()) {
                client.username = username;
                client.hwid = hwid;

                if (response_json["info"].contains("subscriptions")) {
                    auto sub = response_json["info"]["subscriptions"][0];
                    client.sub_type.expire_date = sub["expiry"].get<std::string>();
                    client.sub_type.active = true;
                }
                response = { true, "success" };
                return "success";
            }
            response = { false, response_json["message"].get<std::string>() };
            return response_json["message"].get<std::string>();
        }
        catch (json::parse_error& e) {
            response = { false, "Erro ao processar resposta: " + std::string(e.what()) };
            return "Erro ao processar resposta: " + std::string(e.what());
        }
    }

    std::string parse_date_dual(const std::string& timestamp) {
        time_t t = std::stoll(timestamp);
        std::tm tm_utc = {}, tm_local = {};
        gmtime_s(&tm_utc, &t);
        localtime_s(&tm_local, &t);

        char buffer_utc[100];
        std::strftime(buffer_utc, sizeof(buffer_utc), "%d/%m/%Y %H:%M:%S", &tm_utc);
        return std::string("Expira: ") + buffer_utc;
    }
};

inline c_api g_Api;
