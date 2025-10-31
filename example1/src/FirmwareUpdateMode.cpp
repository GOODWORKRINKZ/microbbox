#include "FirmwareUpdateMode.h"
#include <Update.h>
#include <ESPmDNS.h>
#include <utils.h>
FirmwareUpdateMode *FirmwareUpdateMode::instance = nullptr;

static bool target_seen = false;
static std::string target_found;
static bool target_complete = false;
static size_t target_pos = 0;
static size_t totalSize = 0;
static const char target_prefix[] = "FILIN_";
static const size_t target_prefix_size = sizeof(target_prefix) - 1;
FirmwareUpdateMode::FirmwareUpdateMode(TFT_eSPI &tft) : tft(tft), server(WIFI_PORT), state(UpdateState::WAIT_FOR_CLIENT)
{
    instance = this;
}

void FirmwareUpdateMode::init()
{
    // Инициализация дисплея и WiFi
    WiFi.softAP(WIFI_SSID, (const char *)WIFI_PASSWORD);
    WiFi.softAPConfig(WIFI_IP, WIFI_IP, IPAddress(255, 255, 255, 0));
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    // Инициализация и запуск DNS сервера
    dnsServer.start(53, "*", IP);
    // Инициализация mDNS
    if (!MDNS.begin("filin.local"))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    else
    {
        Serial.println("mDNS responder started. You can access the filin at http://filin.local");
    }
    initServer();
    // Запуск веб-сервера
    server.begin();
    tft.init();
    tft.loadFont(FontDejaVu20);
    showWiFiScreen();
    // Инициализация пинов кнопок
    pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);

    // Инициализация пина для буззера
    pinMode(BUZZER_PIN, OUTPUT);
}

void FirmwareUpdateMode::update()
{
    // Обработка DNS запросов
    dnsServer.processNextRequest();
    // ESPAsyncWebServer сам обрабатывает события, здесь ничего не требуется делать

    // Проверка кнопок для калибровки
    checkButtons();

    switch (state)
    {
    case UpdateState::WAIT_FOR_CLIENT:
        if (WiFi.softAPgetStationNum() > 0)
        {
            userConnected();
        }
        break;

    case UpdateState::CLIENT_CONNECTED:
        if (WiFi.softAPgetStationNum() == 0)
        {
            userDisconnected();
        }
        break;

    case UpdateState::UPLOADING_FILE:
        if (WiFi.softAPgetStationNum() == 0)
        {
            userDisconnected();
        }
        break;

    case UpdateState::UPDATING:
        if (WiFi.softAPgetStationNum() == 0)
        {
            userDisconnected();
        }
        break;

    case UpdateState::UPDATE_FAILED:
        if (WiFi.softAPgetStationNum() == 0)
        {
            userDisconnected();
        }
        break;

    case UpdateState::UPDATE_SUCCESS:
        displayResetEffect();
        break;
    }
}

unsigned long FirmwareUpdateMode::getUpdateInterval()
{
    return 10; // Для этого режима нет задержек между обновлениями.
}

void FirmwareUpdateMode::showWiFiScreen()
{
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("Сервисный режим", TFT_WIDTH / 2, 10, 2);
    tft.drawCentreString("ФИЛИН", TFT_WIDTH / 2, 30, 2);
    tft.drawCentreString("1 Подключитесь к WiFi", TFT_WIDTH / 2, 50, 2);
    tft.unloadFont();
    tft.drawCentreString("SSID: " + String(WIFI_SSID), TFT_WIDTH / 2, 70, 2);
    tft.drawCentreString("Password: " + String(WIFI_PASSWORD), TFT_WIDTH / 2, 85, 2);
    tft.loadFont(FontDejaVu20);
    // Генерация и отображение QR кода
    QRCodeSprite qrCodeSprite(&tft);
    int spriteSize = min(TFT_WIDTH, TFT_HEIGHT) - 20; // Определяем размер спрайта, оставляя небольшие поля по краям
    qrCodeSprite.drawQrCode(wifi_qr_code, wifi_qr_code_width, wifi_qr_code_height, spriteSize);
}

void FirmwareUpdateMode::showWebLinkScreen()
{
    // Аналогично предыдущей версии
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("Сервисный режим", TFT_WIDTH / 2, 10, 2);
    tft.drawCentreString("ФИЛИН", TFT_WIDTH / 2, 30, 2);
    tft.drawCentreString("2. Откройте страницу", TFT_WIDTH / 2, 50, 2);
    tft.unloadFont();
    String url = "http://10.0.0.5";
    tft.drawCentreString(url, TFT_WIDTH / 2, 70, 2);
    tft.loadFont(FontDejaVu20);
    // Генерация и отображение QR-кода для открытия страницы
    QRCodeSprite qrCodeSprite(&tft);
    int spriteSize = min(TFT_WIDTH, TFT_HEIGHT) - 80;
    qrCodeSprite.drawQrCode(update_page_qr_code, update_page_qr_code_width, update_page_qr_code_height, spriteSize);
}

void FirmwareUpdateMode::displayResetEffect()
{
    const int lineCount = 20;                    // Количество линий для отображения эффекта
    const int lineWidth = TFT_WIDTH / lineCount; // Ширина каждого блока

    for (int i = 0; i < lineCount; ++i)
    {
        tft.fillRect(i * lineWidth, 0, lineWidth, TFT_HEIGHT, random(0xffff)); // Заполняем блок случайным цветом
        delay(50);                                                             // Задержка для создания эффекта
    }

    delay(500);    // Немного подождем перед перезагрузкой
    ESP.restart(); // Перезагружаем устройство
}

void FirmwareUpdateMode::setState(UpdateState newState)
{
    if (state == newState)
    {
        return;
    }
    state = newState;
    switch (state)
    {
    case UpdateState::WAIT_FOR_CLIENT:
        break;

    case UpdateState::CLIENT_CONNECTED:
        showWebLinkScreen();
        break;

    case UpdateState::UPLOADING_FILE:
        // Идет загрузка файла
        break;

    case UpdateState::UPDATING:
        // Идет обновление
        break;

    case UpdateState::UPDATE_FAILED:
        displayUpdateFailed();
        state = UpdateState::CLIENT_CONNECTED;
        break;

    case UpdateState::UPDATE_SUCCESS:
        displayUpdateSuccess();
        break;
    }
}

void FirmwareUpdateMode::displayUpdateSuccess()
{
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("Обновление успешно!", TFT_WIDTH / 2, 50, 2);
}

void FirmwareUpdateMode::displayUpdateFailed()
{
    tft.fillRect(0, 90, TFT_WIDTH, 40, TFT_WHITE);
    tft.drawCentreString("Ошибка обновления!", TFT_WIDTH / 2, 90, 2);
}

void FirmwareUpdateMode::drawProgressBar(float progress)
{
    int barY = 90; // Y координата прогрессбара (центр экрана по вертикали)
    TFT_eSprite bar = TFT_eSprite(&tft);
    bar.createSprite(TFT_WIDTH - 20, 40);
    bar.fillScreen(TFT_WHITE);
    // Рисуем фон прогрессбара
    bar.fillRect(0, 0, bar.width(), 20, TFT_LIGHTGREY);

    // Рисуем заполненную часть прогрессбара
    int fillWidth = progress * bar.width();
    bar.fillRect(0, 0, fillWidth, 20, TFT_BLUE);

    // Рисуем рамку прогрессбара
    bar.drawRect(0, 0, bar.width(), 20, TFT_BLACK);
    // Отображаем процент выполнения
    char progressText[10];
    snprintf(progressText, sizeof(progressText), "%.1f%%", progress * 100);
    bar.setTextColor(TFT_BLACK, TFT_WHITE); // Чёрный текст на белом фоне для предотвращения мерцания
    bar.drawCentreString(progressText, TFT_WIDTH / 2, 25, 2);
    bar.pushSprite(10, barY);
    bar.deleteSprite();
}

// Остальной метод init и функции-обработчики remain unchanged
void FirmwareUpdateMode::initServer()
{
    // Обработчик для сервирования страницы обновления прошивки
    server.on("/", HTTP_GET, handleFilinupRequest);

    // Обработчик для подачи логотипа
    server.on("/logo.svg", HTTP_GET, handleLogoRequest);

    // Обработчик для подачи favicon
    server.on("/favicon.ico", HTTP_GET, handleFaviconRequest);

    // Обработчик для подачи стилей
    server.on("/styles.css", HTTP_GET, handleStylesRequest);

    // Обработчик для подачи скриптов
    server.on("/script.js", HTTP_GET, handleScriptRequest);

    // Обработчик для получения информации об устройстве
    server.on("/device_info", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      String jsonResponse = "{";
      jsonResponse += "\"RADIO_1_2G_ENABLED\": " + String(RADIO_1_2G_ENABLED) + ",";
      jsonResponse += "\"RADIO_2_4G_ENABLED\": " + String(RADIO_2_4G_ENABLED) + ",";
      jsonResponse += "\"RADIO_5_8G_ENABLED\": " + String(RADIO_5_8G_ENABLED) + ",";
      jsonResponse += "\"FIRMWARE_VERSION\": \"" + String(APP_VERSION) + "\",";
      jsonResponse += "\"TARGET_VERSION\": \"" + String(getTargetVersion()) + "\",";
      jsonResponse += "\"MAX_CHANNELS_5_8G\": " + String(MAX_CHANNELS_5_8G) + ",";
      jsonResponse += "\"MIN_5800_FREQ\": " + String(MIN_5800_FREQ) + ",";
      jsonResponse += "\"MAX_5800_FREQ\": " + String(MAX_5800_FREQ) + ",";
      jsonResponse += "\"MAX_CHANNELS_2_4G\": " + String(MAX_CHANNELS_2_4G) + ",";
      jsonResponse += "\"MIN_2400_FREQ\": " + String(MIN_2400_FREQ) + ",";
      jsonResponse += "\"MAX_2400_FREQ\": " + String(MAX_2400_FREQ) + ",";
      jsonResponse += "\"MAX_CHANNELS_1_2G\": " + String(MAX_CHANNELS_1_2G) + ",";
      jsonResponse += "\"MIN_1200_FREQ\": " + String(MIN_1200_FREQ) + ",";
      jsonResponse += "\"MAX_1200_FREQ\": " + String(MAX_1200_FREQ) + ",";

        // Загрузка данных калибровки RSSI
        RSSICalibrationData calibData = loadCalibrationData();
        jsonResponse += "\"RSSI_BAND_1_2_MIN\": " + String(calibData.band_1_2.minRssi) + ",";
        jsonResponse += "\"RSSI_BAND_1_2_MAX\": " + String(calibData.band_1_2.maxRssi) + ",";
        jsonResponse += "\"RSSI_BAND_2_4_MIN\": " + String(calibData.band_2_4.minRssi) + ",";
        jsonResponse += "\"RSSI_BAND_2_4_MAX\": " + String(calibData.band_2_4.maxRssi) + ",";
        jsonResponse += "\"RSSI_BAND_5_8_MIN\": " + String(calibData.band_5_8.minRssi) + ",";
        jsonResponse += "\"RSSI_BAND_5_8_MAX\": " + String(calibData.band_5_8.maxRssi) + "";

      jsonResponse += "}";

      request->send(200, "application/json", jsonResponse); });
    // Обработчик для загрузки файла прошивки
    server.on("/update", HTTP_POST, handleUpdateRequest, handleUpdateUpload);

    // Handler for updating RSSI settings
    server.on("/update_rssi", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        String rssi_1_2_min, rssi_1_2_max, rssi_2_4_min, rssi_2_4_max, rssi_5_8_min, rssi_5_8_max;

        if (request->hasParam("RSSI_BAND_1_2_MIN", true)) {
            rssi_1_2_min = request->getParam("RSSI_BAND_1_2_MIN", true)->value();
        }
        if (request->hasParam("RSSI_BAND_1_2_MAX", true)) {
            rssi_1_2_max = request->getParam("RSSI_BAND_1_2_MAX", true)->value();
        }
        if (request->hasParam("RSSI_BAND_2_4_MIN", true)) {
            rssi_2_4_min = request->getParam("RSSI_BAND_2_4_MIN", true)->value();
        }
        if (request->hasParam("RSSI_BAND_2_4_MAX", true)) {
            rssi_2_4_max = request->getParam("RSSI_BAND_2_4_MAX", true)->value();
        }
        if (request->hasParam("RSSI_BAND_5_8_MIN", true)) {
            rssi_5_8_min = request->getParam("RSSI_BAND_5_8_MIN", true)->value();
        }
        if (request->hasParam("RSSI_BAND_5_8_MAX", true)) {
            rssi_5_8_max = request->getParam("RSSI_BAND_5_8_MAX", true)->value();
        }

        RSSICalibrationData calibData = loadCalibrationData();
        if (!rssi_1_2_min.isEmpty()) calibData.band_1_2.minRssi = rssi_1_2_min.toInt();
        if (!rssi_1_2_max.isEmpty()) calibData.band_1_2.maxRssi = rssi_1_2_max.toInt();
        if (!rssi_2_4_min.isEmpty()) calibData.band_2_4.minRssi = rssi_2_4_min.toInt();
        if (!rssi_2_4_max.isEmpty()) calibData.band_2_4.maxRssi = rssi_2_4_max.toInt();
        if (!rssi_5_8_min.isEmpty()) calibData.band_5_8.minRssi = rssi_5_8_min.toInt();
        if (!rssi_5_8_max.isEmpty()) calibData.band_5_8.maxRssi = rssi_5_8_max.toInt();

        saveCalibrationData(calibData);

        request->send(200, "application/json", "{\"status\":\"success\"}"); });
}

// Методы handleStylesRequest и handleScriptRequest
void FirmwareUpdateMode::handleStylesRequest(AsyncWebServerRequest *request)
{
    request->send_P(200, "text/css", stylesCss, stylesCss_len);
}

void FirmwareUpdateMode::handleScriptRequest(AsyncWebServerRequest *request)
{
    request->send_P(200, "application/javascript", scriptJs, scriptJs_len);
}

void FirmwareUpdateMode::handleFilinupRequest(AsyncWebServerRequest *request)
{
    request->send_P(200, "text/html", updatePageHtml, updatePageHtml_len);
}

void FirmwareUpdateMode::handleLogoRequest(AsyncWebServerRequest *request)
{
    request->send_P(200, "image/svg+xml", logoSvg, logoSvg_len);
}

void FirmwareUpdateMode::handleFaviconRequest(AsyncWebServerRequest *request)
{
    request->send_P(200, "image/x-icon", faviconIco, faviconIco_len);
}
void FirmwareUpdateMode::handleUpdateRequest(AsyncWebServerRequest *request)
{
    FirmwareUpdateMode *instance = FirmwareUpdateMode::instance;
    request->send((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? "Файл обновления прошивки поврежден!" : "Обновление успешно завершено!");
    if (!Update.hasError())
    {
        instance->setState(UpdateState::UPDATE_SUCCESS);
    }
    else
    {
        instance->setState(UpdateState::UPDATE_FAILED);
    }
}

void FirmwareUpdateMode::handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    FirmwareUpdateMode *instance = FirmwareUpdateMode::instance;
    static std::string target_identifier = "FILIN_";
    static std::string version_str;
    size_t filesize = request->header("X-FileSize").toInt();

    if (index == 0)
    {
        Serial.printf("Update: '%s' size %u\n", filename.c_str(), filesize);

        if (!Update.begin(filesize, U_FLASH))
        { // Начать с макс. доступного размера
            Update.printError(Serial);
            instance->setState(UpdateState::UPDATE_FAILED);
        }

        target_seen = false;
        target_found.clear();
        target_complete = false;
        target_pos = 0;
        totalSize = 0;
        version_str.clear();
    }

    if (len)
    {
        if (Update.write(data, len) == len)
        {
            totalSize += len;

            // Обновляем прогрессбар
            float progress = (float)totalSize / filesize;
            instance->drawProgressBar(progress);

            // Поиск версии прошивки в загружаемом файле
            for (size_t i = 0; i < len; i++)
            {
                if (!target_complete && target_seen)
                {
                    target_found += (char)data[i];
                    if (target_found.size() == target_identifier.size() + 3)
                    {
                        if (target_found.size() >= target_identifier.size() + 3 &&
                            (target_found.substr(target_identifier.size(), 3).find_first_not_of("01") == std::string::npos))
                        {
                            target_complete = true;
                            version_str = target_found.substr(target_identifier.size(), 3);
                            target_seen = false;
                        }
                        else
                        {
                            target_seen = false;
                            target_found.clear();
                        }
                    }
                }
                else if (data[i] == target_identifier[target_pos])
                {
                    target_found += (char)data[i];
                    if (++target_pos == target_identifier.size())
                    {
                        target_seen = true;
                        target_pos = 0;
                    }
                }
                else
                {
                    target_pos = 0;
                    target_found.clear();
                }
            }
        }
        else
        {
            Update.printError(Serial);
            instance->setState(UpdateState::UPDATE_FAILED);
        }
    }

    if (final)
    {
        if (Update.end(true))
        { // true для принудительной записи конечного блока
            Serial.printf("Обновление успешно завершено: %s\n", filename.c_str());
            // Проверка целевой платформы после того, как данные прошивки были получены
            const char *current_target_version = getTargetVersion();

            Serial.printf("Текущая версия прошивки: %s\n", current_target_version);
            Serial.printf("Целевая версия прошивки: %s\n", version_str.c_str());

            if (version_str == current_target_version || FORCE_TARGET_ON_UPDATE)
            {
                instance->setState(UpdateState::UPDATE_SUCCESS);
            }
            else
            {
                Serial.println("Целевая платформа не совпадает. Обновление отменено.");
                Update.abort();
                instance->setState(UpdateState::UPDATE_FAILED);
            }
        }
        else
        {
            Update.printError(Serial);
            instance->setState(UpdateState::UPDATE_FAILED);
        }
    }
}

// Обработчики событий
void FirmwareUpdateMode::userConnected()
{
    Serial.println("Client connected");
    setState(UpdateState::CLIENT_CONNECTED);
    showWebLinkScreen();
}

void FirmwareUpdateMode::userDisconnected()
{
    Serial.println("Client disconnected");
    setState(UpdateState::WAIT_FOR_CLIENT);
    showWiFiScreen();
}

void FirmwareUpdateMode::fileUploaded()
{
    Serial.println("File uploaded successfully");
    setState(UpdateState::UPDATING);
}

void FirmwareUpdateMode::updateSuccess()
{
    Serial.println("Update success");
    setState(UpdateState::UPDATE_SUCCESS);
}

void FirmwareUpdateMode::updateFailed()
{
    Serial.println("Update failed");
    setState(UpdateState::UPDATE_FAILED);
}

// Метод для проверки состояния кнопок
void FirmwareUpdateMode::checkButtons()
{
    // Чтение состояния кнопок
    bool upButtonPressed = digitalRead(BUTTON_UP_PIN) == LOW;
    bool downButtonPressed = digitalRead(BUTTON_DOWN_PIN) == LOW;

    // Проверка длительности нажатия кнопки вверх
    static unsigned long upButtonPressStart = 0;
    static bool upButtonHandled = false;
    if (upButtonPressed && !upButtonHandled)
    {
        if (upButtonPressStart == 0)
        {
            upButtonPressStart = millis();
        }
        else if (millis() - upButtonPressStart > 10000)
        {
            // Кнопка вверх удерживается более 10 секунд
            handleCalibMode(CALIB_MAX_RSSI);
            upButtonHandled = true;
        }
    }
    else
    {
        upButtonPressStart = 0;
        upButtonHandled = false;
    }

    // Проверка длительности нажатия кнопки вниз
    static unsigned long downButtonPressStart = 0;
    static bool downButtonHandled = false;
    if (downButtonPressed && !downButtonHandled)
    {
        if (downButtonPressStart == 0)
        {
            downButtonPressStart = millis();
        }
        else if (millis() - downButtonPressStart > 10000)
        {
            // Кнопка вниз удерживается более 10 секунд
            handleCalibMode(CALIB_MIN_RSSI);
            downButtonHandled = true;
        }
    }
    else
    {
        downButtonPressStart = 0;
        downButtonHandled = false;
    }
}

// Метод для обработки режима калибровки
void FirmwareUpdateMode::handleCalibMode(CalibMode mode)
{
    RSSICalibrationData calibData = loadCalibrationData();
    calibData.calibMode = mode;
    saveCalibrationData(calibData);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);

    // Создаем спрайт для отображения цифр
    TFT_eSprite digitSprite = TFT_eSprite(&tft);
    digitSprite.createSprite(100, 100);             // Создаем спрайт размером 100x100 пикселей
    digitSprite.setTextColor(TFT_WHITE, TFT_BLACK); // Белый текст на черном фоне

    for (int i = 10; i >= 0; i--)
    {
        digitSprite.fillSprite(TFT_BLACK);                  // Очищаем спрайт перед каждым выводом
        digitSprite.drawCentreString(String(i), 50, 25, 7); // Рисуем цифру в центре спрайта

        digitSprite.pushSprite((TFT_WIDTH - 100) / 2, (TFT_HEIGHT - 100) / 2); // Пушим спрайт на экран

        // Подаем звуковой сигнал
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100); // Длительность сигнала
        digitalWrite(BUZZER_PIN, LOW);
        delay(900); // Длительность паузы
    }
    handleReboot();
}

// Метод для перезагрузки устройства
void FirmwareUpdateMode::handleReboot()
{
    const int glitchIterations = 20;
    const int glitchLines = 10;

    for (int i = 0; i < glitchIterations; i++)
    {
        tft.fillScreen(TFT_BLACK);
        for (int j = 0; j < glitchLines; j++)
        {
            int x = random(0, TFT_WIDTH);
            int y = random(0, TFT_HEIGHT);
            int w = random(10, TFT_WIDTH / 4);
            int h = random(1, 10);
            int color = random(0xffff);

            tft.fillRect(x, y, w, h, color);
        }
        delay(50);
    }

    // Немного подождем перед перезагрузкой, чтобы эффект завершился
    delay(500);
    ESP.restart(); // Перезагружаем устройство
}