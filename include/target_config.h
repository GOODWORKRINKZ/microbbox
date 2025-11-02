#ifndef TARGET_CONFIG_H
#define TARGET_CONFIG_H

// ═══════════════════════════════════════════════════════════════
// КОНФИГУРАЦИЯ ТИПА РОБОТА (выбирается через platformio.ini)
// ═══════════════════════════════════════════════════════════════
// 
// Этот файл определяет, какой тип робота собирается.
// НЕ изменяйте этот файл напрямую - используйте build_flags в platformio.ini
//
// Доступные типы:
// - TARGET_CLASSIC: Управляемый робот с камерой (по умолчанию)
// - TARGET_LINER: Автономный робот следующий по линии
// - TARGET_BRAIN: Модуль управления для других роботов

// Проверка, что определен только один тип робота
#if defined(TARGET_CLASSIC) && defined(TARGET_LINER)
    #error "Нельзя определить одновременно TARGET_CLASSIC и TARGET_LINER"
#endif
#if defined(TARGET_CLASSIC) && defined(TARGET_BRAIN)
    #error "Нельзя определить одновременно TARGET_CLASSIC и TARGET_BRAIN"
#endif
#if defined(TARGET_LINER) && defined(TARGET_BRAIN)
    #error "Нельзя определить одновременно TARGET_LINER и TARGET_BRAIN"
#endif

// Если ничего не определено, используем CLASSIC по умолчанию
#if !defined(TARGET_CLASSIC) && !defined(TARGET_LINER) && !defined(TARGET_BRAIN)
    #define TARGET_CLASSIC
    #warning "Тип робота не указан, используется TARGET_CLASSIC по умолчанию"
#endif

// ═══════════════════════════════════════════════════════════════
// АВТОМАТИЧЕСКАЯ КОНФИГУРАЦИЯ КОМПОНЕНТОВ ДЛЯ КАЖДОГО ТИПА
// ═══════════════════════════════════════════════════════════════

#ifdef TARGET_CLASSIC
    // МикроБокс Классик - полнофункциональный управляемый робот
    #define ROBOT_NAME "MicroBox-Classic"
    #define FEATURE_MOTORS              // Моторы для движения
    #define FEATURE_CAMERA              // Камера для видео
    #define FEATURE_WIFI                // WiFi для управления
    #define FEATURE_WEBSERVER           // Веб-интерфейс
    #define FEATURE_OTA_UPDATE          // OTA обновления
    #define FEATURE_NEOPIXEL            // Светодиоды
    #define FEATURE_BUZZER              // Звуковые эффекты
    #define FEATURE_REMOTE_CONTROL      // Управление с телефона/компьютера
#endif

#ifdef TARGET_LINER
    // МикроБокс Лайнер - автономный робот следующий по линии
    #define ROBOT_NAME "MicroBox-Liner"
    #define FEATURE_MOTORS              // Моторы для движения
    #define FEATURE_CAMERA              // Камера для линии (96x96 BW)
    #define FEATURE_WIFI                // WiFi для конфигурации и мониторинга
    #define FEATURE_WEBSERVER           // Веб-интерфейс для настройки
    #define FEATURE_OTA_UPDATE          // OTA обновления
    #define FEATURE_NEOPIXEL            // Светодиоды для индикации
    #define FEATURE_BUTTON              // Кнопка запуска автономного режима
    #define FEATURE_LINE_FOLLOWING      // Алгоритм следования по линии
    #define FEATURE_REMOTE_CONTROL      // Опциональное ручное управление
#endif

#ifdef TARGET_BRAIN
    // МикроБокс Брейн - модуль управления для других роботов
    #define ROBOT_NAME "MicroBox-Brain"
    // НЕТ встроенных моторов - подключение к внешнему роботу
    #define FEATURE_CAMERA              // Камера для передачи видео
    #define FEATURE_WIFI                // WiFi для управления
    #define FEATURE_WEBSERVER           // Веб-интерфейс
    #define FEATURE_OTA_UPDATE          // OTA обновления
    #define FEATURE_PROTOCOL_TRANSLATOR // Трансляция команд в различные протоколы
    #define FEATURE_PWM_OUTPUT          // PWM выход
    #define FEATURE_PPM_OUTPUT          // PPM выход
    #define FEATURE_SBUS_OUTPUT         // SBUS выход
    #define FEATURE_TBS_OUTPUT          // TBS Crossfire выход
    #define FEATURE_REMOTE_CONTROL      // Управление с телефона/компьютера
#endif

// ═══════════════════════════════════════════════════════════════
// ОБЩИЕ НАСТРОЙКИ
// ═══════════════════════════════════════════════════════════════

// Версия проекта
#ifndef GIT_VERSION
#define GIT_VERSION "v2.0.0-dev"
#endif

#ifndef PROJECT_NAME
#define PROJECT_NAME "MicRoBBox"
#endif

// GitHub repository для проверки обновлений
#ifndef GITHUB_REPO_URL
#define GITHUB_REPO_URL "GOODWORKRINKZ/microbbox"
#endif

// Debug настройки
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)  
  #define DEBUG_PRINTF(x, ...)
#endif

#endif // TARGET_CONFIG_H
