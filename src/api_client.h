#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <cpr/cpr.h>
#include <httplib.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using json = nlohmann::json;

class VTSClient {
public:
    VTSClient();
    ~VTSClient();

    // Настройка подключения
    void setPort(int port);                     // Установка порта
    void setHost(const std::string& host);      // Установка хоста
    
    // Запросы к API
    json ApiStateRequest();              // Состояние API
    std::string AuthenticationTokenRequest();   // Получение токена аутентификации
    bool AuthenticateRequest(const std::string& token); // Аутентификация с токеном
    json AvailableModelsRequest();       // Доступные модели
    json CurrentModelRequest();          // Текущая модель

    // Управления подписками
    bool Subscribe();
    bool unSubscribe();


private:
    void do_read();

    // Boost.Asio и Boost.Beast объекты для HTTP-запросов
    asio::io_context ioc;
    asio::ip::tcp::resolver resolver{ioc};
    beast::websocket::stream<asio::ip::tcp::socket> ws{ioc};
    
    // Параметры подключения по умолчанию
    std::string host = "localhost";
    int port = 8001;
    std::string token;  // Токен аутентификации   

};

class TwitchClient {
public:
    TwitchClient();
    ~TwitchClient();

    void getAccessToken();


private:
    std::string client_id = "x4h6z4f3b1z8y3b5z1y9r0n2f5w8a1";
    std::string client_secret = "8271hcy8jff62wv4w4btgo3mxkqgb2";
    std::string redirect_uri = "https://localhost:30101/callback";
    std::string scope = "channel:manage:polls channel:read:polls";
    std::string auth_url = "https://id.twitch.tv/oauth2/authorize";
    std::string token;

};