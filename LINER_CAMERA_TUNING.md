# Настройка камеры для следования по линии

## Проблема: засветы и плохой контраст

ESP32-CAM поддерживает множество параметров для компенсации засветов и улучшения качества изображения.

## Применённые оптимизации

### 1. **Автоматическая экспозиция (AEC)**
Компенсирует яркость освещения:
```cpp
#define LINE_CAM_AEC_ENABLE 1           // Включить автоэкспозицию
#define LINE_CAM_AEC_VALUE 300          // 0-1200 (300 = средне-темное)
#define LINE_CAM_AE_LEVEL 0             // -2 to +2 компенсация
```

**Если есть засветы:**
- Уменьшите `LINE_CAM_AEC_VALUE` (попробуйте 200-250)
- Или установите `LINE_CAM_AE_LEVEL -1` (затемнение)

**Если слишком темно:**
- Увеличьте `LINE_CAM_AEC_VALUE` (попробуйте 400-500)
- Или установите `LINE_CAM_AE_LEVEL +1` (осветление)

### 2. **White Pixel Correction (WPC)**
Убирает яркие артефакты и засветы:
```cpp
#define LINE_CAM_WPC_ENABLE 1           // ВАЖНО! Убирает белые пиксели-артефакты
```

### 3. **Gamma коррекция**
Улучшает контраст между чёрным и белым:
```cpp
#define LINE_CAM_RAW_GMA_ENABLE 1       // Gamma коррекция
```

### 4. **Lens Correction**
Убирает виньетирование (затемнение по краям):
```cpp
#define LINE_CAM_LENC_ENABLE 1          // Коррекция объектива
```

### 5. **Автоматическое усиление (AGC)**
Управляет чувствительностью:
```cpp
#define LINE_CAM_AGC_ENABLE 1           // Автоусиление
#define LINE_CAM_GAIN_CEILING 0         // Потолок: 0=2x, 6=128x
```

**Для засвеченных сцен:**
- Установите `LINE_CAM_GAIN_CEILING 0` (минимум 2x)
- Отключите AGC: `LINE_CAM_AGC_ENABLE 0`

**Для темных сцен:**
- Увеличьте потолок: `LINE_CAM_GAIN_CEILING 2-3` (8x-16x)

### 6. **Контраст**
Для чёрно-белой линии можно увеличить:
```cpp
#define LINE_CAM_CONTRAST 0             // Попробуйте +1 или +2 для лучшего разделения
```

## Как настроить под ваши условия

### Шаг 1: Посмотрите на изображение с камеры
Откройте веб-интерфейс робота и перейдите в стрим камеры.

### Шаг 2: Определите проблему

**Проблема: Засветы, белые пятна**
```cpp
// В hardware_config.h измените:
#define LINE_CAM_AEC_VALUE 200          // Было 300 → уменьшаем
#define LINE_CAM_AE_LEVEL -1            // Было 0 → затемняем
#define LINE_CAM_AGC_ENABLE 0           // Отключаем усиление
```

**Проблема: Слишком темно**
```cpp
#define LINE_CAM_AEC_VALUE 500          // Было 300 → увеличиваем
#define LINE_CAM_AE_LEVEL +1            // Было 0 → осветляем
#define LINE_CAM_GAIN_CEILING 2         // Было 0 → увеличиваем усиление (8x)
```

**Проблема: Плохой контраст линии**
```cpp
#define LINE_CAM_CONTRAST +1            // Было 0 → увеличиваем
#define LINE_CAM_RAW_GMA_ENABLE 1       // Gamma коррекция обязательна
```

**Проблема: Мерцание/нестабильность**
```cpp
#define LINE_CAM_AEC2_ENABLE 0          // DSP auto exposure отключен (мешает)
#define LINE_CAM_AWB_ENABLE 0           // Отключить AWB если мерцает цвет
```

### Шаг 3: Перекомпилируйте и загрузите прошивку
```bash
pio run -t upload
```

### Шаг 4: Проверьте в Serial Monitor
При старте увидите:
```
✓ Настройки камеры для линии применены:
  - Экспозиция: AEC=1, AE_level=0, AEC_value=300
  - Усиление: AGC=1, AGC_gain=0, Ceiling=0
  - Коррекция: WPC=1 (засветы), GMA=1 (контраст), LENC=1 (виньет)
```

## Рекомендуемые пресеты

### Пресет 1: Яркое освещение (офис, лампы)
```cpp
#define LINE_CAM_AEC_VALUE 200
#define LINE_CAM_AE_LEVEL -1
#define LINE_CAM_AGC_ENABLE 0
#define LINE_CAM_CONTRAST +1
```

### Пресет 2: Нормальное освещение (комната)
```cpp
#define LINE_CAM_AEC_VALUE 300
#define LINE_CAM_AE_LEVEL 0
#define LINE_CAM_AGC_ENABLE 1
#define LINE_CAM_GAIN_CEILING 0
#define LINE_CAM_CONTRAST 0
```

### Пресет 3: Слабое освещение (вечер)
```cpp
#define LINE_CAM_AEC_VALUE 500
#define LINE_CAM_AE_LEVEL +1
#define LINE_CAM_AGC_ENABLE 1
#define LINE_CAM_GAIN_CEILING 2
#define LINE_CAM_CONTRAST +1
```

## Диагностика через Python

Можно захватить кадр и посмотреть гистограмму яркости:

```python
import cv2
import numpy as np
import requests
from io import BytesIO
from PIL import Image
import matplotlib.pyplot as plt

# Получить кадр с робота
response = requests.get("http://192.168.4.1/cam-lo.jpg")
img = Image.open(BytesIO(response.content)).convert('L')
img_array = np.array(img)

# Гистограмма яркости
plt.figure(figsize=(12, 4))
plt.subplot(1, 2, 1)
plt.imshow(img_array, cmap='gray')
plt.title('Изображение с камеры')

plt.subplot(1, 2, 2)
plt.hist(img_array.ravel(), bins=256, range=(0, 256))
plt.title('Гистограмма яркости')
plt.xlabel('Яркость (0=чёрный, 255=белый)')
plt.ylabel('Количество пикселей')
plt.axvline(x=128, color='r', linestyle='--', label='Порог (128)')
plt.legend()
plt.show()

# Анализ
mean_brightness = np.mean(img_array)
print(f"Средняя яркость: {mean_brightness:.1f}")

if mean_brightness > 180:
    print("⚠️  ЗАСВЕТ! Уменьшите AEC_VALUE или AE_LEVEL")
elif mean_brightness < 80:
    print("⚠️  СЛИШКОМ ТЕМНО! Увеличьте AEC_VALUE или AE_LEVEL")
else:
    print("✓ Яркость в норме")
```

## Важные замечания

1. **AEC (Auto Exposure Control)** - самый важный параметр для засветов
2. **WPC (White Pixel Correction)** - убирает артефакты
3. **Gamma коррекция** - улучшает контраст
4. **Не включайте AEC2** - DSP auto exposure обычно мешает
5. **Калибруйте на реальной трассе** - условия освещения могут отличаться

## Полезные ссылки

- [ESP32-CAM Sensor API](https://github.com/espressif/esp32-camera/blob/master/driver/include/sensor.h)
- [OV2640 Datasheet](https://www.uctronics.com/download/cam_module/OV2640DS.pdf)
- Параметры в коде: `include/hardware_config.h` (строки 148-180)
- Применение настроек: `src/BaseRobot.cpp` (строки 563-600)
