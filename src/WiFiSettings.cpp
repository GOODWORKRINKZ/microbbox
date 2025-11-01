#include "WiFiSettings.h"
#include <WiFi.h>

WiFiSettings::WiFiSettings() : 
    ssid(""),
    password(""),
    deviceName(""),
    mode(WiFiMode::CLIENT)
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
    
    // SSID и пароль пустые - пользователь должен настроить
    ssid = "";
    password = "";
}

void WiFiSettings::loadFromMemory() {
    // Проверяем, были ли сохранены настройки ранее
    bool hasSettings = preferences.getBool("initialized", false);
    
    if (!hasSettings) {
        // Первый запуск - загружаем значения по умолчанию
        loadDefaults();
        // Сохраняем их в память
        save();
    } else {
        // Загружаем сохраненные настройки
        ssid = preferences.getString("ssid", "");
        password = preferences.getString("password", "");
        deviceName = preferences.getString("deviceName", generateDeviceName());
        mode = static_cast<WiFiMode>(preferences.getUChar("mode", static_cast<uint8_t>(WiFiMode::CLIENT)));
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

bool WiFiSettings::save() {
    // Сохраняем все настройки
    preferences.putBool("initialized", true);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("deviceName", deviceName);
    preferences.putUChar("mode", static_cast<uint8_t>(mode));
    
    return true;
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
