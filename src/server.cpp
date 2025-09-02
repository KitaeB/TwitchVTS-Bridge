#include "server.h"
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/json.h>
#include <crow/logging.h>
#include <curl/curl.h>
#include <algorithm>
#include <fstream>
#include <ostream>
#include "api_client.h"

/*
Алгоритмы:

    1 Main. Polling VtubeStudio для определения текущей модели -> [Обновлена модель] -> Чтение json файла с настройками -> для наград с флагом is_vts_reward сформировать updateReward()
    2 Create. POST на создание reward -> [status code 200] -> Считывание json -> пробежались по всем моделям, для каждой добавили награду, с is_enabled = false
    3 addModel. На сервере кнопка обновить -> Считываю список моделей -> Считываю json -> Добавляю отсутствующие модели
    4 updateRewardsList. POST на сохранения -> [Сохраняю json {Model. Rewards[]}] -> Считываю json -> ищу в json файле Model -> обновляю rewards

*/

/* TODO

    1. Изменить запрос с ModelID на ModelName, поскольку ModelID изменчив   +++ (нужно проверить)
    2. Для алгоритма addModel, добавить API endpoint
    3. Изменить запрос списка моделей, добавить фильтр, для определения только тех моделей, которыми я могу управлять ++ (нужно проверить)
    4. Добавить API endpoint POST, для обновления списка наград у модели
    
*/

Server::Server(int port, VTSClient& vtsClient, TwitchClient& twitchClient) : port(port), vtsClient_(vtsClient), twitchClient_(twitchClient) {
    CROW_ROUTE(app, "/")([]() {
        std::ifstream file("static/server.html");
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    });

    CROW_ROUTE(app, "/status")([]() { return crow::response(200, "OK"); });

    serverAPI();
    createJsonFile();
}

Server::~Server() { app.stop(); }

void Server::run() { app.port(port).concurrency(2).run(); }

void Server::stop() { app.stop(); }

void Server::serverAPI() {
    // Запрос списка моделей
    CROW_ROUTE(app, "/model/list").methods(crow::HTTPMethod::GET)([this]() {
        json models = vtsClient_.AvailableModelsRequest();
        json x;
        x["models"] = json::array();
        for (const auto& model : models) {
            x["models"].push_back(model);
        }
        return crow::response(200, x.dump(4));
    });

    // Запрос на создание новой награды за баллы
    CROW_ROUTE(app, "/reward/create").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        json param = json::parse(req.body);

        if (param.contains("title")) {
            twitchClient_.createReward(param);
        } else {
            return crow::response(405, "Request body not include title");
        }
        return crow::response(200,"ok");
    });

    // Запрос структуры json
    CROW_ROUTE(app, "/model/list/reward").methods(crow::HTTPMethod::GET)([this](const crow::request& req) {
        auto modelName = req.url_params.get("modelName");
        // Читаем список наград по моделям
        json fileContent;
        std::ifstream file("ModelRewards.json");
        if (file.is_open()) {
            file >> fileContent;
            file.close();
        } else {
            fileContent = createJsonFile();
        }
        if (!modelName) {  // Возвращаем абсолютно все награды
            return crow::response(200, fileContent.dump(4));
        } else {
            // Ищем модель с таким же id и возвращаем её
            auto it = std::find_if(fileContent.begin(), fileContent.end(), [&](const json& entry) { return entry["model"]["modelName"] == modelName; });
            if (it != fileContent.end()) {
                return crow::response(200, it->dump(4));
            } else {
                return crow::response(404, "Model not found");
            }
        }
    });
}

// Создание JSON файла, если его нет
json Server::createJsonFile() {
    json fileContent = json::array();
    std::ifstream file("ModelRewards.json");
    if (!file.is_open()) {
        // Заполнение файла начальными данными
        json Models = vtsClient_.AvailableModelsRequest();
        json Rewards = twitchClient_.getCustomRewards();

        json allRewards;
        for (const json reward : Rewards) {
            json modelRewards;
            modelRewards["id"] = reward["id"];
            modelRewards["title"] = reward["title"];
            modelRewards["prompt"] = reward["prompt"];
            modelRewards["cost"] = reward["cost"];
            modelRewards["is_enabled"] = reward["is_enabled"];

            allRewards.push_back(modelRewards);
        }
        for (const json model : Models) {
            fileContent.push_back({{"Model", model["modelName"]}, {"Rewards", allRewards}});
        }
        std::ofstream outFile("ModelRewards.json");
        outFile << fileContent.dump(4);  // Красивый отступ в 4 пробела
    } else {
        file >> fileContent;
        file.close();
    }
    return fileContent;
}

// Обновление наград модели
json Server::updateModelRewards(const json& model, const json& rewards) {
    json fileContent;
    // Читаем существующий набор моделей
    std::ifstream file("ModelRewards.json");

    if (file.is_open()) {
        file >> fileContent;
        file.close();
        // Ищем модель с таким же id
        auto it =
            std::find_if(fileContent.begin(), fileContent.end(), [&](const json& entry) { return entry["model"]["modelName"] == model["modelName"]; });
        // Если при поиске мы не дошли до конца списка, т.е. нашли модель, обеовляем её награды
        if (it != fileContent.end()) {
            (*it)["Rewards"] = rewards;
        }
        // Если при поиске мы дошли до конца списка, добавим новую модель
        else {
            fileContent.push_back({{"model", model}, {"Rewards", rewards}});
        }
        // запишем обновления в файл
        std::ofstream outFile("ModelRewards.json");
        if (outFile.is_open()) {
            outFile << fileContent.dump(4);
            outFile.close();
        }
    } else {  // Если файла нет, создаём его
        createJsonFile();
    }
    return fileContent;
}

void Server::twitchvts() {
    std::string readModelName = vtsClient_.CurrentModelRequest()["modelName"];
    std::cout << "Real Model : " << readModelName << ", Old model: " << currentModelName<< std::endl;
    if (currentModelName.empty() || currentModelName != readModelName) {
        currentModelName = readModelName;

        // Читаем json Файл
        json ModelRewars = createJsonFile();

        // Ищем модель с таким же id
        auto it =
            std::find_if(ModelRewars.begin(), ModelRewars.end(), [&](const json& entry) { return entry["model"]["modelName"] == currentModelName; });
        // Если мы нашли модели в файле
        if (it != ModelRewars.end()) {
            for (json reward : (*it)["Rewards"]) {
                twitchClient_.updateCustomReward(reward["id"], reward["title"], reward["prompt"], reward["cost"], reward["is_enabled"]);
            }
        }
    }
}
