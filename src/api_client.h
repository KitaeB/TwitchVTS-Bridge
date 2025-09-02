#pragma once
#include <string>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <cpr/api.h>
#include <cpr/response.h>

#include <httplib.h>


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace basio = boost::asio;
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

private:
    // Boost.Asio и Boost.Beast объекты для HTTP-запросов
    basio::io_context ioc;
    basio::ip::tcp::resolver resolver{ioc};
    beast::websocket::stream<basio::ip::tcp::socket> ws{ioc};
    
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
    void updateAccessToken();
    void createReward(json param);

    json getBroadcastInfo(); // Получение информации о пользователе

    json getCustomRewards(); // Получение кастомных наград
    void updateCustomReward(const std::string& reward_id, const std::string& title, const std::string& prompt, int cost, bool is_enabled); // Обновление кастомной награды




private:
    std::string client_id = "bt1iiov5efszxf1dbgcs2rq7gvvfdy";
    std::string client_secret = "t0yfrmyozsgy44abn1e6bhhppp7a26";
    std::string redirect_uri = "http://localhost:30101/callback";
    std::string scope = "channel:manage:redemptions+channel:read:redemptions";
    std::string auth_url = "https://id.twitch.tv/oauth2/authorize";

    std::string code;
    std::string RefreshToken;
    std::string AccessToken;
    std::string broadcast_id;


};