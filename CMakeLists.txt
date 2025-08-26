cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project(TwitchVTS)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system asio)

add_executable(TwitchVTS src/main.cpp src/server.cpp src/api_client.cpp)

# Только для Windows — линковка с winsock
if(WIN32)
    target_link_libraries(TwitchVTS PRIVATE ws2_32)
endif()

# Линковка библиотек
target_link_libraries(TwitchVTS
    PRIVATE
    Boost::system
)