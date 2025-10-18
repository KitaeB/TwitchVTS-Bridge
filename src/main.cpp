#include "api_client.h"
#include "server.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

namespace {
std::atomic<bool> running{true};
volatile std::sig_atomic_t stopSignalReceived = 0;

void signalHandler(int /*signal*/) {
    stopSignalReceived = 1;
}
}

void vtsThread(VTSClient& vtsClient, std::atomic<bool>& running) {
    while (running.load()) {
        if (stopSignalReceived) break;
        try {
            if (!vtsClient.isConnected()) {
                vtsClient.connect();
            }
        std::this_thread::sleep_for(std::chrono::seconds(10));
        } catch (const std::exception& e) {
            std::cerr << "VTSClient Exception: " << e.what() << std::endl;
        }
    }
}

void daThread(DonationAlertsClient& daClient, std::atomic<bool>& running) {
    while (running.load()) {
        if (stopSignalReceived) break;
        try {
            json donationMessage = daClient.getNewDonationMessage();
            if (!donationMessage.empty()) {
                std::cout << donationMessage.dump(4) << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(10));
        } catch (const std::exception& e) {
            std::cerr << "DonationAlertsClient Exception: " << e.what() << std::endl;
        }
    }
}

void srvThread(Server& server, std::atomic<bool>& running) {
    while (running.load()) {
        try {
        if (stopSignalReceived) {
            std::cout << "\nReceived shutdown signal in srvThread. Stopping...\n";
            running.store(false);
            break;
        }
            if (cpr::Get(cpr::Url{"http://localhost:8010/status"}).status_code == 200) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            } server.run();
        } catch (const std::exception& e) {
            std::cerr << "srvThread exception: " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int main() {
    
    setlocale(LC_ALL, "Russian");
    try {
        /* Установка обработчика сигналов */
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        /* Инициализация клиентов */
        VTSClient vtsClient;
        TwitchClient twitchClient;
        DonationAlertsClient daClient;
        Server server(vtsClient, twitchClient);
        
        /* Запуск потоков */
        std::thread vThread(vtsThread, std::ref(vtsClient), std::ref(running));
        std::thread sThread(srvThread, std::ref(server), std::ref(running));
        std::thread dThread(daThread, std::ref(daClient), std::ref(running));

        if (sThread.joinable()) sThread.join();
        if (vThread.joinable()) vThread.join();
        if (dThread.joinable()) dThread.join();

    } catch (const std::exception& e) {
        std::cerr << "Main exception: " << e.what() << std::endl;
    }
    return 0;
}
