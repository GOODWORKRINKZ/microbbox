# Быстрая настройка камеры против засветов

## Если есть ЗАСВЕТЫ (слишком ярко):

Откройте `include/hardware_config.h` и измените:

```cpp
#define LINE_CAM_AEC_VALUE 200          // Было 300 → уменьшаем экспозицию
#define LINE_CAM_AE_LEVEL -1            // Было 0 → затемняем
#define LINE_CAM_AGC_ENABLE 0           // Отключаем автоусиление
```

## Если СЛИШКОМ ТЕМНО:

```cpp
#define LINE_CAM_AEC_VALUE 500          // Было 300 → увеличиваем экспозицию
#define LINE_CAM_AE_LEVEL +1            // Было 0 → осветляем
#define LINE_CAM_GAIN_CEILING 2         // Было 0 → добавляем усиление (8x)
```

## Если ПЛОХОЙ КОНТРАСТ:

```cpp
#define LINE_CAM_CONTRAST +1            // Было 0 → увеличиваем контраст
```

## Основные параметры:

- **LINE_CAM_AEC_VALUE** (0-1200) - яркость: 200=темно, 300=норма, 500=светло
- **LINE_CAM_AE_LEVEL** (-2 to +2) - компенсация: -1=темнее, 0=норма, +1=светлее
- **LINE_CAM_AGC_ENABLE** (0/1) - усиление: 0=выкл, 1=авто
- **LINE_CAM_GAIN_CEILING** (0-6) - потолок усиления: 0=2x, 2=8x, 4=32x, 6=128x
- **LINE_CAM_CONTRAST** (-2 to +2) - контраст: 0=норма, +1/+2=больше

После изменений: `pio run -t upload`

---

Подробная документация: **LINER_CAMERA_TUNING.md**
