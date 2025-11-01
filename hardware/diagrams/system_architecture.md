# МикРоББокс - Архитектура системы

## Блок-схема компонентов

```mermaid
graph TB
    subgraph Power["Система питания"]
        BAT[LiPo 1S 3.7V<br/>500-1000mAh]
        SW[Выключатель]
        DIODE[Диод Шоттки<br/>1N5819]
        BUCK[Buck Converter<br/>3.7V → 5V]
        CAP1[100µF<br/>Развязка ESP32]
        CAP2[220µF<br/>Развязка моторов]
        
        BAT --> SW
        SW --> DIODE
        DIODE --> CAP2
        DIODE --> BUCK
        BUCK --> CAP1
    end
    
    subgraph ESP32["ESP32-CAM (Основной контроллер)"]
        WIFI[WiFi модуль<br/>AP/Client режим]
        CAM[Камера OV2640<br/>Видеострим]
        GPIO[GPIO пины<br/>2,12-16]
        PWM[PWM контроллер<br/>8 каналов]
    end
    
    subgraph Motors["Система движения"]
        MX[MX1508<br/>Драйвер моторов]
        ML[Левый мотор<br/>3-7V DC]
        MR[Правый мотор<br/>3-7V DC]
        PD1[Pull-down 10kΩ]
        PD2[Pull-down 10kΩ]
        PD3[Pull-down 10kΩ]
        PD4[Pull-down 10kΩ]
        
        PD1 -.-> GND1[GND]
        PD2 -.-> GND1
        PD3 -.-> GND1
        PD4 -.-> GND1
    end
    
    subgraph Effects["Эффекты"]
        LED1[NeoPixel LED #1<br/>Задний левый]
        LED2[NeoPixel LED #2<br/>Задний правый]
        LED3[NeoPixel LED #3<br/>Передний фонарик]
        BUZZ[Активный бузер<br/>3.3V]
    end
    
    subgraph User["Интерфейс пользователя"]
        WEB[Веб-интерфейс<br/>HTML/JS/CSS]
        VR[VR контроллеры<br/>Oculus Quest]
        GAMEPAD[Геймпад]
        KB[Клавиатура]
    end
    
    %% Питание
    CAP1 -->|5V| ESP32
    CAP2 -->|3.7V| MX
    
    %% Управление моторами
    GPIO -->|GPIO12 + PD| PD1
    GPIO -->|GPIO13 + PD| PD2
    GPIO -->|GPIO14 + PD| PD3
    GPIO -->|GPIO15 + PD| PD4
    PD1 -->|IN1| MX
    PD2 -->|IN2| MX
    PD3 -->|IN4| MX
    PD4 -->|IN3| MX
    MX -->|OUT1/OUT2| ML
    MX -->|OUT3/OUT4| MR
    
    %% Светодиоды
    GPIO -->|GPIO2 Data| LED1
    LED1 -->|DOUT→DIN| LED2
    LED2 -->|DOUT→DIN| LED3
    
    %% Бузер
    GPIO -->|GPIO16 PWM| BUZZ
    
    %% WiFi связь
    WIFI -.->|HTTP Stream| WEB
    CAM -->|Video| WIFI
    
    %% Пользовательский ввод
    WEB -->|Commands| WIFI
    VR -.->|WebXR| WEB
    GAMEPAD -.->|Gamepad API| WEB
    KB -.->|Keyboard Events| WEB
    
    %% PWM управление
    PWM --> GPIO
    
    style Power fill:#ffe6e6
    style ESP32 fill:#e6f3ff
    style Motors fill:#e6ffe6
    style Effects fill:#fff0e6
    style User fill:#f0e6ff
```

## Поток данных управления

```mermaid
sequenceDiagram
    participant User as Пользователь
    participant Web as Веб-интерфейс
    participant ESP as ESP32-CAM
    participant Motor as MX1508
    participant LED as NeoPixel
    participant Cam as Камера
    
    Note over User,Cam: Инициализация
    ESP->>Web: Запуск веб-сервера
    ESP->>Cam: Инициализация камеры
    ESP->>LED: Инициализация светодиодов
    ESP->>Motor: Настройка PWM каналов
    
    Note over User,Cam: Видеострим
    loop Каждый кадр
        Cam->>ESP: Захват изображения
        ESP->>Web: Отправка JPEG
        Web->>User: Отображение видео
    end
    
    Note over User,Cam: Управление движением
    User->>Web: Нажатие стрелки вперед
    Web->>ESP: POST /control {forward: true}
    ESP->>Motor: PWM на GPIO12,15 (оба мотора вперед)
    Motor->>Motor: Вращение моторов
    
    Note over User,Cam: Световые эффекты
    User->>Web: Включение режима "Полиция"
    Web->>ESP: POST /effect {mode: "police"}
    ESP->>LED: Анимация красный/синий
    loop Мигание
        LED->>LED: LED1=Красный, LED2=Синий
        LED->>LED: LED1=Синий, LED2=Красный
    end
    
    Note over User,Cam: Звуковой сигнал
    User->>Web: Нажатие пробела
    Web->>ESP: POST /horn
    ESP->>Motor: Включение бузера PWM
```

## Состояния системы

```mermaid
stateDiagram-v2
    [*] --> PowerOn: Подача питания
    
    PowerOn --> CheckWiFi: Инициализация
    
    CheckWiFi --> ClientMode: Настройки WiFi Client найдены
    CheckWiFi --> APMode: Нет настроек или ошибка подключения
    
    ClientMode --> Ready: Подключено к сети
    APMode --> Ready: Создана точка доступа
    
    Ready --> Idle: Ожидание команд
    
    Idle --> Moving: Получена команда движения
    Idle --> Effects: Смена режима эффектов
    Idle --> Streaming: Запрос видео
    Idle --> Settings: Открыты настройки
    
    Moving --> Idle: Команда остановки
    Effects --> Idle: Завершение эффекта
    Streaming --> Idle: Остановка стриминга
    Settings --> Idle: Настройки сохранены
    
    Settings --> Reboot: Сохранение WiFi настроек
    Reboot --> PowerOn: Перезагрузка
    
    Ready --> OTA: Запрос обновления
    OTA --> Reboot: Прошивка загружена
    OTA --> Ready: Ошибка обновления
```

## Схема питания - детализация

```mermaid
graph LR
    subgraph Battery["Аккумулятор"]
        BAT[LiPo 1S<br/>3.7V nominal<br/>4.2V max<br/>3.0V min]
    end
    
    subgraph Protection["Защита"]
        SW[Выключатель<br/>On/Off]
        DIODE[Диод 1N5819<br/>Обратная защита]
    end
    
    subgraph Motor_Power["Питание моторов"]
        CAP2[220µF<br/>Фильтр помех]
        MX[MX1508<br/>VM pin]
        MOT[Моторы 2x<br/>Пиковый ток 2A]
    end
    
    subgraph Logic_Power["Логика 5V"]
        BUCK[Buck Converter<br/>Step-down 5V<br/>Выход до 3A]
        CAP1[100µF<br/>Развязка]
    end
    
    subgraph ESP_System["ESP32-CAM"]
        ESP[ESP32-CAM<br/>5V вход<br/>~300mA]
        LED_STRIP[NeoPixel 3x<br/>5V<br/>до 180mA]
    end
    
    BAT -->|3.7V| SW
    SW -->|3.7V| DIODE
    DIODE -->|3.7V| CAP2
    DIODE -->|3.7V| BUCK
    
    CAP2 -->|Filtered 3.7V| MX
    MX -.->|Drive| MOT
    
    BUCK -->|5V| CAP1
    CAP1 -->|5V| ESP
    CAP1 -->|5V| LED_STRIP
    
    style Battery fill:#ffe6e6
    style Protection fill:#fff4e6
    style Motor_Power fill:#e6ffe6
    style Logic_Power fill:#e6f3ff
    style ESP_System fill:#f0e6ff
```

## Конфигурации робота

```mermaid
graph TB
    subgraph Full["Полная конфигурация"]
        F1[ESP32-CAM + Камера]
        F2[Моторы + MX1508]
        F3[NeoPixel 3x]
        F4[Бузер]
        F5[WiFi + OTA]
        F6[Все эффекты]
    end
    
    subgraph Simplified["Упрощенная"]
        S1[ESP32-CAM + Камера]
        S2[Моторы + MX1508]
        S3[WiFi + OTA]
        S4[Без LED]
        S5[Без бузера]
    end
    
    subgraph Minimal["Минимальная"]
        M1[ESP32 без камеры]
        M2[Моторы + MX1508]
        M3[WiFi базовый]
    end
    
    Full -.->|Убрать эффекты| Simplified
    Simplified -.->|Убрать камеру| Minimal
    
    style Full fill:#90EE90
    style Simplified fill:#FFD700
    style Minimal fill:#FFA07A
```

---

**Дата создания**: 2025-11-01  
**Версия**: 1.0  
**Формат**: Mermaid диаграммы (рендерятся на GitHub автоматически)

## Как использовать эти диаграммы

1. **На GitHub**: Диаграммы автоматически рендерятся при просмотре этого файла
2. **Локально**: Используйте расширения для VSCode/IDE с поддержкой Mermaid
3. **Экспорт**: Скопируйте код в [Mermaid Live Editor](https://mermaid.live) для экспорта в PNG/SVG

## Редактирование

Для редактирования диаграмм:
1. Отредактируйте код внутри блоков ```mermaid
2. Проверьте синтаксис на https://mermaid.live
3. GitHub автоматически обновит отображение
