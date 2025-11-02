#ifndef WIFI_SETTINGS_H
#define WIFI_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

// WiFi режимы
enum class WiFiMode {
    CLIENT = 0,  // Подключение к существующей сети (по умолчанию)
    AP = 1       // Режим точки доступа
};

/**
 * @brief Класс для управления настройками WiFi в энергонезависимой памяти
 */
class WiFiSettings {
public:
    WiFiSettings();
    ~WiFiSettings();

    // Инициализация
    bool init();

    // Получение настроек
    String getSSID() const { return ssid; }
    String getPassword() const { return password; }
    String getDeviceName() const { return deviceName; }
    WiFiMode getMode() const { return mode; }
    
    // Получение настроек моторов
    bool getMotorSwapLeftRight() const { return motorSwapLeftRight; }
    bool getMotorInvertLeft() const { return motorInvertLeft; }
    bool getMotorInvertRight() const { return motorInvertRight; }

    // Установка настроек
    void setSSID(const String& value);
    void setPassword(const String& value);
    void setDeviceName(const String& value);
    void setMode(WiFiMode value);
    
    // Установка настроек моторов
    void setMotorSwapLeftRight(bool value);
    void setMotorInvertLeft(bool value);
    void setMotorInvertRight(bool value);

    // Сохранение в память
    bool save();

    // Сброс к значениям по умолчанию
    void reset();

    // Генерация имени устройства из MAC адреса
    static String generateDeviceName();

private:
    Preferences preferences;
    
    String ssid;
    String password;
    String deviceName;
    WiFiMode mode;
    
    // Настройки моторов
    bool motorSwapLeftRight;    // Поменять местами левый и правый моторы
    bool motorInvertLeft;       // Инвертировать направление левого мотора
    bool motorInvertRight;      // Инвертировать направление правого мотора

    void loadDefaults();
    void loadFromMemory();
};

#endif // WIFI_SETTINGS_H
