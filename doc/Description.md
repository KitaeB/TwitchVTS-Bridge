# Документация для TwitchVTS-Bridge

Этот документ содержит подробное описание функций классов `Server`, `VTSClient` и `TwitchClient`, а также API-эндпоинтов сервера. Проект интегрирует VTube Studio и Twitch для управления моделями и пользовательскими наградами за очки канала.

## Функции классов

### Класс Server

Класс `Server` управляет HTTP-сервером с использованием фреймворка Crow, интегрируясь с VTube Studio (`VTSClient`) и Twitch (`TwitchClient`) для обработки данных моделей и наград.

- **Конструктор: `Server(int port, VTSClient& vtsClient, TwitchClient& twitchClient)`**
  - **Описание**: Инициализирует сервер на указанном порту, устанавливает ссылки на экземпляры `VTSClient` и `TwitchClient`, а также настраивает базовые маршруты (`/` и /status`) и API-эндпоинты сервера.
  - **Параметры**:
    - `port`: Порт, на котором будет работать сервер.
    - `vtsClient`: Ссылка на экземпляр `VTSClient` для взаимодействия с VTube Studio.
    - `twitchClient`: Ссылка на экземпляр `TwitchClient` для взаимодействия с API Twitch.
  - **Поведение**:
    - Настраивает маршрут для `/`, возвращающий статический HTML-файл (`static/server.html`).
    - Настраивает эндпоинт `/status`, возвращающий ответ 200 OK.
    - Вызывает `serverAPI()` для настройки дополнительных API-эндпоинтов.
    - Вызывает `openModelRewards()` для создания файла `ModelRewards.json`, если он отсутствует.

- **Деструктор: `~Server()`**
  - **Описание**: Останавливает сервер Crow.
  - **Поведение**: Вызывает `app.stop()` для корректного завершения работы сервера.

- **Функция: `void run()`**
  - **Описание**: Запускает сервер на указанном порту с двумя потоками обработки.
  - **Поведение**: Вызывает `app.port(port).concurrency(2).run()` для запуска сервера.

- **Функция: `void stop()`**
  - **Описание**: Останавливает сервер.
  - **Поведение**: Вызывает `app.stop()` для завершения работы сервера.

- **Функция: `void serverAPI()`**
  - **Описание**: Настраивает API-эндпоинты сервера.
  - **Поведение**:
    - Определяет маршруты:
      - `/model/list` (GET): Возвращает список доступных моделей из VTube Studio, структура json из [VTubeStudio](https://github.com/DenchiSoft/VTubeStudio) .
      - `/model/list/reward` (GET): Возвращает содержимое `ModelRewards.json` или награды для указанной модели, если передан параметр `modelName`, в формате json.
      - `/model/list/update` (GET): Запрос на обновление `ModelRewards.json`, добавляя новые модели из VTube Studio, возвращает содержимое `ModelRewards.json`.
      - `/reward/create` (POST): Создаёт новую награду за баллы канала Twitch и добавляет её для всех моделей в `ModelRewards.json`, запрос обязательно должен содержать `title`, второстепенно может содержать параметры из [Twitch API: Create Custom Rewards](https://dev.twitch.tv/docs/api/reference#create-custom-rewards), возвращает содержимое `ModelRewards.json`.
      - `/reward/update` (PUT): Обновляет награды для указанной модели в `ModelRewards.json`, тело запроса должно быть в формате:
      ```json
      {
        "modelName": "",
        "Rewards": [
          {
            "id": "",
            "title": "",
            "prompt": "",
            "cost": 150,
            "is_enabled": false,
          }
        ]
      }
- **Функция: `json openModelRewards()`**
  - **Описание**: Открывает или создаёт файл `ModelRewards.json`, инициализируя его моделями и наградами, если он отсутствует.
  - **Возвращает**: JSON-массив, содержащий данные моделей и наград.
  - **Поведение**:
    - Если файл `ModelRewards.json` существует, читает и возвращает его содержимое.
    - Если файла нет, получает модели из `VTSClient` и награды из `TwitchClient`, создаёт JSON-структуру и записывает её в файл.

- **Функция: `json updateModelRewards(const std::string& modelName, const json& rewards)`**
  - **Описание**: Обновляет награды модели или добавляет новую модель с указанными наградами в `ModelRewards.json`.
  - **Параметры**:
    - `modelName`: Имя модели для обновления.
    - `rewards`: JSON-массив наград, которые будут связаны с моделью.
  - **Возвращает**: Обновлённое содержимое JSON-файла `ModelRewards.json`.
  - **Поведение**:
    - Читает существующий файл `ModelRewards.json`.
    - Если модель найдена, обновляет её награды; если нет — добавляет новую запись.
    - Записывает обновлённый JSON обратно в файл.

- **Функция: `json addNewReward(const Reward& reward)`**
  - **Описание**: Добавляет новую награду ко всем моделям в `ModelRewards.json`.
  - **Параметры**:
    - `reward`: Структура `Reward`, содержащая `id`, `title`, `prompt`, `cost` и `is_enabled`.
  - **Возвращает**: Обновлённое содержимое JSON-файла `ModelRewards.json`.
  - **Поведение**:
    - Читает `ModelRewards.json`.
    - Добавляет новую награду в массив `Rewards` каждой модели.
    - Записывает обновлённый JSON обратно в файл.

- **Функция: `void twitchvts()`**
  - **Описание**: Опрашивает VTube Studio для получения текущей модели и обновляет награды Twitch, если модель изменилась.
  - **Поведение**:
    - Получает имя текущей модели из `VTSClient`.
    - Если модель изменилась (или `currentModelName` пусто), обновляет `currentModelName` и читает `ModelRewards.json`.
    - Находит награды для текущей модели и обновляет их через `TwitchClient`.

### Класс VTSClient

Класс `VTSClient` управляет взаимодействием с VTube Studio через WebSocket.

- **Конструктор: `VTSClient()`**
  - **Описание**: Инициализирует WebSocket-соединение с VTube Studio и выполняет аутентификацию.
  - **Поведение**:
    - Подключается к серверу VTube Studio с использованием Boost.Beast WebSocket.
    - Проверяет наличие токена аутентификации в файле `config`.
    - Если токена нет или аутентификация не удалась, запрашивает новый токен и аутентифицируется.
    - Сохраняет токен в файл `config`.

- **Деструктор: `~VTSClient()`**
  - **Описание**: Закрывает WebSocket-соединение.
  - **Поведение**: Закрывает WebSocket с нормальным кодом завершения, регистрируя любые ошибки.

- **Функция: `void setPort(int port)`**
  - **Описание**: Устанавливает порт для WebSocket-соединения с VTube Studio.
  - **Параметры**:
    - `port`: Номер порта.

- **Функция: `void setHost(const std::string& host)`**
  - **Описание**: Устанавливает хост для WebSocket-соединения с VTube Studio.
  - **Параметры**:
    - `host`: Адрес хоста.

- **Функция: `json ApiStateRequest()`**
  - **Описание**: Запрашивает текущее состояние API VTube Studio.
  - **Возвращает**: JSON-объект, содержащий состояние API.
  - **Поведение**: Отправляет сообщение `APIStateRequest` и возвращает поле `data` из ответа.

- **Функция: `std::string AuthenticationTokenRequest()`**
  - **Описание**: Запрашивает токен аутентификации от VTube Studio.
  - **Возвращает**: Токен аутентификации в виде строки.
  - **Поведение**: Отправляет `AuthenticationTokenRequest` с данными плагина и возвращает токен из ответа.

- **Функция: `bool AuthenticateRequest(const std::string& token)`**
  - **Описание**: Выполняет аутентификацию с VTube Studio, используя предоставленный токен.
  - **Параметры**:
    - `token`: Токен аутентификации.
  - **Возвращает**: `true`, если аутентификация успешна, `false` — в противном случае.
  - **Поведение**: Отправляет `AuthenticationRequest` и проверяет поле `authenticated` в ответе.

- **Функция: `json AvailableModelsRequest()`**
  - **Описание**: Получает список доступных моделей из VTube Studio.
  - **Возвращает**: JSON-массив доступных моделей.
  - **Поведение**: Отправляет `AvailableModelsRequest` и возвращает поле `availableModels` из ответа.

- **Функция: `json CurrentModelRequest()`**
  - **Описание**: Получает информацию о текущей активной модели.
  - **Возвращает**: JSON-объект с информацией о текущей модели.
  - **Поведение**: Отправляет `CurrentModelRequest` и возвращает поле `data` из ответа.

### Класс TwitchClient

Класс `TwitchClient` управляет взаимодействием с API Twitch для работы с наградами за очки канала.

- **Конструктор: `TwitchClient()`**
  - **Описание**: Инициализирует клиент Twitch и управляет токенами.
  - **Поведение**:
    - Читает токен обновления из файла `twitch_config`, если он существует, и обновляет токен доступа.
    - Если токена нет, инициирует процесс OAuth для получения нового токена.

- **Деструктор: `~TwitchClient()`**
  - **Описание**: Пустой деструктор (специальная очистка не требуется).

- **Функция: `void getAccessToken()`**
  - **Описание**: Получает новый токен доступа через процесс OAuth Twitch.
  - **Поведение**:
    - Запускает локальный HTTP-сервер для обработки обратного вызова OAuth.
    - Открывает URL авторизации Twitch в браузере по умолчанию.
    - Обменивает код авторизации на токены доступа и обновления.
    - Сохраняет токен обновления в файл `twitch_config`.

- **Функция: `void updateAccessToken()`**
  - **Описание**: Обновляет токен доступа, используя сохранённый токен обновления.
  - **Поведение**:
    - Отправляет POST-запрос на эндпоинт токенов Twitch с токеном обновления.
    - Обновляет токены доступа и обновления.
    - Сохраняет новый токен обновления в файл `twitch_config`.

- **Функция: `json getBroadcastInfo()`**
  - **Описание**: Получает информацию об авторизованном пользователе Twitch.
  - **Возвращает**: JSON-объект с данными пользователя или `NULL` при ошибке.
  - **Поведение**:
    - Отправляет GET-запрос на эндпоинт `/helix/users` Twitch.
    - Обрабатывает ошибки 401, обновляя токен и повторяя запрос.
    - Устанавливает `broadcast_id` и возвращает данные пользователя.

- **Функция: `json getCustomRewards()`**
  - **Описание**: Получает список пользовательских наград за очки канала.
  - **Возвращает**: JSON-массив наград или `NULL` при ошибке.
  - **Поведение**:
    - Убеждается, что `broadcast_id` установлен, вызывая `getBroadcastInfo()` при необходимости.
    - Отправляет GET-запрос на эндпоинт `/helix/channel_points/custom_rewards` с параметром `only_manageable_rewards=true`.
    - Обрабатывает ошибки 401, обновляя токен и повторяя запрос.

- **Функция: `void updateCustomReward(const std::string& reward_id, const std::string& title, const std::string& prompt, int cost, bool is_enabled)`**
  - **Описание**: Обновляет существующую награду за очки канала Twitch.
  - **Параметры**:
    - `reward_id`: ID награды для обновления.
    - `title`: Новый заголовок награды.
    - `prompt`: Новый текст подсказки для награды.
    - `cost`: Новая стоимость в очках канала.
    - `is_enabled`: Включена ли награда.
  - **Поведение**:
    - Убеждается, что `broadcast_id` установлен.
    - Отправляет PATCH-запрос на эндпоинт `/helix/channel_points/custom_rewards`.
    - Обрабатывает ошибки 401, обновляя токен и повторяя запрос.

- **Функция: `json createReward(json param)`**
  - **Описание**: Создаёт новую награду за очки канала Twitch.
  - **Параметры**:
    - `param`: JSON-объект, содержащий параметры награды (например, `title`, `cost`, `prompt` и т. д.).
  - **Возвращает**: JSON-объект с данными созданной награды или пустой объект при ошибке.
  - **Поведение**:
    - Убеждается, что `broadcast_id` установлен.
    - Отправляет POST-запрос на эндпоинт `/helix/channel_points/custom_rewards` с указанными параметрами.
    - Обрабатывает ошибки 401, обновляя токен и повторяя запрос.

## API-эндпоинты сервера

Сервер предоставляет следующие API-эндпоинты для управления моделями VTube Studio и наградами за очки канала Twitch.

- **GET `/`**
  - **Описание**: Возвращает статический HTML-файл `server.html` из директории `static`.
  - **Ответ**:
    - **200 OK**: Содержимое файла `server.html`.
  - **Пример**:
    ```bash
    curl http://localhost:<port>/
    ```

- **GET `/status`**
  - **Описание**: Проверяет состояние сервера.
  - **Ответ**:
    - **200 OK**: Возвращает строку `"OK"`.
  - **Пример**:
    ```bash
    curl http://localhost:<port>/status
    ```

- **GET `/model/list`**
  - **Описание**: Получает список доступных моделей из VTube Studio.
  - **Ответ**:
    - **200 OK**: JSON-массив моделей.
    - **500 Internal Server Error**: Если VTube Studio возвращает пустой список.
  - **Пример**:
    ```bash
    curl http://localhost:<port>/model/list
    ```
    ```json
    [
			{
				"modelLoaded": false,
				"modelName": "My First Model",
				"modelID": "UniqueIDToIdentifyThisModelBy1",
				"vtsModelName": "Model_1.vtube.json",
				"vtsModelIconName": "ModelIconPNGorJPG_1.png"
			},
			{
				"modelLoaded": true,
				"modelName": "My Second Model",
				"modelID": "UniqueIDToIdentifyThisModelBy2",
				"vtsModelName": "Model_2.vtube.json",
				"vtsModelIconName": "ModelIconPNGorJPG_1.png"
			}
		]
    ```

- **GET `/model/list/reward`**
  - **Описание**: Возвращает награды для всех моделей или для указанной модели из `ModelRewards.json`.
  - **Параметры запроса**:
    - `modelName` (опционально): Имя модели для получения наград.
  - **Ответ**:
    - **200 OK**: Полное содержимое `ModelRewards.json` (если `modelName` не указан) или награды для указанной модели.
    - **404 Not Found**: Если указанная модель не найдена.
  - **Пример**:
    ```bash
    curl http://localhost:<port>/model/list/reward?modelName=Model1
    ```
    ```json
    {
      "modelName": "Model1",
      "Rewards": [
        {"id": "reward1", "title": "Название награды", "prompt": "Подсказка", "cost": 100, "is_enabled": true},
        ..
      ]
    }
    ```

- **GET `/model/list/update`**
  - **Описание**: Обновляет `ModelRewards.json`, добавляя новые модели из VTube Studio с наградами первой модели в файле.
  - **Ответ**:
    - **200 OK**: Обновлённое содержимое `ModelRewards.json`.
    - **500 Internal Server Error**: Если VTube Studio возвращает пустой список моделей.
  - **Пример**:
    ```bash
    curl http://localhost:<port>/model/list/update
    ```

- **POST `/reward/create`**
  - **Описание**: Создаёт новую награду за очки канала Twitch и добавляет её ко всем моделям в `ModelRewards.json`.
  - **Тело запроса** (JSON):
    - `title` (обязательно): Название награды.
    - Другие необязательные поля: `cost`, `prompt`, `is_enabled`, `background_color` и т. д.
  - **Ответ**:
    - **200 OK**: Возвращает `"ok"` при успехе.
    - **405 Method Not Allowed**: Если в теле запроса отсутствует `title`.
  - **Пример**:
    ```bash
    curl -X POST http://localhost:<port>/reward/create -H "Content-Type: application/json" -d '{"title": "Новая награда", "cost": 100}'
    ```

- **PUT `/reward/update`**
  - **Описание**: Обновляет награды для указанной модели в `ModelRewards.json`.
  - **Тело запроса** (JSON):
    - `modelName` (обязательно): Имя модели.
    - `Rewards` (обязательно): Массив наград с полями `id`, `title`, `prompt`, `cost` и `is_enabled`.
  - **Ответ**:
    - **200 OK**: Возвращает `"OK"` при успехе.
    - **405 Method Not Allowed**: Если в теле запроса отсутствуют `modelName` или `Rewards`.
  - **Пример**:
    ```bash
    curl -X PUT http://localhost:<port>/reward/update -H "Content-Type: application/json" -d '{"modelName": "Model1", "Rewards": [{"id": "reward1", "title": "Обновлённая награда", "prompt": "Обновлённая подсказка", "cost": 200, "is_enabled": false}]}'
    ```
