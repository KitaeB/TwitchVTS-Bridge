#include "api_client.h"

#include <chrono>
#include <iostream>
#include <thread>
int main(int argc, char** argv) {
    int i;
    while (true) {
        try {
            VTSClient vtsClient;

            TwitchClient twitchClient;
            
            while (true) {
                std::cout << vtsClient.CurrentModelRequest().dump(4) << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    return 0;  // Return success
}
