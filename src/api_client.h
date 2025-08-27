#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <thread>
#include <future>
#include <queue> 
#include <mutex>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using json = nlohmann::json;

class VTSClient {
public:
    VTSClient();
    ~VTSClient();

    // Настройка подключения
    void setPort(int port);                     // Установка порта
    void setHost(const std::string& host);      // Установка хоста

    void reconnect();                           // Переподключение
    
    // Запросы к API
    json ApiStateRequest();              // Состояние API
    std::string AuthenticationTokenRequest();   // Получение токена аутентификации
    bool AuthenticateRequest(const std::string& token); // Аутентификация с токеном
    json AvailableModelsRequest();       // Доступные модели
    json CurrentModelRequest();          // Текущая модель

    // Управления подписками
    bool Subscribe();
    bool unSubscribe();

    // 

private:
    // Boost.Asio и Boost.Beast объекты для HTTP-запросов
    asio::io_context ioc;
    asio::ip::tcp::resolver resolver {ioc};
    websocket::stream<tcp::socket> ws {ioc};
    
    
    // Параметры подключения по умолчанию
    std::string host = "localhost";
    int port = 8001;
    std::string token;  // Токен аутентификации   

};

class TwitchClient {
public:

private:
};