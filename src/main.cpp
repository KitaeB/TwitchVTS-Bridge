#include "api_client.h"

#include <iostream>

int main(int argc, char** argv) {
    int i;
    while (true) {
        try {
            VTSClient vtsClient;

            while (true) {
                switch (std::cin >> i, i) {
                    case 1: {
                        json state = vtsClient.ApiStateRequest();
                        std::cout << "API State: " << state.dump(4) << std::endl;
                        break;
                    }
                    case 2: {
                        json models = vtsClient.AvailableModelsRequest();
                        std::cout << "Available Models: " << models.dump(4) << std::endl;
                        break;
                    }
                    case 3: {
                        json currentModel = vtsClient.CurrentModelRequest();
                        std::cout << "Current Model: " << currentModel.dump(4) << std::endl;
                        break;
                    }
                    case 4: {
                        vtsClient.reconnect();
                        std::cout << "Reconnected to VTS." << std::endl;
                        break;
                    }
                    case 5: {
                        std::cout << "Subscribe State: " << vtsClient.Subscribe();
                        break;
                    }
                    case 6: {
                        std::cout << "unSubscribe State: " << vtsClient.unSubscribe();
                        break;
                    }
                    case 0: {
                        std::cout << "Exiting..." << std::endl;
                        return 0;
                    }
                    default:
                        std::cout << "Invalid choice. Please select a valid option." << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    return 0;  // Return success
}
