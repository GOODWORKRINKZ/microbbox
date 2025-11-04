#include "WiFiSettings.h"
#include "hardware_config.h"
#include <WiFi.h>

WiFiSettings::WiFiSettings() : 
    ssid(""),
    password(""),
    deviceName(""),
    mode(WiFiMode::CLIENT),
    motorSwapLeftRight(false),
    motorInvertLeft(false),
    motorInvertRight(false),
    invertThrottleStick(false),
    invertSteeringStick(false),
    cameraHMirror(false),
    cameraVFlip(false)
{
}

WiFiSettings::~WiFiSettings() {
    preferences.end();
}

bool WiFiSettings::init() {
    // Открываем namespace "wifi" для хранения настроек
    if (!preferences.begin("wifi", false)) { // false = read-write mode
        return false;
    }

    // Загружаем настройки из памяти или используем значения по умолчанию
    loadFromMemory();
    
    return true;
}

void WiFiSettings::loadDefaults() {
    // Генерируем имя устройства на основе MAC адреса
    deviceName = generateDeviceName();
    
    // По умолчанию режим CLIENT
    mode = WiFiMode::CLIENT;
    
    // Используем дефолтные значения из config.h
    ssid = WIFI_SSID_CLIENT;
    password = WIFI_PASSWORD_CLIENT;
    
    // По умолчанию моторы не перевернуты
    motorSwapLeftRight = false;
    motorInvertLeft = false;
    motorInvertRight = false;
    
    // По умолчанию стики не инвертированы (нормальное управление)
    invertThrottleStick = false;
    invertSteeringStick = false;
    
    // По умолчанию камера без зеркалирования и переворота
    cameraHMirror = false;
    cameraVFlip = false;
    
    DEBUG_PRINTLN("WiFiSettings::loadDefaults()");
    DEBUG_PRINT("  SSID: ");
    DEBUG_PRINTLN(ssid);
    DEBUG_PRINTLN("  Mode: CLIENT");
}

void WiFiSettings::loadFromMemory() {
    // Проверяем, были ли сохранены настройки ранее
    bool hasSettings = preferences.getBool("initialized", false);
    
    DEBUG_PRINT("WiFiSettings::loadFromMemory() - initialized flag: ");
    DEBUG_PRINTLN(hasSettings ? "true" : "false");
    
    if (!hasSettings) {
        // Первый запуск - загружаем значения по умолчанию
        DEBUG_PRINTLN("  Первый запуск - загружаем defaults");
        loadDefaults();
        // Сохраняем их в память
        save();
    } else {
        // Загружаем сохраненные настройки
        ssid = preferences.getString("ssid", "");
        password = preferences.getString("password", "");
        deviceName = preferences.getString("deviceName", generateDeviceName());
        mode = static_cast<WiFiMode>(preferences.getUChar("mode", static_cast<uint8_t>(WiFiMode::CLIENT)));
        
        // Загружаем настройки моторов
        motorSwapLeftRight = preferences.getBool("motorSwap", false);
        motorInvertLeft = preferences.getBool("motorInvL", false);
        motorInvertRight = preferences.getBool("motorInvR", false);
        
        // Загружаем настройки инверсии стиков
        invertThrottleStick = preferences.getBool("invThrottle", false);
        invertSteeringStick = preferences.getBool("invSteering", false);
        
        // Загружаем настройки камеры
        cameraHMirror = preferences.getBool("camHMirror", false);
        cameraVFlip = preferences.getBool("camVFlip", false);
        
        DEBUG_PRINTLN("  Загружены сохраненные настройки:");
        DEBUG_PRINT("    SSID: '"); DEBUG_PRINT(ssid); DEBUG_PRINTLN("'");
        DEBUG_PRINT("    Password length: "); DEBUG_PRINTLN(password.length());
        DEBUG_PRINT("    Device name: '"); DEBUG_PRINT(deviceName); DEBUG_PRINTLN("'");
        DEBUG_PRINT("    Mode: "); DEBUG_PRINTLN(mode == WiFiMode::CLIENT ? "CLIENT" : "AP");
        DEBUG_PRINT("    Motor swap L/R: "); DEBUG_PRINTLN(motorSwapLeftRight ? "YES" : "NO");
        DEBUG_PRINT("    Motor invert L: "); DEBUG_PRINTLN(motorInvertLeft ? "YES" : "NO");
        DEBUG_PRINT("    Motor invert R: "); DEBUG_PRINTLN(motorInvertRight ? "YES" : "NO");
        DEBUG_PRINT("    Invert Throttle: "); DEBUG_PRINTLN(invertThrottleStick ? "YES" : "NO");
        DEBUG_PRINT("    Invert Steering: "); DEBUG_PRINTLN(invertSteeringStick ? "YES" : "NO");
        DEBUG_PRINT("    Camera HMirror: "); DEBUG_PRINTLN(cameraHMirror ? "YES" : "NO");
        DEBUG_PRINT("    Camera VFlip: "); DEBUG_PRINTLN(cameraVFlip ? "YES" : "NO");
        
        // ВАЖНО: Если SSID пустой - это значит старые битые настройки, сбрасываем!
        if (ssid.length() == 0) {
            DEBUG_PRINTLN("  ⚠️ SSID пустой - загружаем дефолтные настройки");
            loadDefaults();
            save();
        }
    }
}

void WiFiSettings::setSSID(const String& value) {
    ssid = value;
}

void WiFiSettings::setPassword(const String& value) {
    password = value;
}

void WiFiSettings::setDeviceName(const String& value) {
    deviceName = value;
}

void WiFiSettings::setMode(WiFiMode value) {
    mode = value;
}

void WiFiSettings::setMotorSwapLeftRight(bool value) {
    motorSwapLeftRight = value;
}

void WiFiSettings::setMotorInvertLeft(bool value) {
    motorInvertLeft = value;
}

void WiFiSettings::setMotorInvertRight(bool value) {
    motorInvertRight = value;
}

void WiFiSettings::setInvertThrottleStick(bool value) {
    invertThrottleStick = value;
}

void WiFiSettings::setInvertSteeringStick(bool value) {
    invertSteeringStick = value;
}

void WiFiSettings::setCameraHMirror(bool value) {
    cameraHMirror = value;
}

void WiFiSettings::setCameraVFlip(bool value) {
    cameraVFlip = value;
}

bool WiFiSettings::save() {
    DEBUG_PRINTLN("WiFiSettings::save() - начало сохранения");
    DEBUG_PRINT("  SSID: '"); DEBUG_PRINT(ssid); DEBUG_PRINTLN("'");
    DEBUG_PRINT("  Password length: "); DEBUG_PRINTLN(password.length());
    DEBUG_PRINT("  Device name: '"); DEBUG_PRINT(deviceName); DEBUG_PRINTLN("'");
    DEBUG_PRINT("  Mode: "); DEBUG_PRINTLN(static_cast<uint8_t>(mode));
    DEBUG_PRINT("  Motor swap L/R: "); DEBUG_PRINTLN(motorSwapLeftRight ? "YES" : "NO");
    DEBUG_PRINT("  Motor invert L: "); DEBUG_PRINTLN(motorInvertLeft ? "YES" : "NO");
    DEBUG_PRINT("  Motor invert R: "); DEBUG_PRINTLN(motorInvertRight ? "YES" : "NO");
    DEBUG_PRINT("  Invert Throttle: "); DEBUG_PRINTLN(invertThrottleStick ? "YES" : "NO");
    DEBUG_PRINT("  Invert Steering: "); DEBUG_PRINTLN(invertSteeringStick ? "YES" : "NO");
    DEBUG_PRINT("  Camera HMirror: "); DEBUG_PRINTLN(cameraHMirror ? "YES" : "NO");
    DEBUG_PRINT("  Camera VFlip: "); DEBUG_PRINTLN(cameraVFlip ? "YES" : "NO");
    
    // Сохраняем все настройки и проверяем результат каждой операции
    size_t w1 = preferences.putBool("initialized", true);
    size_t w2 = preferences.putString("ssid", ssid);
    size_t w3 = preferences.putString("password", password);
    size_t w4 = preferences.putString("deviceName", deviceName);
    size_t w5 = preferences.putUChar("mode", static_cast<uint8_t>(mode));
    
    // Сохраняем настройки моторов
    size_t w6 = preferences.putBool("motorSwap", motorSwapLeftRight);
    size_t w7 = preferences.putBool("motorInvL", motorInvertLeft);
    size_t w8 = preferences.putBool("motorInvR", motorInvertRight);
    
    // Сохраняем настройки инверсии стиков
    size_t w9 = preferences.putBool("invThrottle", invertThrottleStick);
    size_t w10 = preferences.putBool("invSteering", invertSteeringStick);
    
    // Сохраняем настройки камеры
    size_t w11 = preferences.putBool("camHMirror", cameraHMirror);
    size_t w12 = preferences.putBool("camVFlip", cameraVFlip);
    
    DEBUG_PRINT("  Записано байт - initialized: "); DEBUG_PRINTLN(w1);
    DEBUG_PRINT("  Записано байт - ssid: "); DEBUG_PRINTLN(w2);
    DEBUG_PRINT("  Записано байт - password: "); DEBUG_PRINTLN(w3);
    DEBUG_PRINT("  Записано байт - deviceName: "); DEBUG_PRINTLN(w4);
    DEBUG_PRINT("  Записано байт - mode: "); DEBUG_PRINTLN(w5);
    DEBUG_PRINT("  Записано байт - motorSwap: "); DEBUG_PRINTLN(w6);
    DEBUG_PRINT("  Записано байт - motorInvL: "); DEBUG_PRINTLN(w7);
    DEBUG_PRINT("  Записано байт - motorInvR: "); DEBUG_PRINTLN(w8);
    DEBUG_PRINT("  Записано байт - invThrottle: "); DEBUG_PRINTLN(w9);
    DEBUG_PRINT("  Записано байт - invSteering: "); DEBUG_PRINTLN(w10);
    DEBUG_PRINT("  Записано байт - camHMirror: "); DEBUG_PRINTLN(w11);
    DEBUG_PRINT("  Записано байт - camVFlip: "); DEBUG_PRINTLN(w12);
    
    // Принудительно сохраняем изменения (commit)
    bool committed = preferences.putBool("_commit", true);
    
    DEBUG_PRINT("  Commit результат: "); DEBUG_PRINTLN(committed ? "OK" : "FAIL");
    
    // Проверяем, что критичные данные были записаны успешно
    bool success = (w1 > 0) && (w2 > 0) && (w5 > 0);
    
    DEBUG_PRINT("WiFiSettings::save() - результат: "); 
    DEBUG_PRINTLN(success ? "УСПЕХ" : "ОШИБКА");
    
    return success;
}

void WiFiSettings::reset() {
    // Очищаем все сохраненные настройки
    preferences.clear();
    
    // Загружаем значения по умолчанию
    loadDefaults();
}

String WiFiSettings::generateDeviceName() {
    // Получаем MAC адрес
    String mac = WiFi.macAddress();
    
    // Убираем двоеточия
    mac.replace(":", "");
    
    // Берем последние 6 символов (3 байта)
    String macSuffix = mac.substring(6);
    
    // Формируем имя: MICROBBOX-XXXXXX
    return "MICROBBOX-" + macSuffix;
}
