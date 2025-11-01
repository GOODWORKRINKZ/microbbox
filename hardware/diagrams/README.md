# Diagrams - Схемы и диаграммы

## 📊 Графические материалы

> **✅ Статус**: Схемы и фото доступны!

Эта папка содержит графические схемы, диаграммы подключения и визуальную документацию проекта МикРоББокс.

## 📋 Доступные материалы

### Электрические схемы (SVG)

**✅ Доступные схемы:**

1. **connection_schematic.svg** (~10KB)
   - Полная схема подключения всех компонентов
   - ESP32-CAM с распиновкой
   - MX1508 драйвер моторов с pull-down резисторами
   - NeoPixel LED цепочка (3 светодиода)
   - Бузер на GPIO 16
   - Все соединения с подписями
   
2. **power_distribution.svg** (~8KB)
   - Схема распределения питания
   - От аккумулятора 9V (крона) до всех компонентов
   - Buck converter Mini-360 для 5V
   - Конденсаторы развязки
   - Расчет токов и нагрузки

### Архитектурные диаграммы (Mermaid)

3. **system_architecture.md** (~9KB)
   - Mermaid диаграммы, рендерятся на GitHub
   - Блок-схема компонентов и связей
   - Последовательность обмена данными
   - Состояния системы
   - Схема питания с детализацией
   - Варианты конфигураций

### Fritzing схемы

Папка **`fritzing/`**:

4. **microbbox.fzz** (~116KB)
   - Основной файл проекта Fritzing
   - Схема подключения в Breadboard виде
   - Включает все компоненты с кастомными деталями
   - Можно открыть в Fritzing для редактирования

5. **microbbox.fzb** (~471 bytes)
   - Бинарный файл Fritzing
   - Дополнительные метаданные

**Компоненты в схеме:**
- ESP32-CAM (AI Thinker)
- MX1508 DC Motor Driver
- Mini-360 Buck Converter
- 2x DC Mini Metal Gear Motors (N-20)
- 9V Battery (Li-Ion 1100mAh)
- WS2812B LED strip
- Соединительные провода

### Фото сборки

Папка **`assembly/`**:

6. **photo_1.jpg** - **photo_6.jpg** (7 фотографий)
   - Этапы сборки робота с реальными компонентами
   - Установка моторов, электроники, проводки
   - Финальная сборка и результат
   
7. **wheels.jpg**
   - Варианты колес для робота
   - Сравнение различных типов колес

## 🎨 Работа со схемами

### Просмотр схем

**SVG схемы** (`connection_schematic.svg`, `power_distribution.svg`):
- Открываются в любом современном браузере
- Можно просмотреть прямо на GitHub
- Масштабируются без потери качества

**Mermaid диаграммы** (`system_architecture.md`):
- Автоматически рендерятся на GitHub
- Исходный код в Markdown формате
- Легко редактировать текстом

**Fritzing файлы**:
- Требуется установка [Fritzing](https://fritzing.org/)
- Откройте `microbbox.fzz` для просмотра и редактирования
- Можно экспортировать в PNG, PDF, SVG

### Редактирование схем

#### Fritzing (.fzz)
```bash
# Установка Fritzing
# Windows/Mac: скачайте с https://fritzing.org/
# Linux:
sudo apt install fritzing
```

Откройте `fritzing/microbbox.fzz` в Fritzing для:
- Просмотра схемы подключения (Breadboard view)
- Изменения компонентов
- Экспорта в различные форматы

#### SVG схемы
Можно редактировать в:
- **Inkscape** (бесплатно, open source)
- **Adobe Illustrator**
- Любой векторный редактор

#### Генерация схем
Используйте Python скрипты для автоматической генерации:
- `generate_svg_schematics.py` - Создает SVG схемы
- `generate_schematics.py` - Альтернативный генератор (schemdraw)
- **KiCad** - Профессиональная разработка PCB
- **EasyEDA** - Онлайн редактор схем
- **Circuit Diagram** - Простой редактор схем

#### Для блок-схем
- **Draw.io (diagrams.net)** - Бесплатный онлайн редактор
- **Lucidchart** - Профессиональные диаграммы
- **Microsoft Visio** - Если есть доступ
- **Inkscape** - Open source векторная графика

#### Для фотографий
- **Smartphone camera** - Хорошее освещение
- **GIMP** - Редактирование и аннотации
- **Photoshop** - Профессиональная обработка

### Требования к файлам

#### Схемы и диаграммы
```
Формат: PNG или SVG
Разрешение: минимум 1920x1080px
DPI: 150-300 для печати
Формат имени: lowercase_with_underscores.png
```

#### Фотографии
```
Формат: JPG или PNG
Разрешение: минимум 2000x1500px
Освещение: Равномерное, без теней
Фон: Однотонный, предпочтительно белый
```

## 📐 Шаблон схемы Fritzing

### Компоненты для добавления

```
Библиотека Fritzing:
- ESP32-CAM (можно найти в интернете или создать)
- MX1508 Motor Driver (custom part)
- DC Motors (Generic)
- WS2812B LED (Adafruit NeoPixel)
- Active Buzzer
- Resistor 10kΩ (x4)
- Capacitor 100µF (x1)
- Capacitor 220µF (x1)
- Capacitor 100nF (x3)
- LiPo Battery
- Buck Converter
- Диод Шоттки
- Wires (различные цвета)
```

### Цветовая схема проводов

| Цвет | Назначение |
|------|------------|
| Красный | VCC/+5V/+3.7V |
| Черный | GND |
| Желтый | Сигналы управления (GPIO) |
| Зеленый | Данные (I2C, SPI) |
| Синий | PWM сигналы |
| Оранжевый | Аналоговые сигналы |
| Белый | Другие соединения |

## 📸 Руководство по фотосъемке

### Подготовка

1. **Очистите робота** - уберите пыль и отпечатки
2. **Подготовьте фон** - белый лист бумаги A3 или больше
3. **Настройте освещение** - два источника света под 45° с каждой стороны
4. **Зарядите камеру** - убедитесь в достаточном заряде

### Съемка

Необходимые ракурсы:
1. **Top view** - Вид сверху, камера параллельно столу
2. **Bottom view** - Вид снизу, показать подключения
3. **Front view** - Вид спереди, показать камеру
4. **Side view** - Вид сбоку, показать профиль
5. **Rear view** - Вид сзади, показать LED
6. **Details** - Крупные планы интересных узлов

### Обработка

1. **Кадрирование** - Убрать лишнее вокруг
2. **Коррекция цвета** - Баланс белого
3. **Резкость** - Немного увеличить
4. **Аннотации** - Добавить стрелки и подписи при необходимости
5. **Сжатие** - JPG качество 85-90%

## 🖼️ Примеры аннотаций

### Пример схемы с подписями

```
         ESP32-CAM
            │
            ├─ GPIO2 ──> LED Chain
            ├─ GPIO12 ─┬─> MX1508 IN1
            │          └─ 10kΩ to GND
            ├─ GPIO13 ─┬─> MX1508 IN2
            │          └─ 10kΩ to GND
            ⋮
```

### Пример фото с подписями

```
        ┌─ ESP32-CAM
        │  (основной контроллер)
        │
        ├─ MX1508 
        │  (драйвер моторов)
        │
        └─ NeoPixel x3
           (светодиоды)
```

## 📦 Структура файлов

```
diagrams/
├── README.md (этот файл)
│
├── electrical/
│   ├── connection_schematic.png
│   ├── wiring_breadboard.png
│   ├── pcb_layout_top.png
│   └── pcb_layout_bottom.png
│
├── assembly/
│   ├── step1_motors.png
│   ├── step2_electronics.png
│   ├── step3_wiring.png
│   ├── step4_battery.png
│   └── step5_final.png
│
├── system/
│   ├── architecture.png
│   ├── power_distribution.png
│   └── data_flow.png
│
└── photos/
    ├── robot_top.jpg
    ├── robot_bottom.jpg
    ├── robot_side.jpg
    └── components.jpg
```

## 🎯 Приоритетные диаграммы

Наиболее важные для создания:

1. **connection_schematic.png** (приоритет 1)
   - Критично для сборки
   - Показывает все соединения
   
2. **wiring_breadboard.png** (приоритет 1)
   - Для начинающих
   - Визуально понятнее схемы
   
3. **Фото собранного робота** (приоритет 2)
   - Показывает результат
   - Мотивирует собрать
   
4. **assembly_steps** (приоритет 2)
   - Пошаговая сборка
   - Снижает ошибки

5. **power_distribution.png** (приоритет 3)
   - Для понимания питания
   - Важно для отладки

## 📤 Как добавить диаграмму

1. **Создайте диаграмму** в соответствии с требованиями
2. **Экспортируйте в PNG/JPG** с правильным разрешением
3. **Добавьте описание** - что показано на диаграмме
4. **Создайте Pull Request** с файлами
5. **Добавьте в README** ссылку на новую диаграмму

### Naming convention
```
<category>_<description>_v<version>.<ext>

Примеры:
- electrical_connection_schematic_v1.0.png
- assembly_step1_motors_v1.0.png
- photo_robot_top_v1.0.jpg
```

## 🔗 Ресурсы для создания схем

### Fritzing Libraries
- [Official Fritzing Parts](https://github.com/fritzing/fritzing-parts)
- [AdaFruit Fritzing Library](https://github.com/adafruit/Fritzing-Library)
- [SparkFun Fritzing Parts](https://github.com/sparkfun/Fritzing_Parts)

### KiCad Libraries
- [Official KiCad Libraries](https://www.kicad.org/libraries/)
- [SnapEDA](https://www.snapeda.com/) - Символы и footprints

### Icons and Symbols
- [Flaticon](https://www.flaticon.com/) - Иконки для диаграмм
- [Font Awesome](https://fontawesome.com/) - Символы

## 🤝 Благодарности

Спасибо всем, кто помогает с созданием визуальной документации проекта!

## ⚠️ Лицензия изображений

- **Схемы и диаграммы**: CERN Open Hardware Licence v2
- **Фотографии**: Creative Commons BY-SA 4.0
- **Аннотированные изображения**: Та же лицензия, что и оригинал

При использовании сторонних материалов - указывайте источник и соблюдайте их лицензию.

---

**Дата обновления**: 2025-11-01  
**Версия**: 1.0  
**Статус**: ✅ Схемы и фото доступны!

## 🔗 Связанные документы

- [ASSEMBLY_GUIDE.md](../ASSEMBLY_GUIDE.md) - Полное руководство по сборке с использованием этих схем
- [WIRING_DIAGRAM.md](../WIRING_DIAGRAM.md) - Детальная схема подключения в текстовом формате
- [BOM.md](../BOM.md) - Список компонентов
- [3d-models/README.md](../3d-models/README.md) - STL файлы для 3D печати
