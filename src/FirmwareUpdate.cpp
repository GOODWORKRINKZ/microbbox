#include "FirmwareUpdate.h"
#ifdef USE_EMBEDDED_RESOURCES
#include "embedded_resources.h"
#endif

FirmwareUpdate::FirmwareUpdate() :
    updateModeActive(false),
    updating(false),
    currentState(UpdateState::IDLE),
    updateStatus("Готов"),
    updateServer(nullptr),
    updateSize(0),
    updateReceived(0),
    updateStartTime(0),
    currentProgress(0)
{
    DEBUG_PRINTLN("FirmwareUpdate конструктор");
}

FirmwareUpdate::~FirmwareUpdate() {
    shutdown();
}

bool FirmwareUpdate::init() {
    DEBUG_PRINTLN("Инициализация системы обновления прошивки...");
    
    // Инициализация веб-сервера для обновлений
    initUpdateWebServer();
    
    DEBUG_PRINTLN("Система обновления прошивки инициализирована");
    return true;
}

void FirmwareUpdate::loop() {
    if (!updateModeActive) {
        return;
    }
    
    // Обработка состояний обновления
    switch (currentState) {
        case UpdateState::WAITING_FOR_CLIENT:
            // Проверяем подключение клиентов
            if (WiFi.softAPgetStationNum() > 0) {
                currentState = UpdateState::CLIENT_CONNECTED;
                updateStatus = "Клиент подключен";
                DEBUG_PRINTLN("Клиент подключился для обновления");
            }
            break;
            
        case UpdateState::CLIENT_CONNECTED:
            // Ожидаем начала загрузки
            if (WiFi.softAPgetStationNum() == 0) {
                currentState = UpdateState::WAITING_FOR_CLIENT;
                updateStatus = "Ожидание подключения";
                DEBUG_PRINTLN("Клиент отключился");
            }
            break;
            
        case UpdateState::UPLOADING:
            // Показываем прогресс
            if (updateSize > 0) {
                int progress = (updateReceived * 100) / updateSize;
                if (progress != currentProgress) {
                    currentProgress = progress;
                    updateProgress(progress);
                    DEBUG_PRINTF("Прогресс загрузки: %d%%\n", progress);
                }
            }
            break;
            
        case UpdateState::SUCCESS:
            // Обновление успешно завершено
            showUpdateResult(true, "Обновление прошивки завершено успешно! Перезагрузка через 3 секунды...");
            delay(3000);
            ESP.restart();
            break;
            
        case UpdateState::FAILED:
            // Ошибка обновления
            showUpdateResult(false, "Ошибка обновления прошивки: " + updateStatus);
            delay(5000);
            currentState = UpdateState::WAITING_FOR_CLIENT;
            updateStatus = "Ожидание подключения";
            break;
            
        default:
            break;
    }
}

void FirmwareUpdate::shutdown() {
    DEBUG_PRINTLN("Выключение системы обновления...");
    
    exitUpdateMode();
    
    if (updateServer) {
        delete updateServer;
        updateServer = nullptr;
    }
}

void FirmwareUpdate::startUpdateMode() {
    DEBUG_PRINTLN("Запуск режима обновления прошивки...");
    
    updateModeActive = true;
    currentState = UpdateState::WAITING_FOR_CLIENT;
    updateStatus = "Ожидание подключения";
    
    // Остановка камеры для освобождения памяти
    esp_camera_deinit();
    
    // Настройка WiFi для обновления
    WiFi.mode(WIFI_AP);
    WiFi.softAP("МикроББокс-Обновление", "12345678");
    
    IPAddress ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gateway, subnet);
    
    // Запуск сервера обновлений
    updateServer->begin();
    
    showUpdateScreen();
    
    DEBUG_PRINT("Режим обновления активен. IP: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
}

void FirmwareUpdate::exitUpdateMode() {
    if (!updateModeActive) {
        return;
    }
    
    DEBUG_PRINTLN("Выход из режима обновления");
    
    updateModeActive = false;
    currentState = UpdateState::IDLE;
    
    if (updateServer) {
        updateServer->end();
    }
}

void FirmwareUpdate::initUpdateWebServer() {
    updateServer = new AsyncWebServer(80);
    
    // Главная страница обновления
    updateServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleUpdateRoot(request);
    });
    
    // Страница загрузки прошивки
    updateServer->on("/update", HTTP_POST, 
        [this](AsyncWebServerRequest *request) {
            // Ответ после завершения загрузки
            AsyncWebServerResponse *response;
            if (Update.hasError()) {
                response = request->beginResponse(500, "text/plain", "Ошибка обновления");
                currentState = UpdateState::FAILED;
                updateStatus = "Ошибка записи прошивки";
            } else {
                response = request->beginResponse(200, "text/plain", "Обновление успешно");
                currentState = UpdateState::SUCCESS;
                updateStatus = "Обновление завершено";
            }
            response->addHeader("Connection", "close");
            request->send(response);
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            this->handleUpdateUpload(request, filename, index, data, len, final);
        });
    
    // API статуса
    updateServer->on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleUpdateStatus(request);
    });
    
    // API перезагрузки
    updateServer->on("/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleUpdateRestart(request);
    });
    
    DEBUG_PRINTLN("Веб-сервер обновлений настроен");
}

void FirmwareUpdate::handleUpdateRoot(AsyncWebServerRequest *request) {
    #ifdef USE_EMBEDDED_RESOURCES
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html; charset=utf-8", updateHtml, updateHtml_len);
    #else
    // Fallback HTML если ресурсы не встроены
    String html = "<!DOCTYPE html><html><head><title>МикроББокс - Обновление</title></head><body>";
    html += "<h1>МикроББокс - Обновление прошивки</h1>";
    html += "<p>Система обновления готова, но веб-ресурсы не загружены.</p>";
    html += "<form method='post' action='/update' enctype='multipart/form-data'>";
    html += "<input type='file' name='update' accept='.bin' required><br>";
    html += "<input type='submit' value='Загрузить прошивку'>";
    html += "</form></body></html>";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    #endif
    
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}

void FirmwareUpdate::handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    // Начало загрузки
    if (index == 0) {
        DEBUG_PRINTF("Начало обновления: %s\n", filename.c_str());
        
        currentState = UpdateState::UPLOADING;
        updateStatus = "Загрузка прошивки";
        updateStartTime = millis();
        updateReceived = 0;
        
        // Получаем размер файла из заголовка Content-Length
        if (request->hasHeader("Content-Length")) {
            updateSize = request->header("Content-Length").toInt();
            DEBUG_PRINTF("Размер прошивки: %d байт\n", updateSize);
        }
        
        // Начинаем обновление
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            currentState = UpdateState::FAILED;
            updateStatus = "Ошибка начала обновления";
            return;
        }
    }
    
    // Запись данных
    if (len) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
            currentState = UpdateState::FAILED;
            updateStatus = "Ошибка записи данных";
            return;
        }
        updateReceived += len;
    }
    
    // Завершение загрузки
    if (final) {
        if (Update.end(true)) {
            DEBUG_PRINTF("Обновление успешно завершено. Размер: %u байт за %lu мс\n", 
                        index + len, millis() - updateStartTime);
            currentState = UpdateState::SUCCESS;
            updateStatus = "Обновление завершено";
        } else {
            Update.printError(Serial);
            currentState = UpdateState::FAILED;
            updateStatus = "Ошибка завершения обновления";
        }
    }
}

void FirmwareUpdate::handleUpdateStatus(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"progress\":" + String(currentProgress) + ",";
    json += "\"status\":\"" + updateStatus + "\",";
    json += "\"state\":" + String((int)currentState) + ",";
    json += "\"received\":" + String(updateReceived) + ",";
    json += "\"size\":" + String(updateSize);
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void FirmwareUpdate::handleUpdateRestart(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Перезагрузка...");
    delay(1000);
    ESP.restart();
}

void FirmwareUpdate::showUpdateScreen() {
    DEBUG_PRINTLN("=================================");
    DEBUG_PRINTLN("     РЕЖИМ ОБНОВЛЕНИЯ ПРОШИВКИ   ");
    DEBUG_PRINTLN("=================================");
    DEBUG_PRINTLN("WiFi: МикроББокс-Обновление");
    DEBUG_PRINTLN("Пароль: 12345678");
    DEBUG_PRINTLN("IP: 192.168.4.1");
    DEBUG_PRINTLN("Откройте браузер и перейдите:");
    DEBUG_PRINTLN("http://192.168.4.1");
    DEBUG_PRINTLN("=================================");
}

void FirmwareUpdate::updateProgress(int progress) {
    // Обновление прогресса можно вывести на дисплей или в Serial
    DEBUG_PRINTF("Прогресс: %d%%\n", progress);
}

void FirmwareUpdate::showUpdateResult(bool success, const String& message) {
    DEBUG_PRINTLN("=================================");
    if (success) {
        DEBUG_PRINTLN("  ✓ ОБНОВЛЕНИЕ ЗАВЕРШЕНО УСПЕШНО");
    } else {
        DEBUG_PRINTLN("  ✗ ОШИБКА ОБНОВЛЕНИЯ");
    }
    DEBUG_PRINTLN("=================================");
    DEBUG_PRINTLN(message);
    DEBUG_PRINTLN("=================================");
}