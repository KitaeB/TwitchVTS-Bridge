#pragma once

#include <string>
#include <iostream>


#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>

#pragma region VTS

class VTSClient {
public:
    VTSClient();
    ~VTSClient();

    // Настройка подключения
    void setPort(int port);                     // Установка порта
    void setHost(const std::string& host);      // Установка хоста

    void reconnect();                           // Переподключение
    
    // Запросы к API
    std::string ApiStateRequest();              // Состояние API
    std::string AvailableModelsRequest();       // Доступные модели
    std::string CurrentModelRequest();          // Текущая модель
    
private:
    // Boost.Asio и Boost.Beast объекты для HTTP-запросов
    asio::io_context ioc;
    asio::ip::tcp::resolver resolver;
    beast::websocket::stream<asio::ip::tcp::socket> ws;
    
    // Параметры подключения по умолчанию
    std::string host = "localhost";
    int port = 8001;

};

#pragma endregion
