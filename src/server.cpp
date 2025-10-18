#include "server.h"
#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/json.h>
#include <crow/logging.h>
#include <curl/curl.h>
#include <algorithm>
#include <fstream>
#include <ostream>
#include <string>
#include <unordered_set> // добавлено
#include "api_client.h"

/*
INFO:

    1 Main. Polling VtubeStudio для определения текущей модели -> [Обновлена модель] -> Чтение json файла с настройками -> для наград с флагом
    is_vts_reward сформировать updateReward()

    2 Create. POST на создание reward -> [status code 200] -> Считывание json -> пробежались по всем моделям,
    для каждой добавили награду, с is_enabled = false

    3 addModel. На сервере кнопка обновить -> Считываю список моделей -> Считываю json -> Добавляю
    отсутствующие модели -> Возвращаю json modelRewards

    4 updateRewardsList. POST на сохранения -> [Сохраняю json {Model. Rewards[]}] -> Считываю json ->
    ищу в json файле Model -> обновляю rewards

*/

/* TODO:
    1. Изменить запрос с ModelID на ModelName, поскольку ModelID изменчив   +++ (нужно проверить)
    2. Для алгоритма addModel, добавить API endpoint +++ (нужно проверить)
    3. Изменить запрос списка моделей, добавить фильтр, для определения только тех моделей, которыми я могу управлять +++ (нужно проверить)
    4. Добавить API endpoint POST, для обновления списка наград у модели +++ (надо проверить)
    5. Добавить функцию, для добавления в json новый reward для всех моделей +++ (Надо проверить)

*/

Server::Server(VTSClient& vtsClient, TwitchClient& twitchClient) : vtsClient_(vtsClient), twitchClient_(twitchClient) {
    CROW_ROUTE(app, "/")([]() {
        std::ifstream file("static/server.html");
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    });
    
    // Запрос на статус работы сервера
    CROW_ROUTE(app, "/status")([]() { return crow::response(200, "OK"); });

    // Обернём инициализацию в try/catch, чтобы ошибки сетевых запросов не крашили процесс
    try {
        serverAPI();         // Обновляем список endpoint
        openModelRewards();  // если отсутствует файл json modelRewards мы его создадим
    } catch (const std::exception& e) {
        CROW_LOG_ERROR << "Server init warning: " << e.what();
        // продолжаем с возможным пустым файлом ModelRewards.json
    }
}

Server::~Server() { app.stop(); }

void Server::run() { app.port(port).run(); }

void Server::stop() { app.stop(); }

void Server::serverAPI() {
    
    // Запрос списка моделей
    CROW_ROUTE(app, "/model/list").methods(crow::HTTPMethod::GET)([this]() {
        json models;
        try {
            models = vtsClient_.AvailableModelsRequest();
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "AvailableModelsRequest failed: " << e.what();
            models = json::array();
        }
        json x;
        x["models"] = json::array();
        for (const auto& model : models) {
            x["models"].push_back(model);
        }
        return crow::response(200, x.dump(4));
    });

    // Запрос структуры json modelRewards
    CROW_ROUTE(app, "/model/list/reward").methods(crow::HTTPMethod::GET)([this](const crow::request& req) {
        auto modelName = req.url_params.get("modelName");
        // Читаем список наград по моделям
        json fileContent;
        std::ifstream file("ModelRewards.json");
        if (file.is_open()) {
            try {
                file >> fileContent;
            } catch (...) {
                fileContent = json::array();
            }
            file.close();
        } else {
            fileContent = openModelRewards();
        }
        if (!modelName) {  // Возвращаем абсолютно все награды
            return crow::response(200, fileContent.dump(4));
        } else {
            // Ищем модель с таким же именем и возвращаем её
            auto it = std::find_if(fileContent.begin(), fileContent.end(), [&](const json& entry) {
                return entry.contains("modelName") && entry["modelName"] == modelName;
            });
            if (it != fileContent.end()) {
                return crow::response(200, it->dump(4));
            } else {
                return crow::response(404, "Model not found");
            }
        }
    });

    // Обновить список моделей ( после добавления новой модели в VtubeStudio )
    CROW_ROUTE(app, "/model/list/update").methods(crow::HTTPMethod::GET)([this]() {
        json modelList;
        try {
            modelList = vtsClient_.AvailableModelsRequest();
        } catch (const std::exception& e) {
            return crow::response(500, std::string("VtubeStudio request failed: ") + e.what());
        }
        if (modelList.empty()) return crow::response(500, "VtubeStudio return empty list");

        // Прочитаем/создадим файл с modelReward
        json fileContent;
        std::ifstream file("ModelRewards.json");
        if (file.is_open()) {
            try {
                file >> fileContent;
            } catch (...) {
                fileContent = json::array();
            }
            file.close();
        } else {
            fileContent = openModelRewards();
        }

        // Создадим множество существующих имён
        std::unordered_set<std::string> modelNames;
        for (const json& entry : fileContent) {
            if (entry.contains("modelName") && entry["modelName"].is_string()) {
                modelNames.insert(entry["modelName"].get<std::string>());
            }
        }

        // Добавим отсутствующие модели с пустым списком Rewards
        for (const json& model : modelList) {
            if (!model.contains("modelName")) continue;
            std::string name = model["modelName"].get<std::string>();
            if (modelNames.find(name) == modelNames.end()) {
                fileContent.push_back({{"modelName", name}, {"Rewards", json::array()}});
                modelNames.insert(name);
            }
        }

        // Сохраним файл
        try {
            std::ofstream outFile("ModelRewards.json");
            outFile << fileContent.dump(4);
            outFile.close();
        } catch (...) {
            CROW_LOG_ERROR << "Failed to write ModelRewards.json";
        }

        return crow::response(200, fileContent.dump(4));
    });

    // Запрос на создание новой награды за баллы
    CROW_ROUTE(app, "/reward/create").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        json param = json::parse(req.body);
        json newReward;
        if (param.contains("title")) {
            newReward = twitchClient_.createReward(param);
        } else {
            return crow::response(405, "Request body not include title");
        }
        // Добавим новую reward в файл
        Reward reward;
        reward.id = newReward["id"];
        reward.title = newReward["title"];
        reward.prompt = newReward["prompt"];
        reward.cost = newReward["cost"];
        reward.is_enabled = newReward["is_enabled"];
        json fileContent = addNewReward(reward);

        return crow::response(200, fileContent.dump(4));
    });

    // Запрос на обновление наград у модели
    CROW_ROUTE(app, "/reward/update").methods(crow::HTTPMethod::PUT)([this](const crow::request& req) {
        json modelRewards = json::parse(req.body);
        if (!(modelRewards.contains("modelName") && modelRewards.contains("Rewards"))) {
            return crow::response(405, "Json request must contain modelName and Rewards{id, title, prompt, cost, is_enabled} ");
        } else {
            updateModelRewards(modelRewards["modelName"], modelRewards["Rewards"]);
            return crow::response(200, "OK");
        }
    });
}

// Создание JSON файла, если его нет
json Server::openModelRewards() {
    json fileContent = json::array();
    std::ifstream file("ModelRewards.json");
    if (!file.is_open()) {
        // Заполнение файла начальными данными (без аварий при ошибках)
        json Models;
        json Rewards;
        try {
            Models = vtsClient_.AvailableModelsRequest();
        } catch (...) {
            Models = json::array();
        }
        try {
            Rewards = twitchClient_.getCustomRewards();
        } catch (...) {
            Rewards = json::array();
        }

        json allRewards = json::array();
        for (const json& reward : Rewards) {
            json modelRewards;
            if (reward.contains("id")) modelRewards["id"] = reward["id"];
            if (reward.contains("title")) modelRewards["title"] = reward["title"];
            if (reward.contains("prompt")) modelRewards["prompt"] = reward["prompt"];
            if (reward.contains("cost")) modelRewards["cost"] = reward["cost"];
            if (reward.contains("is_enabled")) modelRewards["is_enabled"] = reward["is_enabled"];
            allRewards.push_back(modelRewards);
        }
        for (const json& model : Models) {
            if (model.contains("modelName"))
                fileContent.push_back({{"modelName", model["modelName"]}, {"Rewards", allRewards}});
        }
        try {
            std::ofstream outFile("ModelRewards.json");
            outFile << fileContent.dump(4);
            outFile.close();
        } catch (...) {
            CROW_LOG_ERROR << "Failed to create ModelRewards.json";
        }
    } else {
        try {
            file >> fileContent;
        } catch (...) {
            fileContent = json::array();
        }
        file.close();
    }
    return fileContent;
}

// Обновление наград модели
json Server::updateModelRewards(const std::string& modelName, const json& rewards) {
    json fileContent;
    std::ifstream file("ModelRewards.json");

    if (file.is_open()) {
        try {
            file >> fileContent;
        } catch (...) {
            fileContent = json::array();
        }
        file.close();

        auto it = std::find_if(fileContent.begin(), fileContent.end(), [&](const json& entry) {
            return entry.contains("modelName") && entry["modelName"] == modelName;
        });

        if (it != fileContent.end()) {
            (*it)["Rewards"] = rewards;
        } else {
            fileContent.push_back({{"modelName", modelName}, {"Rewards", rewards}});
        }

        try {
            std::ofstream outFile("ModelRewards.json");
            outFile << fileContent.dump(4);
            outFile.close();
        } catch (...) {
            CROW_LOG_ERROR << "Failed to write ModelRewards.json";
        }

    } else {
        fileContent = openModelRewards();
    }
    return fileContent;
}

// Добавление нового Reward в Json файл
json Server::addNewReward(const Reward& reward) {
    json fileContent;
    std::ifstream file("ModelRewards.json");
    if (file.is_open()) {
        try {
            file >> fileContent;
        } catch (...) {
            fileContent = json::array();
        }
        file.close();
    } else {
        fileContent = openModelRewards();
    }

    json reward_json;
    reward_json["id"] = reward.id;
    reward_json["title"] = reward.title;
    reward_json["prompt"] = reward.prompt;
    reward_json["cost"] = reward.cost;
    reward_json["is_enabled"] = reward.is_enabled;

    for (json& model : fileContent) {
        if (model.contains("Rewards") && model["Rewards"].is_array()) {
            model["Rewards"].push_back(reward_json);
        } else if (!model.contains("Rewards")) {
            model["Rewards"] = json::array({reward_json});
        }
    }

    try {
        std::ofstream outFile("ModelRewards.json");
        outFile << fileContent.dump(4);
        outFile.close();
    } catch (...) {
        CROW_LOG_ERROR << "Failed to write ModelRewards.json";
    }

    return fileContent;
}

void Server::twitchvts() {
    json curModelJson;
    try {
        curModelJson = vtsClient_.CurrentModelRequest();
    } catch (const std::exception& e) {
        CROW_LOG_ERROR << "CurrentModelRequest failed: " << e.what();
        return;
    }
    std::string readModelName;
    if (curModelJson.contains("modelName") && curModelJson["modelName"].is_string())
        readModelName = curModelJson["modelName"].get<std::string>();

    std::cout << "Real Model : " << readModelName << ", Old model: " << currentModelName << std::endl;
    if (readModelName.empty()) return;

    if (currentModelName.empty() || currentModelName != readModelName) {
        currentModelName = readModelName;

        json ModelRewards = openModelRewards();

        auto it = std::find_if(ModelRewards.begin(), ModelRewards.end(), [&](const json& entry) {
            return entry.contains("modelName") && entry["modelName"] == currentModelName;
        });

        if (it != ModelRewards.end()) {
            for (const json& reward : (*it)["Rewards"]) {
                try {
                    twitchClient_.updateCustomReward(reward["id"], reward["title"], reward["prompt"], reward["cost"], reward["is_enabled"]);
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "updateCustomReward failed: " << e.what();
                }
            }
        }
    }
}
