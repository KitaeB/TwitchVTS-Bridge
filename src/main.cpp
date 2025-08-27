#include "api_client.h"

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    while (true) {
        try {
            VTSClient vtsClient;
            vtsClient.message_handler([](std::string msg){
                std::cout << "Received: " << msg << std::endl;
            });
            vtsClient.Subscribe();

            
            while (true)
                {                    
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    return 0;  // Return success
}
