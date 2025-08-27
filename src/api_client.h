#pragma once

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <thread>
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

    void Connect();                           // Переподключение
    
    // Запросы к API
    void ApiStateRequest();              // Состояние API
    std::string AuthenticationTokenRequest();   // Получение токена аутентификации
    bool AuthenticateRequest(const std::string& token); // Аутентификация с токеном
    void AvailableModelsRequest();       // Доступные модели
    void CurrentModelRequest();          // Текущая модель

    void send(const std::string& message);
    void message_handler(std::function<void(std::string)> handler);
    // Управления подписками
    void Subscribe();
    void unSubscribe();



private:
    void do_read();

    // Boost.Asio и Boost.Beast объекты для HTTP-запросов
    asio::io_context ioc;
    asio::ip::tcp::resolver resolver {ioc};
    websocket::stream<tcp::socket> ws {ioc};
    
    
    // Параметры подключения по умолчанию
    std::string host = "localhost";
    int port = 8001;
    std::string token;  // Токен аутентификации
    
    // Ассинхронный поток
    std::thread thread_;
    std::mutex mutex_;
    beast::flat_buffer buffer_;
    std::function<void(std::string)> message_handler_;


};

class TwitchClient {
public:

private:
};