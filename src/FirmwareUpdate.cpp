#include "FirmwareUpdate.h"
#include "esp_task_wdt.h"

FirmwareUpdate::FirmwareUpdate() :
    updating(false),
    currentState(UpdateState::IDLE),
    updateStatus("Готов"),
    updateSize(0),
    updateReceived(0),
    updateStartTime(0),
    currentProgress(0),
    shouldReboot(false),
    rebootScheduledTime(0),
    autoUpdateEnabled(false),
    dontOfferUpdates(false)
{
    DEBUG_PRINTLN("FirmwareUpdate конструктор");
    
    // Определяем тип робота из конфигурации компиляции
    #ifdef TARGET_CLASSIC
        robotType_ = RobotType::CLASSIC;
    #elif defined(TARGET_LINER)
        robotType_ = RobotType::LINER;
    #elif defined(TARGET_BRAIN)
        robotType_ = RobotType::BRAIN;
    #else
        robotType_ = RobotType::UNKNOWN;
    #endif
    
    DEBUG_PRINT("Тип робота для обновлений: ");
    DEBUG_PRINTLN(robotTypeToString(robotType_));
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
    // Проверяем, нужна ли отложенная перезагрузка
    if (shouldReboot && millis() >= rebootScheduledTime) {
        DEBUG_PRINTLN("Выполняем отложенную перезагрузку...");
        ESP.restart();
    }
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
                
                // Планируем перезагрузку через 1 секунду, чтобы ответ успел отправиться
                shouldReboot = true;
                rebootScheduledTime = millis() + 1000;
                DEBUG_PRINTLN("Перезагрузка запланирована через 1 секунду");
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
                        int endPos = colonPos + 10;
                        if (endPos > (int)body.length()) endPos = body.length();
                        String valueSection = body.substring(colonPos + 1, endPos);
                        valueSection.trim();
                        enabled = valueSection.startsWith("true");
                    }
                    setAutoUpdateEnabled(enabled);
                }
                
                int dontOfferPos = body.indexOf("\"dontOffer\"");
                if (dontOfferPos >= 0) {
                    int colonPos = body.indexOf(":", dontOfferPos);
                    if (colonPos >= 0) {
                        int endPos = colonPos + 10;
                        if (endPos > (int)body.length()) endPos = body.length();
                        String valueSection = body.substring(colonPos + 1, endPos);
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
    
    // API для автоматического скачивания и установки прошивки
    server->on("/api/update/download", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            this->handleDownloadAndInstall(request);
        });
    
    // API для проверки необходимости выбора типа робота (миграция с 0.0.1)
    server->on("/api/update/needs-robot-type", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool needs = FirmwareUpdate::needsRobotTypeSelection();
        String response = "{\"needsSelection\":" + String(needs ? "true" : "false") + "}";
        AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        request->send(resp);
    });
    
    // API для получения доступных типов роботов
    server->on("/api/update/robot-types", HTTP_GET, [](AsyncWebServerRequest *request) {
        String response = "{\"types\":[";
        response += "{\"id\":\"classic\",\"name\":\"МикроБокс Классик\",\"description\":\"Полнофункциональный управляемый робот\"},";
        response += "{\"id\":\"liner\",\"name\":\"МикроБокс Лайнер\",\"description\":\"Автономный робот следующий по линии\"},";
        response += "{\"id\":\"brain\",\"name\":\"МикроБокс Брейн\",\"description\":\"Модуль управления для других роботов\"}";
        response += "]}";
        AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        request->send(resp);
    });
    
    // API для сохранения выбранного типа робота
    server->on("/api/update/set-robot-type", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String body = "";
            for (size_t i = 0; i < len; i++) {
                body += (char)data[i];
            }
            
            if (index + len == total) {
                // Извлекаем тип робота из JSON
                int typePos = body.indexOf("\"type\"");
                if (typePos >= 0) {
                    int colonPos = body.indexOf(":", typePos);
                    if (colonPos >= 0) {
                        int quoteStart = body.indexOf("\"", colonPos);
                        int quoteEnd = body.indexOf("\"", quoteStart + 1);
                        if (quoteStart >= 0 && quoteEnd >= 0) {
                            String typeStr = body.substring(quoteStart + 1, quoteEnd);
                            RobotType type = stringToRobotType(typeStr);
                            
                            if (type != RobotType::UNKNOWN) {
                                FirmwareUpdate::setUserSelectedRobotType(type);
                                String response = "{\"status\":\"ok\",\"type\":\"" + typeStr + "\"}";
                                request->send(200, "application/json", response);
                            } else {
                                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid robot type\"}");
                            }
                        } else {
                            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON format\"}");
                        }
                    } else {
                        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON format\"}");
                    }
                } else {
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing type field\"}");
                }
                body = "";
            }
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
    // Проверяем, нужен ли выбор типа робота перед проверкой обновлений
    bool needsTypeSelection = needsRobotTypeSelection();
    
    String json = "{";
    json += "\"needsRobotTypeSelection\":" + String(needsTypeSelection ? "true" : "false");
    
    if (!needsTypeSelection) {
        ReleaseInfo releaseInfo;
        bool hasUpdate = checkForUpdates(releaseInfo);
        
        json += ",\"hasUpdate\":" + String(hasUpdate ? "true" : "false");
        if (hasUpdate) {
            json += ",\"version\":\"" + releaseInfo.version + "\"";
            json += ",\"releaseName\":\"" + releaseInfo.releaseName + "\"";
            json += ",\"releaseNotes\":\"" + releaseInfo.releaseNotes + "\"";
            json += ",\"downloadUrl\":\"" + releaseInfo.downloadUrl + "\"";
            json += ",\"publishedAt\":\"" + releaseInfo.publishedAt + "\"";
        }
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
    
    // Определяем тип робота: либо из конфигурации компиляции, либо из выбора пользователя
    RobotType targetRobotType = robotType_;
    if (hasUserSelectedRobotType()) {
        targetRobotType = getUserSelectedRobotType();
        DEBUG_PRINTF("Используется выбранный пользователем тип: %s\n", robotTypeToString(targetRobotType));
    }
    releaseInfo.robotType = targetRobotType;
    
    // Получаем строковое представление типа робота (lowercase)
    String robotTypeStr = robotTypeToLowerString(targetRobotType);
    
    // Ищем URL для скачивания .bin файла для нашего типа робота
    // Формат имени: microbox-{type}-{version}-release.bin
    // Примеры: microbox-classic-v0.1.0-release.bin, microbox-liner-v0.1.0-release.bin
    String targetFilename = "microbox-" + robotTypeStr + "-" + releaseInfo.version + "-release.bin";
    
    int assetsPos = json.indexOf("\"assets\":");
    if (assetsPos >= 0) {
        String assetsSection = json.substring(assetsPos);
        
        // Ищем все browser_download_url в assets
        int searchPos = 0;
        while (true) {
            int urlPos = assetsSection.indexOf("\"browser_download_url\":\"", searchPos);
            if (urlPos < 0) break;
            
            int urlStart = urlPos + 24;
            int urlEnd = assetsSection.indexOf("\"", urlStart);
            if (urlEnd < 0) break;
            
            String url = assetsSection.substring(urlStart, urlEnd);
            
            // Проверяем, содержит ли URL наш тип робота
            if (url.indexOf(targetFilename) >= 0 || 
                (url.endsWith("-release.bin") && url.indexOf(robotTypeStr) >= 0)) {
                releaseInfo.downloadUrl = url;
                DEBUG_PRINTF("Найден бинарник для %s: %s\n", robotTypeStr.c_str(), url.c_str());
                break;
            }
            
            searchPos = urlEnd;
        }
        
        // Если не нашли специфичный файл, пробуем найти универсальный (для обратной совместимости)
        if (releaseInfo.downloadUrl.length() == 0) {
            DEBUG_PRINTLN("Специфичный бинарник не найден, ищем универсальный...");
            int urlPos = assetsSection.indexOf("\"browser_download_url\":\"");
            if (urlPos >= 0) {
                int urlStart = urlPos + 24;
                int urlEnd = assetsSection.indexOf("\"", urlStart);
                String url = assetsSection.substring(urlStart, urlEnd);
                if (url.endsWith("-release.bin")) {
                    releaseInfo.downloadUrl = url;
                    DEBUG_PRINTF("Используется универсальный бинарник: %s\n", url.c_str());
                }
            }
        }
    }
    
    if (releaseInfo.downloadUrl.length() == 0) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Не найден подходящий бинарник для обновления!");
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

void FirmwareUpdate::handleDownloadAndInstall(AsyncWebServerRequest *request) {
    // Получаем URL из параметров запроса
    if (!request->hasParam("url", true)) {
        AsyncWebServerResponse *response = request->beginResponse(400, "application/json", 
            "{\"status\":\"error\",\"message\":\"URL не указан\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }
    
    String url = request->getParam("url", true)->value();
    
    // Проверяем, что уже не идет обновление
    if (updating) {
        AsyncWebServerResponse *response = request->beginResponse(409, "application/json", 
            "{\"status\":\"error\",\"message\":\"Обновление уже в процессе\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }
    
    // НОВЫЙ ПОДХОД: Сохраняем URL в EEPROM и перезагружаемся в безопасном режиме
    // Это освободит память камеры и веб-сервера для OTA обновления
    DEBUG_PRINTLN("Сохраняем URL обновления и планируем перезагрузку в безопасном режиме...");
    
    // Сохраняем URL обновления
    // ВАЖНО: Используем локальную переменную, а не member variable preferences,
    // который уже открыт с namespace "firmware"
    Preferences otaPrefs;
    if (!otaPrefs.begin("ota", false)) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось открыть preferences для сохранения URL");
        AsyncWebServerResponse *response = request->beginResponse(500, "application/json", 
            "{\"status\":\"error\",\"message\":\"Ошибка сохранения URL обновления\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }
    
    size_t urlLen = otaPrefs.putString("url", url);
    if (urlLen == 0) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось сохранить URL в EEPROM");
        otaPrefs.end();
        AsyncWebServerResponse *response = request->beginResponse(500, "application/json", 
            "{\"status\":\"error\",\"message\":\"Ошибка записи URL в память\"}");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }
    
    DEBUG_PRINTF("URL сохранен в EEPROM (длина: %d): %s\n", urlLen, url.c_str());
    
    // Устанавливаем флаг OTA (через тот же namespace)
    otaPrefs.putBool("pending", true);
    otaPrefs.end();
    DEBUG_PRINTLN("OTA pending flag set to: true");
    
    // Отправляем ответ с информацией о перезагрузке
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", 
        "{\"status\":\"ok\",\"message\":\"Устройство перезагружается для обновления\",\"rebooting\":true}");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    
    // Даем время на отправку ответа и перезагружаемся
    delay(1000);
    ESP.restart();
}

bool FirmwareUpdate::downloadAndInstallFirmware(const String& url) {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi не подключен");
        currentState = UpdateState::FAILED;
        updateStatus = "WiFi не подключен";
        return false;
    }
    
    DEBUG_PRINTF("Скачивание прошивки с: %s\n", url.c_str());
    
    // Обновляем статус (уже должен быть установлен в вызывающем коде, но на всякий случай)
    currentState = UpdateState::DOWNLOADING;
    updateStatus = "Скачивание прошивки с GitHub";
    
    // КРИТИЧНО: Сбрасываем watchdog перед началом HTTP соединения
    esp_task_wdt_reset();
    yield();
    
    HTTPClient http;
    http.begin(url);
    http.addHeader("User-Agent", "MicroBox-Firmware-Updater");
    
    // Следуем редиректам GitHub
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    
    // КРИТИЧНО: Сбрасываем watchdog перед GET запросом (SSL handshake может занять время)
    esp_task_wdt_reset();
    yield();
    
    int httpCode = http.GET();
    
    // КРИТИЧНО: Сбрасываем watchdog после завершения GET запроса
    esp_task_wdt_reset();
    yield();
    
    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTF("Ошибка HTTP: %d\n", httpCode);
        currentState = UpdateState::FAILED;
        updateStatus = "Ошибка скачивания: HTTP " + String(httpCode);
        http.end();
        updating = false;
        return false;
    }
    
    updateSize = http.getSize();
    DEBUG_PRINTF("Размер прошивки: %d байт\n", updateSize);
    
    // Проверяем размер файла
    if (updateSize <= 0) {
        DEBUG_PRINTLN("Размер прошивки неизвестен, используем UPDATE_SIZE_UNKNOWN");
        // Для Update.begin используем UPDATE_SIZE_UNKNOWN
        updateSize = 0; // Внутри храним 0 для логики прогресса
    }
    
    // КРИТИЧНО: Сбрасываем watchdog перед началом OTA обновления
    esp_task_wdt_reset();
    yield();
    
    // Начинаем OTA обновление
    size_t updateCapacity = (updateSize > 0) ? updateSize : UPDATE_SIZE_UNKNOWN;
    if (!Update.begin(updateCapacity)) {
        Update.printError(Serial);
        currentState = UpdateState::FAILED;
        updateStatus = "Ошибка начала обновления";
        http.end();
        updating = false;
        return false;
    }
    
    currentState = UpdateState::UPLOADING;
    updateStatus = "Запись прошивки";
    
    // Получаем stream и записываем данные
    WiFiClient* stream = http.getStreamPtr();
    
    uint8_t buff[512] = { 0 };
    unsigned long lastDataTime = millis();
    const unsigned long DOWNLOAD_TIMEOUT_MS = 30000; // 30 секунд таймаут без данных
    int noDataCounter = 0; // Счетчик проверок без данных
    const int MAX_NO_DATA_CHECKS = 50; // Максимум 50 проверок без данных (с yield между ними)
    
    // Скачиваем файл полностью
    // Используем updateSize если известен, иначе читаем до конца потока
    while (http.connected()) {
        size_t available = stream->available();
        
        if (available) {
            // Сбрасываем таймер и счетчик при получении данных
            lastDataTime = millis();
            noDataCounter = 0;
            
            // Читаем данные порциями
            int readLen = stream->readBytes(buff, ((available > sizeof(buff)) ? sizeof(buff) : available));
            
            if (readLen > 0) {
                // Записываем в Update
                if (Update.write(buff, readLen) != (size_t)readLen) {
                    Update.printError(Serial);
                    currentState = UpdateState::FAILED;
                    updateStatus = "Ошибка записи прошивки";
                    http.end();
                    updating = false;
                    return false;
                }
                
                updateReceived += readLen;
                
                // Обновляем прогресс
                if (updateSize > 0) {
                    int progress = (updateReceived * 100) / updateSize;
                    if (progress != currentProgress) {
                        currentProgress = progress;
                        updateProgress(progress);
                    }
                }
                
                // Если известен размер и достигли его - выходим
                if (updateSize > 0 && updateReceived >= updateSize) {
                    break;
                }
            }
        } else {
            // Нет доступных данных
            noDataCounter++;
            
            // Проверяем таймаут без данных
            if (millis() - lastDataTime > DOWNLOAD_TIMEOUT_MS) {
                DEBUG_PRINTLN("Таймаут загрузки - нет данных более 30 секунд");
                currentState = UpdateState::FAILED;
                updateStatus = "Таймаут загрузки";
                http.end();
                updating = false;
                return false;
            }
            
            // Для неизвестного размера: если долго нет данных и соединение открыто,
            // считаем что загрузка завершена
            if (updateSize == 0 && noDataCounter >= MAX_NO_DATA_CHECKS) {
                DEBUG_PRINTLN("Достигнут конец потока (неизвестный размер)");
                break;
            }
        }
        
        // КРИТИЧНО: Сбрасываем watchdog и даем время другим задачам
        esp_task_wdt_reset();
        yield();
        delay(1); // Минимальная задержка для стабильности async_tcp
    }
    
    http.end();
    
    // КРИТИЧНО: Сбрасываем watchdog перед завершением обновления
    esp_task_wdt_reset();
    yield();
    
    // Завершаем обновление
    if (Update.end(true)) {
        DEBUG_PRINTF("Обновление успешно. Размер: %u байт за %lu мс\n", 
                    updateReceived, millis() - updateStartTime);
        currentState = UpdateState::SUCCESS;
        updateStatus = "Обновление завершено";
        updating = false;
        return true;
    } else {
        Update.printError(Serial);
        currentState = UpdateState::FAILED;
        updateStatus = "Ошибка завершения обновления";
        updating = false;
        return false;
    }
}

// Static методы для управления флагом OTA обновления
bool FirmwareUpdate::isOTAPending() {
    Preferences prefs;
    if (!prefs.begin("ota", true)) {  // Read-only
        DEBUG_PRINTLN("ОШИБКА: Не удалось открыть preferences для чтения OTA флага");
        return false;  // По умолчанию считаем что OTA не ожидается
    }
    bool pending = prefs.getBool("pending", false);
    prefs.end();
    return pending;
}

void FirmwareUpdate::setOTAPending(bool pending) {
    Preferences prefs;
    if (!prefs.begin("ota", false)) {  // Read-write
        DEBUG_PRINTLN("ОШИБКА: Не удалось открыть preferences для записи OTA флага");
        return;
    }
    prefs.putBool("pending", pending);
    prefs.end();
    DEBUG_PRINTF("OTA pending flag set to: %s\n", pending ? "true" : "false");
}

void FirmwareUpdate::clearOTAPending() {
    Preferences prefs;
    if (!prefs.begin("ota", false)) {  // Read-write
        DEBUG_PRINTLN("ОШИБКА: Не удалось открыть preferences для очистки OTA данных");
        return;
    }
    
    // Очищаем флаг pending
    prefs.putBool("pending", false);
    
    // ВАЖНО: Также очищаем сохраненный URL, чтобы избежать путаницы
    prefs.remove("url");
    
    prefs.end();
    DEBUG_PRINTLN("OTA pending flag and URL cleared");
}

// ═══════════════════════════════════════════════════════════════
// МЕТОДЫ ДЛЯ МИГРАЦИИ С ВЕРСИИ 0.0.1 НА 0.1
// ═══════════════════════════════════════════════════════════════

bool FirmwareUpdate::hasUserSelectedRobotType() {
    Preferences prefs;
    if (!prefs.begin("robotType", true)) {  // Read-only
        return false;
    }
    bool hasType = prefs.isKey("selected");
    prefs.end();
    return hasType;
}

RobotType FirmwareUpdate::getUserSelectedRobotType() {
    Preferences prefs;
    if (!prefs.begin("robotType", true)) {  // Read-only
        return RobotType::UNKNOWN;
    }
    int typeInt = prefs.getInt("selected", 0);
    prefs.end();
    return intToRobotType(typeInt);
}

void FirmwareUpdate::setUserSelectedRobotType(RobotType type) {
    Preferences prefs;
    if (!prefs.begin("robotType", false)) {  // Read-write
        DEBUG_PRINTLN("ОШИБКА: Не удалось открыть preferences для сохранения типа робота");
        return;
    }
    prefs.putInt("selected", robotTypeToInt(type));
    prefs.end();
    DEBUG_PRINTF("Выбран тип робота: %s\n", robotTypeToString(type));
}

bool FirmwareUpdate::needsRobotTypeSelection() {
    // Проверяем текущую версию
    String currentVersion = GIT_VERSION;
    
    // Если версия 0.0.X и нет сохраненного типа робота - нужен выбор
    if (currentVersion.startsWith("v0.0.") || currentVersion.startsWith("0.0.")) {
        return !hasUserSelectedRobotType();
    }
    
    // Для версий 0.1+ тип определяется на этапе компиляции
    return false;
}