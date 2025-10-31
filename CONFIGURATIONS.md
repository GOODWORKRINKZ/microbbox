# Примеры конфигураций МикроББокс

## 🎛️ Быстрая конфигурация

Откройте `include/config.h` и закомментируйте/раскомментируйте нужные `#define`:

```cpp
// ═══════════════════════════════════════════════════════════════
// КОНФИГУРАЦИЯ ФИЧ - Включение/выключение компонентов
// ═══════════════════════════════════════════════════════════════

#define FEATURE_NEOPIXEL        // Адресные светодиоды WS2812B
#define FEATURE_BUZZER          // Звуковой бузер
#define FEATURE_MOTORS          // Моторы с MX1508
#define FEATURE_CAMERA          // Камера ESP32CAM
#define FEATURE_WIFI            // WiFi и веб-сервер
#define FEATURE_OTA_UPDATE      // OTA обновления прошивки
```

---

## 📦 Готовые конфигурации

### 1. 🚗 Минимальная (только моторы)

**Использование**: Тестирование моторов без дополнительных компонентов

```cpp
//#define FEATURE_NEOPIXEL        // ❌ Отключено
//#define FEATURE_BUZZER          // ❌ Отключено
#define FEATURE_MOTORS          // ✅ Включено
//#define FEATURE_CAMERA          // ❌ Отключено
#define FEATURE_WIFI            // ✅ Включено (для управления)
//#define FEATURE_OTA_UPDATE      // ❌ Отключено
```

**Свободные пины**: GPIO2, GPIO16  
**Память Flash**: ~50% экономии  
**Память RAM**: ~30% экономии

---

### 2. 📹 Камера + управление (без эффектов)

**Использование**: Видеострим с простым управлением

```cpp
//#define FEATURE_NEOPIXEL        // ❌ Отключено
//#define FEATURE_BUZZER          // ❌ Отключено
#define FEATURE_MOTORS          // ✅ Включено
#define FEATURE_CAMERA          // ✅ Включено
#define FEATURE_WIFI            // ✅ Включено
#define FEATURE_OTA_UPDATE      // ✅ Включено
```

**Свободные пины**: GPIO2, GPIO16  
**Память Flash**: ~40% экономии  
**Память RAM**: ~20% экономии

---

### 3. 🎉 Полная конфигурация (всё включено)

**Использование**: Максимум возможностей

```cpp
#define FEATURE_NEOPIXEL        // ✅ Включено
#define FEATURE_BUZZER          // ✅ Включено
#define FEATURE_MOTORS          // ✅ Включено
#define FEATURE_CAMERA          // ✅ Включено
#define FEATURE_WIFI            // ✅ Включено
#define FEATURE_OTA_UPDATE      // ✅ Включено
```

**Свободные пины**: Нет  
**Память Flash**: 100%  
**Память RAM**: 100%

---

### 4. 🔊 Звук + свет (без моторов)

**Использование**: Световые и звуковые эффекты

```cpp
#define FEATURE_NEOPIXEL        // ✅ Включено
#define FEATURE_BUZZER          // ✅ Включено
//#define FEATURE_MOTORS          // ❌ Отключено
//#define FEATURE_CAMERA          // ❌ Отключено
#define FEATURE_WIFI            // ✅ Включено
//#define FEATURE_OTA_UPDATE      // ❌ Отключено
```

**Свободные пины**: GPIO12-15 (моторы)  
**Память Flash**: ~60% экономии  
**Память RAM**: ~40% экономии

---

### 5. 🎥 Камера + эффекты (без моторов)

**Использование**: Видеострим с визуальными эффектами

```cpp
#define FEATURE_NEOPIXEL        // ✅ Включено
#define FEATURE_BUZZER          // ✅ Включено
//#define FEATURE_MOTORS          // ❌ Отключено
#define FEATURE_CAMERA          // ✅ Включено
#define FEATURE_WIFI            // ✅ Включено
#define FEATURE_OTA_UPDATE      // ✅ Включено
```

**Свободные пины**: GPIO12-15 (моторы)  
**Память Flash**: ~30% экономии  
**Память RAM**: ~15% экономии

---

## ⚙️ Как применить конфигурацию

1. Откройте `include/config.h`
2. Найдите секцию "КОНФИГУРАЦИЯ ФИЧ"
3. Закомментируйте (`//`) ненужные `#define`
4. Сохраните файл
5. Пересоберите проект: `pio run --environment release`

## 📊 Таблица зависимостей

| Фича | Требует | Зависит от | Свободные пины при отключении |
|------|---------|------------|-------------------------------|
| NEOPIXEL | GPIO2 | - | GPIO2 |
| BUZZER | GPIO16 | - | GPIO16 |
| MOTORS | GPIO12-15 | - | GPIO12-15 |
| CAMERA | GPIO0,5,18-27,32,34-36,39 | - | Много |
| WIFI | - | - | - |
| OTA_UPDATE | WIFI | WIFI | - |

## 💡 Советы по оптимизации

### Для экономии Flash памяти:
- Отключите `FEATURE_CAMERA` (-500KB)
- Отключите `FEATURE_OTA_UPDATE` (-100KB)
- Отключите `FEATURE_NEOPIXEL` (-50KB)

### Для экономии RAM:
- Отключите `FEATURE_CAMERA` (экономия ~80KB)
- Отключите `FEATURE_WIFI` (экономия ~40KB)

### Для минимального энергопотребления:
- Отключите `FEATURE_NEOPIXEL` (до 180mA)
- Отключите `FEATURE_BUZZER` (до 30mA)
- Отключите `FEATURE_CAMERA` (до 100mA)

## 🔍 Проверка конфигурации

После сборки проверьте использование памяти:

```bash
pio run --environment release --target size
```

Вывод покажет:
- **Flash used**: Занятая память программы
- **RAM used**: Занятая оперативная память

## ⚠️ Важные примечания

1. **FEATURE_WIFI** нужен для:
   - Веб-интерфейса управления
   - OTA обновлений
   - Видеостриминга

2. **FEATURE_MOTORS** требует pull-down резисторы:
   - См. `WIRING.md` для схемы подключения
   - Без резисторов моторы могут крутиться при Reset!

3. **FEATURE_CAMERA** занимает много ресурсов:
   - ~500KB Flash
   - ~80KB RAM
   - Отключайте если не нужна

4. **Минимальная конфигурация**: Только `FEATURE_MOTORS` + `FEATURE_WIFI`

---

**Дата обновления**: 2025-10-31  
**Версия**: 1.0
