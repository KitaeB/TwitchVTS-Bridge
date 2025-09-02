#pragma once

#include <crow.h>
#include <string>
#include "api_client.h"

struct Reward {
    std::string id;
    std::string title;
    std::string prompt;
    int cost;
    bool is_enabled;
    bool is_vts_reward{false};

};
struct Model {
    std::string modelName;
    std::string modelID;
};
struct ModelRewards {
    Model model;
    std::vector<Reward> rewards;
};

class Server {
public:
    Server(int port, VTSClient& vtsClient, TwitchClient& twitchClient);
    ~Server();

    json createJsonFile();
    json updateModelRewards(const json& modelID, const json& rewards);
    json addNewReward(const Reward& reward);
    json addNewModel(const std::string& modelID);

    void twitchvts();

    void run();
    void stop();
private:
    void serverAPI();
    crow::SimpleApp app;
    int port = 801;

    // Объекты стороних клиентов
    VTSClient& vtsClient_;
    TwitchClient& twitchClient_;
    std::string currentModelName; 
};