#include "BrainRobot.h"

#ifdef TARGET_BRAIN

#include "hardware_config.h"

BrainRobot::BrainRobot() :
    BaseRobot(),
    currentProtocol_(OutputProtocol::PWM)
{
    DEBUG_PRINTLN("Создание BrainRobot");
    
    // Инициализация каналов нейтральными значениями
    for (int i = 0; i < 8; i++) {
        channelValues_[i] = 1500; // Центр
    }
}

BrainRobot::~BrainRobot() {
    shutdown();
}

bool BrainRobot::initSpecificComponents() {
    DEBUG_PRINTLN("=== Инициализация компонентов Brain робота ===");
    
    // Инициализация протокола по умолчанию (PWM)
#ifdef FEATURE_PWM_OUTPUT
    if (!initPWMOutput()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Не удалось инициализировать PWM выход");
    }
#endif
    
    DEBUG_PRINTLN("=== Brain робот готов ===");
    return true;
}

void BrainRobot::updateSpecificComponents() {
    // Обновление выходов в зависимости от протокола
    updateOutputs();
}

void BrainRobot::shutdownSpecificComponents() {
    // Установка нейтральных значений на выходах
    for (int i = 0; i < 8; i++) {
        channelValues_[i] = 1500;
    }
    updateOutputs();
}

void BrainRobot::setupWebHandlers(AsyncWebServer* server) {
    DEBUG_PRINTLN("Настройка веб-обработчиков для Brain робота");
    
    // Главная страница
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    
    // Команды управления
    server->on("/cmd", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleCommand(request);
    });
    
    // Выбор протокола
    server->on("/protocol", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleProtocol(request);
    });
    
    // API: Тип робота
    server->on("/api/robot-type", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = "{\"type\":\"brain\",\"name\":\"MicroBox Brain\"}";
        request->send(200, "application/json", json);
    });
    
    // 404
    server->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not Found");
    });
}

bool BrainRobot::initPWMOutput() {
#ifdef FEATURE_PWM_OUTPUT
    DEBUG_PRINTLN("Инициализация PWM выхода...");
    
    // Настройка PWM на пинах
    pinMode(PWM_OUT_PIN_1, OUTPUT);
    pinMode(PWM_OUT_PIN_2, OUTPUT);
    pinMode(PWM_OUT_PIN_3, OUTPUT);
    pinMode(PWM_OUT_PIN_4, OUTPUT);
    
    // Настройка LEDC для PWM
    ledcSetup(0, PWM_OUT_FREQ, PWM_OUT_RESOLUTION);
    ledcSetup(1, PWM_OUT_FREQ, PWM_OUT_RESOLUTION);
    ledcSetup(2, PWM_OUT_FREQ, PWM_OUT_RESOLUTION);
    ledcSetup(3, PWM_OUT_FREQ, PWM_OUT_RESOLUTION);
    
    ledcAttachPin(PWM_OUT_PIN_1, 0);
    ledcAttachPin(PWM_OUT_PIN_2, 1);
    ledcAttachPin(PWM_OUT_PIN_3, 2);
    ledcAttachPin(PWM_OUT_PIN_4, 3);
    
    DEBUG_PRINTLN("PWM выход инициализирован");
    return true;
#else
    return false;
#endif
}

bool BrainRobot::initPPMOutput() {
#ifdef FEATURE_PPM_OUTPUT
    DEBUG_PRINTLN("Инициализация PPM выхода...");
    
    pinMode(PPM_OUT_PIN, OUTPUT);
    digitalWrite(PPM_OUT_PIN, LOW);
    
    // TODO: Настроить таймер для генерации PPM сигнала
    
    DEBUG_PRINTLN("PPM выход инициализирован");
    return true;
#else
    return false;
#endif
}

bool BrainRobot::initSBUSOutput() {
#ifdef FEATURE_SBUS_OUTPUT
    DEBUG_PRINTLN("Инициализация SBUS выхода...");
    
    // TODO: Настроить UART для SBUS
    Serial2.begin(SBUS_BAUD, SERIAL_8E2, -1, SBUS_TX_PIN);
    
    DEBUG_PRINTLN("SBUS выход инициализирован");
    return true;
#else
    return false;
#endif
}

bool BrainRobot::initTBSOutput() {
#ifdef FEATURE_TBS_OUTPUT
    DEBUG_PRINTLN("Инициализация TBS Crossfire выхода...");
    
    // TODO: Настроить UART для TBS Crossfire
    Serial2.begin(TBS_BAUD, SERIAL_8N1, -1, TBS_TX_PIN);
    
    DEBUG_PRINTLN("TBS выход инициализирован");
    return true;
#else
    return false;
#endif
}

void BrainRobot::updateOutputs() {
    switch (currentProtocol_) {
        case OutputProtocol::PWM:
            sendPWMOutput(channelValues_[0], channelValues_[1], 
                         channelValues_[2], channelValues_[3]);
            break;
        case OutputProtocol::PPM:
            sendPPMOutput(channelValues_, PPM_CHANNELS);
            break;
        case OutputProtocol::SBUS:
            sendSBUSOutput(channelValues_, 16);
            break;
        case OutputProtocol::TBS:
            sendTBSOutput(channelValues_, 8);
            break;
    }
}

void BrainRobot::sendPWMOutput(int ch1, int ch2, int ch3, int ch4) {
#ifdef FEATURE_PWM_OUTPUT
    // Преобразование микросекунд в duty cycle
    // PWM 50 Гц = 20 мс период
    // 1000-2000 мкс = 5-10% duty cycle для 16-бит PWM
    
    int maxDuty = (1 << PWM_OUT_RESOLUTION) - 1; // 65535 для 16-бит
    
    int duty1 = map(ch1, 1000, 2000, maxDuty * 0.05, maxDuty * 0.10);
    int duty2 = map(ch2, 1000, 2000, maxDuty * 0.05, maxDuty * 0.10);
    int duty3 = map(ch3, 1000, 2000, maxDuty * 0.05, maxDuty * 0.10);
    int duty4 = map(ch4, 1000, 2000, maxDuty * 0.05, maxDuty * 0.10);
    
    ledcWrite(0, duty1);
    ledcWrite(1, duty2);
    ledcWrite(2, duty3);
    ledcWrite(3, duty4);
#endif
}

void BrainRobot::sendPPMOutput(int channels[], int count) {
#ifdef FEATURE_PPM_OUTPUT
    // NOTE: PPM протокол не реализован в версии 2.0
    // Будет добавлен в будущих версиях
    // PPM формат: последовательность импульсов с переменной длительностью
    // Для реализации требуется настроить таймер для генерации PPM сигнала
    DEBUG_PRINTLN("PPM выход: функция доступна в будущих версиях");
#endif
}

void BrainRobot::sendSBUSOutput(int channels[], int count) {
#ifdef FEATURE_SBUS_OUTPUT
    // NOTE: SBUS протокол не реализован в версии 2.0
    // Будет добавлен в будущих версиях
    // SBUS: 25 байт пакет, 11-бит каналы, 100k baud, инвертированный
    // Требуется реализация протокола и настройка UART
    DEBUG_PRINTLN("SBUS выход: функция доступна в будущих версиях");
#endif
}

void BrainRobot::sendTBSOutput(int channels[], int count) {
#ifdef FEATURE_TBS_OUTPUT
    // NOTE: TBS Crossfire протокол не реализован в версии 2.0
    // Будет добавлен в будущих версиях
    // Crossfire: CRSF протокол, 420k baud
    // Требуется реализация CRSF протокола
    DEBUG_PRINTLN("TBS выход: функция доступна в будущих версиях");
#endif
}

void BrainRobot::handleRoot(AsyncWebServerRequest* request) {
    String html = "<html><head><title>MicroBox Brain</title></head><body>";
    html += "<h1>MicroBox Brain</h1>";
    html += "<p>Модуль управления другими роботами</p>";
    html += "<p>Протокол: ";
    switch (currentProtocol_) {
        case OutputProtocol::PWM: html += "PWM"; break;
        case OutputProtocol::PPM: html += "PPM"; break;
        case OutputProtocol::SBUS: html += "SBUS"; break;
        case OutputProtocol::TBS: html += "TBS Crossfire"; break;
    }
    html += "</p>";
    html += "<button onclick=\"fetch('/protocol?type=pwm')\">PWM</button> ";
    html += "<button onclick=\"fetch('/protocol?type=ppm')\">PPM</button> ";
    html += "<button onclick=\"fetch('/protocol?type=sbus')\">SBUS</button> ";
    html += "<button onclick=\"fetch('/protocol?type=tbs')\">TBS</button>";
    html += "</body></html>";
    
    request->send(200, "text/html", html);
}

void BrainRobot::handleCommand(AsyncWebServerRequest* request) {
    // Обработка команд управления
    bool updated = false;
    
    for (int i = 0; i < 8; i++) {
        String paramName = "ch" + String(i + 1);
        if (request->hasParam(paramName)) {
            int value = request->getParam(paramName)->value().toInt();
            channelValues_[i] = constrain(value, 1000, 2000);
            updated = true;
        }
    }
    
    if (updated) {
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

void BrainRobot::handleProtocol(AsyncWebServerRequest* request) {
    if (request->hasParam("type")) {
        String type = request->getParam("type")->value();
        
        if (type == "pwm") {
            currentProtocol_ = OutputProtocol::PWM;
            initPWMOutput();
        } else if (type == "ppm") {
            currentProtocol_ = OutputProtocol::PPM;
            initPPMOutput();
        } else if (type == "sbus") {
            currentProtocol_ = OutputProtocol::SBUS;
            initSBUSOutput();
        } else if (type == "tbs") {
            currentProtocol_ = OutputProtocol::TBS;
            initTBSOutput();
        } else {
            request->send(400, "text/plain", "Unknown protocol");
            return;
        }
        
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

#endif // TARGET_BRAIN
