#include "api_client.h"
#include <httplib.h>

#include <boost/intrusive/options.hpp>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#

#pragma region VTS

// Конструктор и деструктор клиента VTS
VTSClient::VTSClient() {
    
    auto results = resolver.resolve(host, std::to_string(port));

    asio::connect(ws.next_layer(), results);
    beast::error_code ec;
    ws.handshake(host + ":" + std::to_string(port), "/", ec);
    if (ec) {
        std::cerr << "Handshake error: " << ec.value() << " - " << ec.message() << std::endl;
        throw std::runtime_error(ec.message());
    }

    // Проверим наличие токена аутентификации в файле
    std::ifstream tokenFile("config");  // Файл для хранения токена
    if (tokenFile.is_open()) {
        std::getline(tokenFile, token);
        tokenFile.close();
    }
    // Если токен есть, попробуем аутентифицироваться
    if (token.empty() || (!token.empty() && !AuthenticateRequest(token))) {
        // Иначе запросим новый токен
        token = AuthenticationTokenRequest();

        // Сохраним токен в файл
        std::ofstream outFile("config");  // Файл для хранения токена
        if (outFile.is_open()) {
            outFile << token;
            outFile.close();
        }
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


// Запросы к API
json VTSClient::ApiStateRequest() {  // Состояние API, возвращает JSON-строку с состоянием
    std::string request = R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"APIStateRequest"
                            })";

    ws.write(asio::buffer(request));

    beast::flat_buffer buffer;
    ws.read(buffer);

    return json::parse(beast::buffers_to_string(buffer.data()))["data"];
}

std::string VTSClient::AuthenticationTokenRequest() {
    // Получение токена аутентификации
    std::string request = R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"AuthenticationTokenRequest",
                                "data":{
                                    "pluginName":"TwitchVTS-Bridge",
                                    "pluginDeveloper":"KitaeB",
                                    "pluginIcon":"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"
                                }
                            })";

    ws.write(asio::buffer(request));

    beast::flat_buffer buffer;
    ws.read(buffer);

    return json::parse(beast::buffers_to_string(buffer.data()))["data"]["authenticationToken"];
}

bool VTSClient::AuthenticateRequest(const std::string& token) {  // Аутентификация с применением токена, возвращает true/false
    std::string request = R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"AuthenticationRequest",
                                "data":{
                                    "pluginName": "TwitchVTS-Bridge",
                                    "pluginDeveloper": "KitaeB",
                                    "authenticationToken":")" +
                          token + R"("}
                            })";

    ws.write(asio::buffer(request));

    beast::flat_buffer buffer;
    ws.read(buffer);
    return json::parse(beast::buffers_to_string(buffer.data()))["data"]["authenticated"];
}

json VTSClient::AvailableModelsRequest() {  // Доступные модели, возвращает JSON-строку с моделями
    std::string request = R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"AvailableModelsRequest"
                            })";

    ws.write(asio::buffer(request));

    beast::flat_buffer buffer;
    ws.read(buffer);

    return json::parse(beast::buffers_to_string(buffer.data()))["data"];
}

json VTSClient::CurrentModelRequest() {  // Запрос текущей модели, возвращает JSON-строку с информацией о модели
    std::string request = R"({"apiName":"VTubeStudioPublicAPI",
                                "apiVersion":"1.0",
                                "requestID":"1",
                                "messageType":"CurrentModelRequest"
                            })";

    ws.write(asio::buffer(request));

    beast::flat_buffer buffer;
    ws.read(buffer);

    return json::parse(beast::buffers_to_string(buffer.data()))["data"];
}

// Управления подписками (Event)

bool VTSClient::Subscribe() {

    std::string request = R"({"apiName": "VTubeStudioPublicAPI",
                                "apiVersion": "1.0",
                                "requestID": "SomeID",
                                "messageType": "EventSubscriptionRequest",
                                "data": {
                                    "eventName": "ModelLoadedEvent",
                                    "subscribe": true,
                                    "config": {
                                    }
                                }
                        })";
    ws.write(asio::buffer(request));

    beast::flat_buffer buffer;
    ws.read(buffer);
    return !json::parse(beast::buffers_to_string(buffer.data()))["data"].contains("ErrorId");
}


bool VTSClient::unSubscribe() {

    std::string request = R"({"apiName": "VTubeStudioPublicAPI",
                                "apiVersion": "1.0",
                                "requestID": "SomeID",
                                "messageType": "EventSubscriptionRequest",
                                "data": {
                                    "eventName": "ModelLoadedEvent",
                                    "subscribe": false,
                                    "config": {
                                    }
                                }
                        })";
    ws.write(asio::buffer(request));

    beast::flat_buffer buffer;
    ws.read(buffer);
    return !json::parse(beast::buffers_to_string(buffer.data()))["data"].contains("ErrorId");
}

#pragma endregion

#pragma region Twitch

TwitchClient::TwitchClient() {
    // Конструктор
    //Читаем файл, если там есть токен, то используем его
    std::ifstream tokenFile("twitch_config");  // Файл для хранения токена
    if (tokenFile.is_open()) {
        std::getline(tokenFile, token);
        tokenFile.close();
    } else {
        // Иначе запрашиваем новый токен
        getAccessToken();
    }

    std::cout << "Twitch Token: " << token << std::endl;
}

TwitchClient::~TwitchClient() {
    // Деструктор
}

void TwitchClient::getAccessToken() {
    // Запрос токена доступа (Access Token) у Twitch
    httplib::Server srv;
    srv.Get("/callback", [&](const httplib::Request& req, httplib::Response& res) {

        std::string cmd = "start \"\" \"https://id.twitch.tv/oauth2/authorize?client_id=" + client_id +
                      "&redirect_uri=" + redirect_uri +
                      "&response_type=code&scope=channel:manage:redemptions\"";

        system(cmd.c_str());

        if(req.has_param("code")) {
            std::string code = req.get_param_value("code");

            cpr::Response r = cpr::Post(cpr::Url{"https://id.twitch.tv/oauth2/token"},
                                        cpr::Payload{
                                            {"client_id", client_id},
                                            {"client_secret", client_secret}, // Замените на ваш клиентский секрет
                                            {"code", code},
                                            {"grant_type", "authorization_code"},
                                            {"redirect_uri", redirect_uri}
                                        });

            if (r.status_code == 200) {
                auto response_json = json::parse(r.text);
                token = response_json["access_token"];

                // Сохраним токен в файл
                std::ofstream outFile("twitch_config");  // Файл для хранения токена
                if (outFile.is_open()) {
                    outFile << token;
                    outFile.close();
                }

                res.set_content("Authentication successful! You can close this window.", "text/plain");
            } else {
                res.set_content("Failed to get access token.", "text/plain");
            }
        } else {
            res.set_content("No code parameter found in the request.", "text/plain");
        }
    });

    srv.listen("localhost", 30101);
    
    //Запишем токен в файл
    std::ofstream outFile("twitch_config");  // Файл для хранения токена
    if (outFile.is_open()) {
        outFile << token;
        outFile.close();
    }
}