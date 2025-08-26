#include "api_client.h"
#include <cassert>
#include <string>


#pragma region VTS

VTSClient::VTSClient()
    : resolver(ioc), ws(ioc) {
    // Конструктор
    auto results = resolver.resolve(host, std::to_string(port));

    asio::connect(ws.next_layer(), results.begin(), results.end());
    beast::error_code ec;
    ws.handshake(host, "/", ec);
    if(ec) {
        std::cerr << "Handshake error: " << ec.value() << " - " << ec.message() << std::endl;
        throw std::runtime_error(ec.message());
    }
}

VTSClient::~VTSClient() {
    // Деструктор
    beast::error_code ec;
    if (ws.is_open()) {
        ws.close(beast::websocket::close_code::normal, ec);
        if(ec) {
            std::cerr << "Error closing WebSocket: " << ec.message() << std::endl;
        }
    }
}

void VTSClient::setPort(int port) {
    this->port = port;
}
void VTSClient::setHost(const std::string& host) {
    this->host = host;
}

void VTSClient::reconnect() {
    // Переподключение
    beast::error_code ec;
    if (ws.is_open()) {
        ws.close(beast::websocket::close_code::normal, ec);
        if(ec) {
            std::cerr << "Error closing WebSocket (" << ec.value() << "): " << ec.message() << std::endl;
        }
    }
    asio::ip::tcp::resolver::results_type results = resolver.resolve(host, std::to_string(port));
    asio::connect(ws.next_layer(), results.begin(), results.end());
    ws.handshake(host, "/");
}

std::string VTSClient::ApiStateRequest() {
    // Состояние API
    if (!ws.is_open()) {
        std::cout << "WebSocket is not open. Reconnecting..." << std::endl;
        reconnect();
    }
    std::string request =   R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"APIStateRequest"
                            })";

    ws.write(asio::buffer(request));
    
    beast::flat_buffer buffer;
    ws.read(buffer);
    
    return beast::buffers_to_string(buffer.data());
}

std::string VTSClient::AvailableModelsRequest() {
    // Доступные модели
    if (!ws.is_open()) {
        std::cout << "WebSocket is not open. Reconnecting..." << std::endl;
        reconnect();
    }
    std::string request =   R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"AvailableModelsRequest"
                            })";
                                
    ws.write(asio::buffer(request));
    
    beast::flat_buffer buffer;
    ws.read(buffer);
    
    return beast::buffers_to_string(buffer.data());
}

std::string VTSClient::CurrentModelRequest() {
    // Текущая модель
    if (!ws.is_open()) {
        std::cout << "WebSocket is not open. Reconnecting..." << std::endl;
        reconnect();
    }
    std::string request =   R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"CurrentModelRequest"
                            })";
                                
    ws.write(asio::buffer(request));
    
    beast::flat_buffer buffer;
    ws.read(buffer);
    
    return beast::buffers_to_string(buffer.data());
}
#pragma endregion