#include "FirmwareUpdate.h"

FirmwareUpdate::FirmwareUpdate() :
    updating(false),
    currentState(UpdateState::IDLE),
    updateStatus("Готов"),
    updateSize(0),
    updateReceived(0),
    updateStartTime(0),
    currentProgress(0),
    autoUpdateEnabled(false),
    dontOfferUpdates(false)
{
    DEBUG_PRINTLN("FirmwareUpdate конструктор");
}

FirmwareUpdate::~FirmwareUpdate() {
    shutdown();
}

bool FirmwareUpdate::init(AsyncWebServer* server) {
    DEBUG_PRINTLN("Инициализация системы обновления прошивки...");
    
    // Загрузка настроек
    preferences.begin("firmware", false);
    autoUpdateEnabled = preferences.getBool("autoUpdate", false);
    dontOfferUpdates = preferences.getBool("dontOffer", false);
    
    // Регистрация обработчиков
    if (server) {
        registerUpdateHandlers(server);
    }
    
    DEBUG_PRINTLN("Система обновления прошивки инициализирована");
    return true;
}

void FirmwareUpdate::loop() {
    // В новой системе не нужен отдельный loop для режима обновления
    // Обновления происходят напрямую через HTTP upload
}

void FirmwareUpdate::shutdown() {
    DEBUG_PRINTLN("Выключение системы обновления...");
    preferences.end();
}

void FirmwareUpdate::registerUpdateHandlers(AsyncWebServer* server) {
    // API для загрузки прошивки
    server->on("/api/update/upload", HTTP_POST, 
        [this](AsyncWebServerRequest *request) {
            // Ответ после завершения загрузки
            if (Update.hasError()) {
                AsyncWebServerResponse *response = request->beginResponse(500, "application/json", 
                    "{\"status\":\"error\",\"message\":\"Ошибка обновления\"}");
                response->addHeader("Connection", "close");
                response->addHeader("Access-Control-Allow-Origin", "*");
                currentState = UpdateState::FAILED;
                updateStatus = "Ошибка записи прошивки";
                request->send(response);
            } else {
                AsyncWebServerResponse *response = request->beginResponse(200, "application/json", 
                    "{\"status\":\"success\",\"message\":\"Обновление завершено, перезагрузка...\"}");
                response->addHeader("Connection", "close");
                response->addHeader("Access-Control-Allow-Origin", "*");
                currentState = UpdateState::SUCCESS;
                updateStatus = "Обновление завершено";
                request->send(response);
                
                // Перезагрузка в отдельной задаче с минимальной задержкой
                // чтобы не блокировать сервер
                delay(100);
                ESP.restart();
            }
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            this->handleUpdateUpload(request, filename, index, data, len, final);
        });
    
    // API статуса обновления
    server->on("/api/update/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleUpdateStatus(request);
    });
    
    // API проверки обновлений на GitHub
    server->on("/api/update/check", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleCheckUpdates(request);
    });
    
    // API получения текущей версии
    server->on("/api/update/current", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleCurrentVersion(request);
    });
    
    // API настроек автообновления
    server->on("/api/update/settings", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            // Ответ будет отправлен в onBody
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String body = "";
            for (size_t i = 0; i < len; i++) {
                body += (char)data[i];
            }
            
            if (index + len == total) {
                // Парсим JSON более надежно
                bool enabled = false;
                bool dontOffer = false;
                
                int autoUpdatePos = body.indexOf("\"autoUpdate\"");
                if (autoUpdatePos >= 0) {
                    // Ищем значение после двоеточия
                    int colonPos = body.indexOf(":", autoUpdatePos);
                    if (colonPos >= 0) {
                        // Берем небольшой участок строки после : и ищем true
                        String valueSection = body.substring(colonPos + 1, min(colonPos + 10, (int)body.length()));
                        valueSection.trim();
                        enabled = valueSection.startsWith("true");
                    }
                    setAutoUpdateEnabled(enabled);
                }
                
                int dontOfferPos = body.indexOf("\"dontOffer\"");
                if (dontOfferPos >= 0) {
                    int colonPos = body.indexOf(":", dontOfferPos);
                    if (colonPos >= 0) {
                        String valueSection = body.substring(colonPos + 1, min(colonPos + 10, (int)body.length()));
                        valueSection.trim();
                        dontOffer = valueSection.startsWith("true");
                    }
                    setDontOfferUpdates(dontOffer);
                }
                
                String response = "{\"status\":\"ok\",\"autoUpdate\":" + String(autoUpdateEnabled ? "true" : "false") + 
                                ",\"dontOffer\":" + String(dontOfferUpdates ? "true" : "false") + "}";
                request->send(200, "application/json", response);
                body = "";
            }
        });
    
    // API получения настроек
    server->on("/api/update/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String response = "{\"autoUpdate\":" + String(autoUpdateEnabled ? "true" : "false") + 
                        ",\"dontOffer\":" + String(dontOfferUpdates ? "true" : "false") + "}";
        AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        request->send(resp);
    });
    
    DEBUG_PRINTLN("Обработчики обновлений зарегистрированы");
}

void FirmwareUpdate::handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    // Начало загрузки
    if (index == 0) {
        DEBUG_PRINTF("Начало обновления: %s\n", filename.c_str());
        
        updating = true;
        currentState = UpdateState::UPLOADING;
        updateStatus = "Загрузка прошивки";
        updateStartTime = millis();
        updateReceived = 0;
        currentProgress = 0;
        
        // Получаем размер файла
        if (request->hasHeader("Content-Length")) {
            updateSize = request->header("Content-Length").toInt();
            DEBUG_PRINTF("Размер прошивки: %d байт\n", updateSize);
        }
        
        // Начинаем обновление
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            currentState = UpdateState::FAILED;
            updateStatus = "Ошибка начала обновления";
            updating = false;
            return;
        }
    }
    
    // Запись данных
    if (len) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
            currentState = UpdateState::FAILED;
            updateStatus = "Ошибка записи данных";
            updating = false;
            return;
        }
        updateReceived += len;
        
        // Обновляем прогресс
        if (updateSize > 0) {
            int progress = (updateReceived * 100) / updateSize;
            if (progress != currentProgress) {
                currentProgress = progress;
                updateProgress(progress);
            }
        }
    }
    
    // Завершение загрузки
    if (final) {
        if (Update.end(true)) {
            DEBUG_PRINTF("Обновление успешно завершено. Размер: %u байт за %lu мс\n", 
                        index + len, millis() - updateStartTime);
            currentState = UpdateState::SUCCESS;
            updateStatus = "Обновление завершено";
            updating = false;
        } else {
            Update.printError(Serial);
            currentState = UpdateState::FAILED;
            updateStatus = "Ошибка завершения обновления";
            updating = false;
        }
    }
}

void FirmwareUpdate::handleUpdateStatus(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"progress\":" + String(currentProgress) + ",";
    json += "\"status\":\"" + updateStatus + "\",";
    json += "\"state\":" + String((int)currentState) + ",";
    json += "\"received\":" + String(updateReceived) + ",";
    json += "\"size\":" + String(updateSize) + ",";
    json += "\"updating\":" + String(updating ? "true" : "false");
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void FirmwareUpdate::handleCheckUpdates(AsyncWebServerRequest *request) {
    ReleaseInfo releaseInfo;
    bool hasUpdate = checkForUpdates(releaseInfo);
    
    String json = "{";
    json += "\"hasUpdate\":" + String(hasUpdate ? "true" : "false") + ",";
    if (hasUpdate) {
        json += "\"version\":\"" + releaseInfo.version + "\",";
        json += "\"releaseName\":\"" + releaseInfo.releaseName + "\",";
        json += "\"releaseNotes\":\"" + releaseInfo.releaseNotes + "\",";
        json += "\"downloadUrl\":\"" + releaseInfo.downloadUrl + "\",";
        json += "\"publishedAt\":\"" + releaseInfo.publishedAt + "\"";
    }
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void FirmwareUpdate::handleCurrentVersion(AsyncWebServerRequest *request) {
    ReleaseInfo currentInfo = getCurrentVersionInfo();
    
    String json = "{";
    json += "\"version\":\"" + currentInfo.version + "\",";
    json += "\"releaseName\":\"" + currentInfo.releaseName + "\",";
    json += "\"projectName\":\"" + String(PROJECT_NAME) + "\"";
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

bool FirmwareUpdate::checkForUpdates(ReleaseInfo& releaseInfo) {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi не подключен, невозможно проверить обновления");
        return false;
    }
    
    HTTPClient http;
    String url = "https://api.github.com/repos/" + String(GITHUB_REPO_URL) + "/releases/latest";
    http.begin(url);
    http.addHeader("Accept", "application/vnd.github.v3+json");
    http.addHeader("User-Agent", "MicroBox-Firmware-Updater");
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        DEBUG_PRINTLN("Получен ответ от GitHub API");
        
        if (parseGitHubRelease(payload, releaseInfo)) {
            String currentVersion = GIT_VERSION;
            releaseInfo.isNewer = isVersionNewer(currentVersion, releaseInfo.version);
            http.end();
            return releaseInfo.isNewer;
        }
    } else {
        DEBUG_PRINTF("Ошибка HTTP запроса: %d\n", httpCode);
    }
    
    http.end();
    return false;
}

ReleaseInfo FirmwareUpdate::getCurrentVersionInfo() {
    ReleaseInfo info;
    info.version = GIT_VERSION;
    info.releaseName = PROJECT_NAME;
    info.isNewer = false;
    return info;
}

void FirmwareUpdate::setAutoUpdateEnabled(bool enabled) {
    autoUpdateEnabled = enabled;
    preferences.putBool("autoUpdate", enabled);
    DEBUG_PRINTF("Автообновление %s\n", enabled ? "включено" : "выключено");
}

bool FirmwareUpdate::isAutoUpdateEnabled() const {
    return autoUpdateEnabled;
}

void FirmwareUpdate::setDontOfferUpdates(bool dontOffer) {
    dontOfferUpdates = dontOffer;
    preferences.putBool("dontOffer", dontOffer);
    DEBUG_PRINTF("Не предлагать обновления: %s\n", dontOffer ? "да" : "нет");
}

bool FirmwareUpdate::isDontOfferUpdates() const {
    return dontOfferUpdates;
}

bool FirmwareUpdate::parseGitHubRelease(const String& json, ReleaseInfo& releaseInfo) {
    // Простой парсинг JSON без библиотеки для экономии памяти
    releaseInfo.version = extractJsonValue(json, "tag_name");
    releaseInfo.releaseName = extractJsonValue(json, "name");
    releaseInfo.releaseNotes = extractJsonValue(json, "body");
    releaseInfo.publishedAt = extractJsonValue(json, "published_at");
    
    // Ищем URL для скачивания .bin файла
    int assetsPos = json.indexOf("\"assets\":");
    if (assetsPos >= 0) {
        String assetsSection = json.substring(assetsPos);
        int urlPos = assetsSection.indexOf("\"browser_download_url\":\"");
        if (urlPos >= 0) {
            int urlStart = urlPos + 24;
            int urlEnd = assetsSection.indexOf("\"", urlStart);
            String url = assetsSection.substring(urlStart, urlEnd);
            // Ищем .bin файл с release в имени (более строгая проверка)
            if (url.endsWith(".bin") && url.indexOf("-release.bin") > 0) {
                releaseInfo.downloadUrl = url;
            }
        }
    }
    
    return releaseInfo.version.length() > 0;
}

String FirmwareUpdate::extractJsonValue(const String& json, const String& key) {
    String searchKey = "\"" + key + "\":\"";
    int keyPos = json.indexOf(searchKey);
    if (keyPos < 0) return "";
    
    int valueStart = keyPos + searchKey.length();
    int valueEnd = json.indexOf("\"", valueStart);
    if (valueEnd < 0) return "";
    
    return json.substring(valueStart, valueEnd);
}

bool FirmwareUpdate::isVersionNewer(const String& current, const String& latest) {
    // Простое сравнение версий (v1.2.3 формат)
    // Удаляем 'v' если есть
    String cur = current;
    String lat = latest;
    
    if (cur.startsWith("v")) cur = cur.substring(1);
    if (lat.startsWith("v")) lat = lat.substring(1);
    
    // Разбиваем на компоненты безопасно
    int curMajor = 0, curMinor = 0, curPatch = 0;
    int latMajor = 0, latMinor = 0, latPatch = 0;
    
    // Безопасный парсинг версии
    int dot1Cur = cur.indexOf('.');
    int dot2Cur = cur.indexOf('.', dot1Cur + 1);
    int dot1Lat = lat.indexOf('.');
    int dot2Lat = lat.indexOf('.', dot1Lat + 1);
    
    if (dot1Cur > 0) {
        curMajor = cur.substring(0, dot1Cur).toInt();
        if (dot2Cur > 0) {
            curMinor = cur.substring(dot1Cur + 1, dot2Cur).toInt();
            curPatch = cur.substring(dot2Cur + 1).toInt();
        } else {
            curMinor = cur.substring(dot1Cur + 1).toInt();
        }
    }
    
    if (dot1Lat > 0) {
        latMajor = lat.substring(0, dot1Lat).toInt();
        if (dot2Lat > 0) {
            latMinor = lat.substring(dot1Lat + 1, dot2Lat).toInt();
            latPatch = lat.substring(dot2Lat + 1).toInt();
        } else {
            latMinor = lat.substring(dot1Lat + 1).toInt();
        }
    }
    
    if (latMajor > curMajor) return true;
    if (latMajor < curMajor) return false;
    
    if (latMinor > curMinor) return true;
    if (latMinor < curMinor) return false;
    
    return latPatch > curPatch;
}

void FirmwareUpdate::updateProgress(int progress) {
    // Обновление прогресса
    DEBUG_PRINTF("Прогресс: %d%%\n", progress);
}