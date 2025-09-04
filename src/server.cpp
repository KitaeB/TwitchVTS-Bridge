#include "server.h"
#include <basetsd.h>
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

Server::Server(int port, VTSClient& vtsClient, TwitchClient& twitchClient) : port(port), vtsClient_(vtsClient), twitchClient_(twitchClient) {
    CROW_ROUTE(app, "/")([]() {
        std::ifstream file("static/server.html");
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    });

    CROW_ROUTE(app, "/status")([]() { return crow::response(200, "OK"); });

    serverAPI();         // Обновляем список endpoint
    openModelRewards();  // если отсутствует файл json modelRewards мы его создадим
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

    // Запрос структуры json modelRewards
    CROW_ROUTE(app, "/model/list/reward").methods(crow::HTTPMethod::GET)([this](const crow::request& req) {
        auto modelName = req.url_params.get("modelName");
        // Читаем список наград по моделям
        json fileContent;
        std::ifstream file("ModelRewards.json");
        if (file.is_open()) {
            file >> fileContent;
            file.close();
        } else {
            fileContent = openModelRewards();
        }
        if (!modelName) {  // Возвращаем абсолютно все награды
            return crow::response(200, fileContent.dump(4));
        } else {
            // Ищем модель с таким же id и возвращаем её
            auto it =
                std::find_if(fileContent.begin(), fileContent.end(), [&](const json& entry) { return entry["model"]["modelName"] == modelName; });
            if (it != fileContent.end()) {
                return crow::response(200, it->dump(4));
            } else {
                return crow::response(404, "Model not found");
            }
        }
    });

    // Обновить список моделей ( после добавления новой модели в VtubeStudio )
    CROW_ROUTE(app, "/model/list/update").methods(crow::HTTPMethod::GET)([this]() {
        // Запросим список моделей с VTube Studio
        json modelList = vtsClient_.AvailableModelsRequest();
        if (modelList.empty()) return crow::response(500, "VtubeStudio return empty list");

        // Прочитаем файл с modelReward
        json fileContent;
        // Откроем файл с json modelRewards
        std::ifstream file("ModelRewards.json");
        if (file.is_open()) {
            // Прочитаем файл и закроем его
            file >> fileContent;
            file.close();
        } else {
            fileContent = openModelRewards();
            // Создадим список имеющихся моделей
            std::unordered_set<std::string> modelNames;
            for (json model : fileContent) {
                modelNames.insert(model["modelName"].get<std::string>());
            }

            // Пробежимся по списку моделей прочитанных по api
            for (json model : modelList) {
                if (modelNames.find(model["modelName"]) == modelNames.end()) {
                    // Если модель не нашлась, добавим её
                    fileContent = updateModelRewards(model["modelName"], fileContent[0]["Rewards"]);
                }
            }
        }
        return crow::response(crow::OK, fileContent, );
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
        // Выше залупный код, т.к. в функции я его обратно в json преобразую
        json fileContent = addNewReward(reward);

        return crow::response(200, fileContent);
    });

    // Запрос на обновление наград у модели
    CROW_ROUTE(app, "/reward/update").methods(crow::HTTPMethod::PUT)([this](const crow::request& req) {
        // json в req должен содержать modelName и Rewards
        json modelRewards = json::parse(req.body);
        if (!(modelRewards.contains("modelName") && modelRewards.contains("Rewards"))) {
            // Если не содержит, выводим ошибку
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
json Server::updateModelRewards(const std::string& modelName, const json& rewards) {
    json fileContent;
    // Читаем существующий набор моделей
    std::ifstream file("ModelRewards.json");

    if (file.is_open()) {
        file >> fileContent;
        file.close();
        // Ищем модель с таким же id
        auto it = std::find_if(fileContent.begin(), fileContent.end(), [&](const json& entry) { return entry["model"]["modelName"] == modelName; });

        // Если при поиске мы не дошли до конца списка, т.е. нашли модель, обеовляем её награды
        if (it != fileContent.end()) {
            (*it)["Rewards"] = rewards;
        }
        // Если при поиске мы дошли до конца списка, добавим новую модель
        else {
            fileContent.push_back({{"model", modelName}, {"Rewards", rewards}});
        }
        // Запишем обновления в файл
        std::ofstream outFile("ModelRewards.json");
        if (outFile.is_open()) {
            outFile << fileContent.dump(4);
            outFile.close();
        }

    } else {  // Если файла нет, создаём его
        fileContent = openModelRewards();
    }
    return fileContent;
}

// Добавление нового Reward в Json файл
json Server::addNewReward(const Reward& reward) {
    // Откроем и прочитаем файл
    json fileContent;
    std::ifstream file("ModelRewards.json");
    if (file.is_open()) {
        file >> fileContent;
        file.close();
    }
    // Сформируем json из Reward
    json reward_json;
    reward_json["id"] = reward.id;
    reward_json["title"] = reward.title;
    reward_json["prompt"] = reward.prompt;
    reward_json["cost"] = reward.cost;
    reward_json["is_enabled"] = reward.is_enabled;

    // Пройдёмся по всем моделям и добавим в каждую нашу модель
    for (json& model : fileContent) {
        if (model.contains("Rewards") && model["Rewards"].is_array()) {
            model["Rewards"].push_back(reward_json);
        }
    }

    // Запишем обновления в файл
    std::ofstream outFile("ModelRewards.json");
    if (outFile.is_open()) {
        outFile << fileContent.dump(4);
        outFile.close();
    }

    return fileContent;
}

void Server::twitchvts() {
    std::string readModelName = vtsClient_.CurrentModelRequest()["modelName"];
    std::cout << "Real Model : " << readModelName << ", Old model: " << currentModelName << std::endl;
    if (currentModelName.empty() || currentModelName != readModelName) {
        currentModelName = readModelName;

        // Читаем json Файл
        json ModelRewars = openModelRewards();

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
