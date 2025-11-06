#!/usr/bin/env python3
"""
Тесты алгоритма распознавания линии для МикРоББокс Лайнер

Этот скрипт тестирует алгоритм detectLinePosition() на реальных снимках
с камеры робота. Учитывает настройки камеры (hMirror, vFlip).

Структура данных:
data/
  img_straight/  - линия прямо (ожидается ~0.0)
  img_left/      - линия слева (ожидается < 0)
  img_right/     - линия справа (ожидается > 0)
  img terminate/ - конец линии (T-пересечение или обрыв)
"""

import os
import sys
from pathlib import Path
import numpy as np
from PIL import Image
import json

# Константы из hardware_config.h
LINE_CAMERA_WIDTH = 160
LINE_CAMERA_HEIGHT = 120
LINE_THRESHOLD = 128
LINE_T_JUNCTION_THRESHOLD = 0.7


def load_camera_config(config_path='data/camera_config.json'):
    """Загрузка настроек камеры из конфигурации"""
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config = json.load(f)
            return config.get('camera', {})
    return {'hMirror': True, 'vFlip': True}


def apply_camera_transforms(image):
    """Применяет трансформации камеры (отражения)"""
    img_array = np.array(image)    
    return img_array


def normalize_image(img_array, use_edge_detection=True, use_binarization=True):
    """
    Обработка изображения с усилением контраста, детекцией границ и бинаризацией.
    
    Проблема: изображения с камеры робота имеют низкую яркость и плохой контраст,
    из-за чего линия плохо различима на фоне.
    
    Решение: 
    1. Усиление контраста (растяжение гистограммы)
    2. Edge detection (детекция границ) для выделения линии
    3. Бинаризация для получения четкого черно-белого изображения
    
    Args:
        img_array: numpy массив изображения в grayscale
        use_edge_detection: применять ли детекцию границ
        use_binarization: применять ли бинаризацию
    
    Returns:
        numpy массив обработанного изображения [0, 255]
    """
    # 1. Растяжение контраста (нормализация гистограммы)
    min_val = img_array.min()
    max_val = img_array.max()
    
    if max_val > min_val:
        img_contrast = ((img_array.astype(np.float32) - min_val) / (max_val - min_val) * 255.0).astype(np.uint8)
    else:
        img_contrast = img_array
    
    # 2. Детекция границ (опционально)
    if use_edge_detection:
        # Фильтр Собеля для детекции границ линии
        # Помогает выделить края линии на фоне
        edges = apply_sobel_filter(img_contrast)
        
        # Комбинируем исходное изображение с границами
        # np.maximum берет максимум из двух значений, что позволяет
        # сохранить яркие области исходного изображения и добавить выделенные границы
        img_result = np.maximum(img_contrast, edges)
    else:
        img_result = img_contrast
    
    # 3. Бинаризация (приведение к черно-белому)
    if use_binarization:
        # Используем адаптивный порог (метод Otsu)
        # Находим оптимальный порог автоматически
        threshold = calculate_otsu_threshold(img_result)
        img_result = np.where(img_result > threshold, 255, 0).astype(np.uint8)
    
    return img_result


def apply_sobel_filter(img_array):
    """
    Применение фильтра Собеля для детекции границ.
    
    Примечание: Реализация использует циклы для простоты и отсутствия зависимостей.
    Для больших изображений или production использования рекомендуется
    использовать scipy.ndimage.convolve или cv2.filter2D для лучшей производительности.
    
    Для текущих изображений 160x120 производительность приемлема.
    """
    img_float = img_array.astype(np.float32)
    height, width = img_float.shape
    
    # Sobel ядра для горизонтальных и вертикальных границ
    sobel_x = np.array([[-1, 0, 1],
                        [-2, 0, 2],
                        [-1, 0, 1]], dtype=np.float32)
    
    sobel_y = np.array([[-1, -2, -1],
                        [ 0,  0,  0],
                        [ 1,  2,  1]], dtype=np.float32)
    
    # Результирующее изображение
    edges = np.zeros_like(img_float)
    
    # Применяем свертку вручную (только для внутренних пикселей)
    for y in range(1, height - 1):
        for x in range(1, width - 1):
            # Извлекаем окрестность 3x3
            region = img_float[y-1:y+2, x-1:x+2]
            
            # Вычисляем градиенты
            gx = np.sum(region * sobel_x)
            gy = np.sum(region * sobel_y)
            
            # Величина градиента
            edges[y, x] = np.sqrt(gx**2 + gy**2)
    
    # Нормализуем результат
    if edges.max() > 0:
        edges = (edges / edges.max() * 255.0).astype(np.uint8)
    else:
        edges = edges.astype(np.uint8)
    
    return edges


def calculate_otsu_threshold(img_array):
    """
    Вычисление оптимального порога бинаризации методом Otsu.
    Автоматически находит порог, который лучше всего разделяет
    темный фон и светлую линию.
    
    Примечание: Для оптимизации можно использовать готовые реализации
    из opencv (cv2.threshold с THRESH_OTSU) или skimage.filters.threshold_otsu.
    Текущая реализация не требует дополнительных зависимостей.
    """
    # Построение гистограммы
    hist, bin_edges = np.histogram(img_array.flatten(), bins=256, range=(0, 256))
    
    # Нормализация гистограммы
    hist = hist.astype(np.float32)
    hist_norm = hist / hist.sum()
    
    # Вычисление кумулятивных сумм
    cumsum = np.cumsum(hist_norm)
    cumsum_mean = np.cumsum(hist_norm * np.arange(256))
    
    # Полная средняя яркость
    global_mean = cumsum_mean[-1]
    
    # Вычисление межклассовой дисперсии для каждого порога
    max_variance = 0
    best_threshold = 128
    
    for t in range(1, 255):
        w0 = cumsum[t]
        w1 = 1.0 - w0
        
        # Пропускаем случаи когда один из классов пуст
        if w0 == 0 or w1 == 0:
            continue
        
        mean0 = cumsum_mean[t] / w0
        mean1 = (global_mean - cumsum_mean[t]) / w1
        
        # Межклассовая дисперсия
        variance = w0 * w1 * (mean0 - mean1) ** 2
        
        if variance > max_variance:
            max_variance = variance
            best_threshold = t
    
    return best_threshold


def detect_line_position(image_path, camera_config):
    """
    Реализация алгоритма detectLinePosition() из LinerRobot.cpp
    
    Args:
        image_path: путь к изображению
        camera_config: настройки камеры {hMirror, vFlip}
    
    Returns:
        dict: {
            'position': float,      # -1.0 (слева) до 1.0 (справа)
            'detected': bool,       # найдена ли линия
            'width_percent': float, # % ширины кадра занятый линией
            'is_terminate': bool    # T-пересечение или обрыв
        }
    """
    # Загрузка изображения
    img = Image.open(image_path)
    
    # Конвертация в grayscale если нужно
    if img.mode != 'L':
        img = img.convert('L')
    
    # Изменение размера до LINE_CAMERA_WIDTH x LINE_CAMERA_HEIGHT
    if img.size != (LINE_CAMERA_WIDTH, LINE_CAMERA_HEIGHT):
        img = img.resize((LINE_CAMERA_WIDTH, LINE_CAMERA_HEIGHT), Image.Resampling.LANCZOS)
    
    # Применяем трансформации камеры согласно конфигурации
    # Эти трансформации нужны для правильной интерпретации направления движения
    img_array = apply_camera_transforms(
        img, 
        camera_config.get('hMirror', True),
        camera_config.get('vFlip', True)
    )
    
    # Применяем обработку изображения: усиление контраста, edge detection, бинаризацию
    img_array = normalize_image(img_array)
    
    # Параметры сканирования
    width = img_array.shape[1]
    height = img_array.shape[0]
    scan_line = int(height * 3 / 4)  # Сканируем на 75% высоты
    
    # Подсчет суммы позиций белых пикселей
    sum_position = 0.0
    count = 0
    
    for x in range(width):
        pixel = img_array[scan_line, x]
        
        if pixel > LINE_THRESHOLD:
            # Белый пиксель (линия)
            sum_position += float(x)
            count += 1
    
    result = {
        'position': 0.0,
        'detected': False,
        'width_percent': 0.0,
        'is_terminate': False,
        'scan_line': scan_line
    }
    
    if count == 0:
        # Линия не найдена (обрыв)
        result['is_terminate'] = True
        return result
    
    # Проверка на T-образное пересечение
    line_width_percent = float(count) / float(width)
    result['width_percent'] = line_width_percent
    
    if line_width_percent > LINE_T_JUNCTION_THRESHOLD:
        # T-образное пересечение
        result['is_terminate'] = True
        return result
    
    # Линия найдена
    result['detected'] = True
    
    # Средняя позиция линии
    avg_position = sum_position / float(count)
    
    # Нормализация от -1.0 (левый край) до 1.0 (правый край)
    normalized = (avg_position / float(width)) * 2.0 - 1.0
    result['position'] = normalized
    
    return result


def visualize_detection(image_path, result, camera_config, output_path=None):
    """Визуализация результата детекции"""
    import matplotlib.pyplot as plt
    
    # Загрузка изображения
    img = Image.open(image_path)
    if img.mode != 'L':
        img = img.convert('L')
    if img.size != (LINE_CAMERA_WIDTH, LINE_CAMERA_HEIGHT):
        img = img.resize((LINE_CAMERA_WIDTH, LINE_CAMERA_HEIGHT), Image.Resampling.LANCZOS)
    
    # Применяем трансформации камеры для правильной ориентации
    img_array = apply_camera_transforms(
        img,
        camera_config.get('hMirror', True),
        camera_config.get('vFlip', True)
    )
    
    # Применяем нормализацию (как в алгоритме детекции)
    img_normalized = normalize_image(img_array)
    
    # Создание визуализации с 3 панелями
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(18, 5))
    
    # Исходное изображение
    ax1.imshow(img_array, cmap='gray', vmin=0, vmax=255)
    ax1.set_title(f'After Camera Transforms\n{os.path.basename(image_path)}\nMin: {img_array.min()}, Max: {img_array.max()}')
    ax1.axis('off')
    
    # Нормализованное изображение
    ax2.imshow(img_normalized, cmap='gray', vmin=0, vmax=255)
    ax2.set_title(f'Processed (edges + binary)\nMin: {img_normalized.min()}, Max: {img_normalized.max()}')
    ax2.axis('off')
    
    # Изображение с детекцией
    ax3.imshow(img_normalized, cmap='gray')
    
    # Линия сканирования
    scan_line = result['scan_line']
    ax3.axhline(y=scan_line, color='red', linestyle='--', linewidth=1, label='Scan line')
    
    # Отображение позиции линии
    if result['detected']:
        # Преобразование normalized position обратно в пиксели
        position_normalized = result['position']
        position_pixel = (position_normalized + 1.0) * LINE_CAMERA_WIDTH / 2.0
        
        ax3.axvline(x=position_pixel, color='green', linewidth=2, label=f'Line center: {position_normalized:.2f}')
        
        # Стрелка направления
        if abs(position_normalized) > 0.1:
            direction = 'LEFT' if position_normalized < 0 else 'RIGHT'
            color = 'yellow' if position_normalized < 0 else 'cyan'
            ax3.text(position_pixel, scan_line - 10, f'← {direction}' if position_normalized < 0 else f'{direction} →',
                    color=color, fontsize=12, fontweight='bold',
                    ha='center', va='bottom')
    
    # Информация о результате
    info_text = f"Position: {result['position']:.3f}\n"
    info_text += f"Detected: {result['detected']}\n"
    info_text += f"Width: {result['width_percent']*100:.1f}%\n"
    info_text += f"Terminate: {result['is_terminate']}"
    
    ax3.text(5, 5, info_text, color='white', fontsize=10,
            bbox=dict(boxstyle='round', facecolor='black', alpha=0.7),
            verticalalignment='top')
    
    ax3.set_title('Line Detection Result')
    ax3.legend(loc='upper right')
    ax3.axis('off')
    
    plt.tight_layout()
    
    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"Saved visualization: {output_path}")
    else:
        plt.show()
    
    plt.close()


def test_category(category_path, expected_range, camera_config, visualize=False):
    """
    Тестирование категории изображений
    
    Args:
        category_path: путь к папке с изображениями
        expected_range: ожидаемый диапазон (min, max) или 'terminate'
        camera_config: настройки камеры
        visualize: создавать ли визуализацию
    """
    category_name = os.path.basename(category_path)
    print(f"\n{'='*60}")
    print(f"Тестирование категории: {category_name}")
    print(f"{'='*60}")
    
    if not os.path.exists(category_path):
        print(f"⚠️  Папка не найдена: {category_path}")
        return []
    
    image_files = [f for f in os.listdir(category_path) if f.lower().endswith(('.jpg', '.jpeg', '.png'))]
    
    if not image_files:
        print(f"⚠️  Нет изображений в папке: {category_path}")
        return []
    
    results = []
    
    for img_file in sorted(image_files):
        img_path = os.path.join(category_path, img_file)
        print(f"\n📷 {img_file}:")
        
        result = detect_line_position(img_path, camera_config)
        results.append(result)
        
        print(f"   Position: {result['position']:+.3f}")
        print(f"   Detected: {result['detected']}")
        print(f"   Width: {result['width_percent']*100:.1f}%")
        print(f"   Terminate: {result['is_terminate']}")
        
        # Проверка ожиданий
        if expected_range == 'terminate':
            if result['is_terminate']:
                print("   ✅ PASS - Правильно определен конец линии")
            else:
                print("   ❌ FAIL - Ожидался конец линии, но линия обнаружена")
        else:
            min_val, max_val = expected_range
            if result['detected'] and min_val <= result['position'] <= max_val:
                print(f"   ✅ PASS - Позиция в ожидаемом диапазоне [{min_val}, {max_val}]")
            else:
                print(f"   ❌ FAIL - Позиция вне диапазона [{min_val}, {max_val}]")
        
        # Визуализация
        if visualize:
            output_dir = os.path.join('test', 'output', category_name)
            os.makedirs(output_dir, exist_ok=True)
            output_path = os.path.join(output_dir, f'{os.path.splitext(img_file)[0]}_result.png')
            visualize_detection(img_path, result, camera_config, output_path)
    
    return results


def main():
    """Главная функция тестирования"""
    print("="*60)
    print("ТЕСТ АЛГОРИТМА РАСПОЗНАВАНИЯ ЛИНИИ")
    print("МикРоББокс Лайнер")
    print("="*60)
    
    # Загрузка конфигурации камеры
    config_path = os.path.join('data', 'camera_config.json')
    camera_config = load_camera_config(config_path)
    
    print(f"\n📷 Настройки камеры:")
    print(f"   H-Mirror: {camera_config.get('hMirror', True)}")
    print(f"   V-Flip: {camera_config.get('vFlip', True)}")
    
    # Проверяем наличие matplotlib для визуализации
    visualize = True
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("\n⚠️  matplotlib не установлен, визуализация отключена")
        print("   Для установки: pip install matplotlib")
        visualize = False
    
    # Определение путей к категориям
    data_dir = 'data'
    categories = {
        'straight': (os.path.join(data_dir, 'img_straight'), (-0.2, 0.2)),
        'left': (os.path.join(data_dir, 'img_left'), (-1.0, -0.1)),
        'right': (os.path.join(data_dir, 'img_right'), (0.1, 1.0)),
        'terminate': (os.path.join(data_dir, 'img terminate'), 'terminate')
    }
    
    # Тестирование каждой категории
    all_results = {}
    for cat_name, (cat_path, expected_range) in categories.items():
        results = test_category(cat_path, expected_range, camera_config, visualize)
        all_results[cat_name] = results
    
    # Итоговая статистика
    print(f"\n{'='*60}")
    print("ИТОГОВАЯ СТАТИСТИКА")
    print(f"{'='*60}")
    
    total_tests = sum(len(results) for results in all_results.values())
    print(f"Всего протестировано изображений: {total_tests}")
    
    for cat_name, results in all_results.items():
        if results:
            print(f"\n{cat_name.upper()}:")
            print(f"  Изображений: {len(results)}")
            detected = sum(1 for r in results if r['detected'])
            print(f"  Обнаружено: {detected}/{len(results)}")
            if detected > 0:
                positions = [r['position'] for r in results if r['detected']]
                print(f"  Позиция: мин={min(positions):.3f}, макс={max(positions):.3f}, средн={np.mean(positions):.3f}")


if __name__ == '__main__':
    # Проверка наличия необходимых библиотек
    try:
        import numpy
        import PIL
    except ImportError as e:
        print(f"❌ Ошибка: {e}")
        print("\nУстановите необходимые библиотеки:")
        print("pip install numpy pillow matplotlib")
        sys.exit(1)
    
    main()
