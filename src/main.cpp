
#include "api_client.h"
#include "server.h"

#include <chrono>
#include <iostream>
#include <thread>

/* Надо написать обработчик, что будет по изменению модели в VTS обновлять награды twitch
    Также надо написать обработчик, что будет обновлять список наград, при их изменении на стороне твитча
    Также надо написать обработчик, что будет обновлять список моделей, при их обновлении на стороне VTS
    Также надо написать обработчик, что будет api, что будет обноввлять json файл с настрйоками
*/

int main(int argc, char** argv) {
    int i;
    cpr::Response r;
    while (true) {
        try {
            // Initialize the clients
            VTSClient vtsClient;
            TwitchClient twitchClient;

            // Start the server
            Server server(801, vtsClient, twitchClient);
            server.run();

            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    return 0;  // Return success
}
