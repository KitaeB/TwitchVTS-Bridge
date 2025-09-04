# TwitchVTS-Bridge

## Описание
Этот проект представляет собой мост между **VTube Studio** и **Twitch**, позволяющий синхронизировать кастомные награды (Channel Points Rewards) с моделями в VTube Studio.

---

## 📌 Файл: server.cpp

### Класс `Server`
- **Server::Server(...)** – конструктор, запускает веб-сервер на основе Crow, регистрирует роуты и создаёт JSON-файл с моделью и наградами.  
- **Server::~Server()** – деструктор, останавливает сервер.  
- **Server::run()** – запускает сервер на указанном порту.  
- **Server::stop()** – останавливает сервер.  
- **Server::serverAPI()** – регистрирует API-эндпоинты:  
  - `/model/list` – список доступных моделей из VTubeStudio.  
  - `/reward/create` – создание новой Twitch-награды.  
  - `/model/list/reward` – возвращает награды для всех моделей или конкретной.  
  - `/model/update` – обновляет список моделей и их наград.  
- **Server::createJsonFile()** – создаёт (или загружает) файл `ModelRewards.json`. В него кладутся все модели с их наградами.  
- **Server::updateModelRewards(model, rewards)** – обновляет награды у конкретной модели в `ModelRewards.json`. Если модель не найдена, добавляет новую.  
- **Server::twitchvts()** – следит за сменой модели в VTubeStudio и обновляет награды на Twitch в соответствии с сохранёнными настройками.  

---

## 📌 Файл: api_client.cpp

### 🔹 Класс `VTSClient` (работа с VTubeStudio API через WebSocket)
- **VTSClient::VTSClient()** – конструктор, подключается к VTS, обрабатывает аутентификацию и токены.  
- **VTSClient::~VTSClient()** – деструктор, закрывает соединение.  
- **VTSClient::setPort / setHost** – настройка подключения.  
- **VTSClient::ApiStateRequest()** – запрос состояния API VTubeStudio.  
- **VTSClient::AuthenticationTokenRequest()** – получение нового токена аутентификации.  
- **VTSClient::AuthenticateRequest(token)** – проверка валидности токена.  
- **VTSClient::AvailableModelsRequest()** – запрос доступных моделей.  
- **VTSClient::CurrentModelRequest()** – запрос информации о текущей модели.  

### 🔹 Класс `TwitchClient` (работа с Twitch API)
- **TwitchClient::TwitchClient()** – конструктор, загружает refresh-токен из файла или получает новый.  
- **TwitchClient::~TwitchClient()** – деструктор (пустой).  
- **TwitchClient::getAccessToken()** – OAuth-авторизация через браузер, получение Access/Refresh токена.  
- **TwitchClient::updateAccessToken()** – обновление Access токена с помощью Refresh токена.  
- **TwitchClient::getBroadcastInfo()** – получение информации о канале (ID стримера и др.).  
- **TwitchClient::getCustomRewards()** – список пользовательских наград Twitch.  
- **TwitchClient::updateCustomReward(...)** – обновление параметров награды (стоимость, включение/выключение и т.п.).  
- **TwitchClient::createReward(json param)** – создание новой награды на Twitch.  

---

## 🚀 Итог
- `server.cpp` — отвечает за веб-сервер и взаимодействие между JSON-настройками, Twitch и VTube Studio.  
- `api_client.cpp` — реализует клиентов для работы с **VTube Studio API** и **Twitch API**.  

