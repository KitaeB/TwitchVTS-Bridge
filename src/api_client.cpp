#include "api_client.h"
#include <cpr/curl_container.h>
#include <cpr/parameters.h>
#include <httplib.h>
#include <shellapi.h>

#include <boost/intrusive/options.hpp>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>

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
        if (ec) {
            std::cerr << "Error closing WebSocket: " << ec.message() << std::endl;
        }
    }
}

void VTSClient::setPort(int port) { this->port = port; }
void VTSClient::setHost(const std::string& host) { this->host = host; }

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
#pragma endregion

#pragma region Twitch

TwitchClient::TwitchClient() {
    // Конструктор
    // Читаем файл, если там есть refresh токен, то используем его
    std::ifstream tokenFile("twitch_config");  // Файл для хранения токена
    if (tokenFile.is_open()) {
        std::getline(tokenFile, RefreshToken);
        tokenFile.close();
        // Обновляем Access Token
        updateAccessToken();
    } else {
        // Иначе запрашиваем новый токен
        getAccessToken();
    }
}

TwitchClient::~TwitchClient() {
    // Деструктор
}

void TwitchClient::getAccessToken() {
    // Запрос токена доступа (Access Token) у Twitch
    httplib::Server srv;
    srv.Get("/callback", [&](const httplib::Request& req, httplib::Response& res) {
        code = req.get_param_value("code");
        // HTML код для закрытия страницы
        std::string html = R"(
                    <!DOCTYPE html>
                    <html>
                    <head>
                        <title>Authorization Complete</title>
                        <meta charset="UTF-8">
                    </head>
                    <body>
                        <p>Вы успешно авторизованы! Окно закроется через 3 секунды.</p>
                        <script>
                            setTimeout(function() {
                                window.close();
                            }, 3000);
                        </script>
                    </body>
                    </html>
                )";
        res.set_header("Content-Type", "text/html; charset=UTF-8");
        res.set_content(html, "text/html");
        srv.stop();  // останавливаем сервер
    });
    // Запускаем сервер в отдельном потоке
    std::thread server_thread([&]() { srv.listen("localhost", 30101); });

    // Открываем браузер для авторизации
    std::string auth_url = "https://id.twitch.tv/oauth2/authorize?client_id=" + client_id + "&redirect_uri=" + redirect_uri +
                           "&response_type=code&scope=" + scope;
#if defined(_WIN32)
    ShellExecuteA(NULL, "open", auth_url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif

    // Ждем, пока сервер не получит код
    server_thread.join();
    // Сервер остановлен, проверяем, получили ли код
    if (code.empty()) {
        std::cerr << "Authorization code not received!" << std::endl;
        return;
    }

    // Обмен code на токен доступа
    cpr::Response tokenResponse = cpr::Post(cpr::Url{"https://id.twitch.tv/oauth2/token"}, cpr::Parameters{{"client_id", client_id},
                                                                                                           {"client_secret", client_secret},
                                                                                                           {"code", code},
                                                                                                           {"grant_type", "authorization_code"},
                                                                                                           {"redirect_uri", redirect_uri}});
    if (tokenResponse.status_code != 200) {
        return;
    } else {
        auto jsonResponse = json::parse(tokenResponse.text);
        AccessToken = jsonResponse["access_token"];
        RefreshToken = jsonResponse["refresh_token"];

        // Запишем токен в файл
        std::ofstream outFile("twitch_config");  // Файл для хранения токена
        if (outFile.is_open()) {
            outFile << RefreshToken;
            outFile.close();
        }
    }
}

void TwitchClient::updateAccessToken() {
    // Обновление токена доступа (Access Token) у Twitch
    if (RefreshToken.empty()) {
        std::cerr << "No refresh token available!" << std::endl;
        getAccessToken();
        return;
    }
    cpr::Response tokenResponse =
        cpr::Post(cpr::Url{"https://id.twitch.tv/oauth2/token"},
                  cpr::Parameters{
                      {"client_id", client_id}, {"client_secret", client_secret}, {"grant_type", "refresh_token"}, {"refresh_token", RefreshToken}});
    if (tokenResponse.status_code != 200) {
        return;
    } else {
        auto jsonResponse = json::parse(tokenResponse.text);
        AccessToken = jsonResponse["access_token"];
        RefreshToken = jsonResponse["refresh_token"];

        // Запишем токен в файл
        std::ofstream outFile("twitch_config");  // Файл для хранения токена
        if (outFile.is_open()) {
            outFile << RefreshToken;
            outFile.close();
        }
    }
}

json TwitchClient::getBroadcastInfo() {
    // Получение информации о пользователе
    cpr::Response userResponse = cpr::Get(cpr::Url{"https://api.twitch.tv/helix/users"}, cpr::Parameters{},
                                          cpr::Header{{"Authorization", "Bearer " + AccessToken}, {"Client-ID", client_id}});

    if (userResponse.status_code == 401) {
        updateAccessToken();
        this->getBroadcastInfo();
    } else if (userResponse.status_code != 200) {
        return NULL;
    } else {
        json jsonResponse = json::parse(userResponse.text);
        if (jsonResponse["data"].empty()) {
            return NULL;
        }
        broadcast_id = jsonResponse["data"][0]["id"];
        return jsonResponse["data"][0];
    }
    return NULL;
}

json TwitchClient::getCustomRewards() {
    // Получение кастомных наград
    if (broadcast_id.empty()) {
        getBroadcastInfo();
    }
    cpr::Response rewardsResponse = cpr::Get(cpr::Url{"https://api.twitch.tv/helix/channel_points/custom_rewards"},
                                            cpr::Header{{"Authorization", "Bearer " + AccessToken}, {"Client-ID", client_id}},
                                            cpr::Parameters{{"broadcaster_id", broadcast_id}});
    if (rewardsResponse.status_code == 401) {
        updateAccessToken();
        this->getCustomRewards();
    } else if (rewardsResponse.status_code != 200) {
        std::cerr << "Error fetching custom rewards: " << rewardsResponse.status_code << " - " << rewardsResponse.text << std::endl;
        return NULL;
    } else {
        json jsonResponse = json::parse(rewardsResponse.text);
        return jsonResponse["data"];
    }
    return NULL;
}

void TwitchClient::updateCustomReward(const std::string& reward_id, const std::string& title, const std::string& prompt, int cost, bool is_enabled) {
    // Обновление кастомной награды
    if (broadcast_id.empty()) {
        getBroadcastInfo();
    }
    json body = {
        {"title", title},
        {"prompt", prompt},
        {"cost", cost},
        {"is_enabled", is_enabled},
    };

    cpr::Response updateResponse = cpr::Patch(cpr::Url{"https://api.twitch.tv/helix/channel_points/custom_rewards"},
                                              cpr::Header{{"Authorization", "Bearer " + AccessToken}, {"Client-ID", client_id}, {"Content-Type", "application/json"}},
                                              cpr::Body{body.dump()},
                                              cpr::Parameters{{"broadcaster_id", broadcast_id}, {"id", reward_id}});

    if (updateResponse.status_code == 401) {
        updateAccessToken();
        this->updateCustomReward(reward_id, title, prompt, cost, is_enabled);
    } else if (updateResponse.status_code != 200) {
        std::cerr << "Error updating custom reward: " << updateResponse.status_code << " - " << updateResponse.text << std::endl;
    } else {
        // Успешно обновлено
    }
}
#pragma endregion
