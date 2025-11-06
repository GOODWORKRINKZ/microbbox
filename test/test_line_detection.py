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

Физические параметры робота:
  Камера расположена на расстоянии 81.62 мм впереди центра оси вращения колес.
  
  Для ПИД-регулятора:
  - Алгоритм возвращает позицию линии относительно центра камеры
  - ПИД должен учитывать смещение камеры при расчете управляющего воздействия
  - Упрощенная формула: correction = Kp * position + Kd * trend + camera_offset_correction
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

# Физические параметры робота
CAMERA_TO_WHEEL_AXIS_DISTANCE_MM = 81.62  # Расстояние от центра камеры до центра оси вращения колес (мм)

# Константы для анализа тренда направления
TREND_THRESHOLD_SHARP_TURN = 0.7    # Очень крутой поворот (90°)
TREND_THRESHOLD_MEDIUM_TURN = 0.5   # Крутой поворот
TREND_THRESHOLD_GENTLE_TURN = 0.3   # Средний поворот

# Веса для вычисления финальной позиции
# Примечание: каждая пара (BASE + TREND) должна в сумме давать 1.0 для правильной нормализации
WEIGHT_SHARP_TURN_BASE = 0.2        # Вес базовой позиции при крутом повороте
WEIGHT_SHARP_TURN_TREND = 0.8       # Вес тренда при крутом повороте (0.2 + 0.8 = 1.0)
WEIGHT_MEDIUM_TURN_BASE = 0.3       # Вес базовой позиции при среднем повороте
WEIGHT_MEDIUM_TURN_TREND = 0.7      # Вес тренда при среднем повороте (0.3 + 0.7 = 1.0)
WEIGHT_GENTLE_TURN_BASE = 0.5       # Вес базовой позиции при плавном повороте
WEIGHT_GENTLE_TURN_TREND = 0.5      # Вес тренда при плавном повороте (0.5 + 0.5 = 1.0)
WEIGHT_NORMAL_BASE = 0.7            # Вес базовой позиции при нормальном движении
WEIGHT_NORMAL_TREND = 0.3           # Вес тренда при нормальном движении (0.7 + 0.3 = 1.0)

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


def calculate_weighted_position(base_position, trend, weight_base, weight_trend):
    """
    Вспомогательная функция для вычисления взвешенной позиции линии.
    
    Args:
        base_position: базовая позиция (обычно нижняя сканирующая линия)
        trend: тренд направления движения
        weight_base: вес базовой позиции
        weight_trend: вес тренда
    
    Returns:
        float: взвешенная позиция
    """
    return base_position * weight_base + trend * weight_trend


def detect_line_position(image_path):
    """
    Реализация алгоритма detectLinePosition() с использованием 4 горизонтальных 
    и 4 вертикальных сканирующих линий для максимально точного определения направления.
    
    ВАЖНО: Центр изображения (X=80, позиция=0.0) соответствует центру камеры/робота!
    - Линия в центре (позиция ≈ 0.0) → робот едет ПРЯМО
    - Линия слева (позиция < 0) → робот поворачивает ВЛЕВО, чтобы вернуться на линию
    - Линия справа (позиция > 0) → робот поворачивает ВПРАВО, чтобы вернуться на линию
    
    ФИЗИЧЕСКИЕ ПАРАМЕТРЫ:
    - Расстояние от центра камеры до центра оси вращения колес: 81.62 мм
    - Это смещение должно учитываться в ПИД-регуляторе при управлении моторами
    - Позиция возвращается относительно центра камеры, а не оси вращения
    
    Улучшения:
    - 4 горизонтальные линии (25%, 50%, 75%, 90% высоты) для точного определения траектории
    - 4 вертикальные линии (20%, 40%, 60%, 80% ширины) для точного определения T-пересечений
    - Вычисляет максимальный тренд между любыми двумя горизонтальными линиями
    - Сильно увеличивает влияние тренда для крутых поворотов (|тренд| > 0.7)
    
    Args:
        image_path: путь к изображению
    
    Returns:
        dict: {
            'position': float,      # -1.0 (линия слева) до 1.0 (линия справа) относительно центра камеры
            'detected': bool,       # найдена ли линия
            'width_percent': float, # % ширины кадра занятый линией
            'is_terminate': bool,   # T-пересечение или обрыв
            'horizontal_scans': list  # результаты горизонтального сканирования (4 линии)
            'vertical_scans': list    # результаты вертикального сканирования (4 линии)
            'direction_trend': float  # максимальный тренд направления (для предсказания траектории)
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
    img_array = apply_camera_transforms(img)
    
    # Применяем обработку изображения: усиление контраста, edge detection, бинаризацию
    img_array = normalize_image(img_array)
    
    # Параметры сканирования
    width = img_array.shape[1]
    height = img_array.shape[0]
    
    # === 1. ЧЕТЫРЕ ГОРИЗОНТАЛЬНЫЕ СКАНИРУЮЩИЕ ЛИНИИ ===
    # Для точного определения крутых поворотов и направления движения
    horizontal_scan_heights = [
        int(height * 0.25),  # 25% - верхняя (самая дальняя)
        int(height * 0.50),  # 50% - средняя-верхняя
        int(height * 0.75),  # 75% - средняя-нижняя
        int(height * 0.90),  # 90% - нижняя (самая близкая к роботу)
    ]
    
    horizontal_results = []
    
    for scan_y in horizontal_scan_heights:
        sum_position = 0.0
        count = 0
        
        # Сканируем горизонтально
        for x in range(width):
            pixel = img_array[scan_y, x]
            
            if pixel > LINE_THRESHOLD:
                sum_position += float(x)
                count += 1
        
        if count > 0:
            avg_position = sum_position / float(count)
            normalized = (avg_position / float(width)) * 2.0 - 1.0
            width_percent = float(count) / float(width)
        else:
            avg_position = None
            normalized = None
            width_percent = 0.0
        
        horizontal_results.append({
            'y': scan_y,
            'position': normalized,
            'pixel_position': avg_position,
            'count': count,
            'width_percent': width_percent
        })
    
    # === 2. ЧЕТЫРЕ ВЕРТИКАЛЬНЫЕ СКАНИРУЮЩИЕ ЛИНИИ ===
    # Для более точного определения T-пересечений и ширины линии
    vertical_scan_positions = [
        int(width * 0.20),   # 20% - левая
        int(width * 0.40),   # 40% - средняя-левая
        int(width * 0.60),   # 60% - средняя-правая
        int(width * 0.80),   # 80% - правая
    ]
    
    vertical_results = []
    
    for scan_x in vertical_scan_positions:
        sum_position = 0.0
        count = 0
        
        # Сканируем вертикально
        for y in range(height):
            pixel = img_array[y, scan_x]
            
            if pixel > LINE_THRESHOLD:
                sum_position += float(y)
                count += 1
        
        if count > 0:
            avg_position = sum_position / float(count)
            # Для вертикали нормализуем по высоте
            normalized = (avg_position / float(height)) * 2.0 - 1.0
            height_percent = float(count) / float(height)
        else:
            avg_position = None
            normalized = None
            height_percent = 0.0
        
        vertical_results.append({
            'x': scan_x,
            'position': normalized,
            'pixel_position': avg_position,
            'count': count,
            'height_percent': height_percent
        })
    
    # === АНАЛИЗ РЕЗУЛЬТАТОВ ===
    
    result = {
        'position': 0.0,
        'detected': False,
        'width_percent': 0.0,
        'is_terminate': False,
        'horizontal_scans': horizontal_results,
        'vertical_scans': vertical_results,
        'direction_trend': 0.0
    }
    
    # Проверяем, найдена ли линия на горизонтальных сканах
    detected_horizontal = [r for r in horizontal_results if r['position'] is not None]
    
    if len(detected_horizontal) == 0:
        # Линия не найдена ни на одной горизонтальной линии (обрыв)
        result['is_terminate'] = True
        return result
    
    # Средняя ширина линии по горизонтальным сканам
    total_width = sum(r['width_percent'] for r in horizontal_results)
    avg_width_percent = total_width / len(horizontal_results)
    result['width_percent'] = avg_width_percent
    
    # === ПРОВЕРКА НА T-ПЕРЕСЕЧЕНИЕ ===
    # 1. Если линия очень широкая на горизонтальных сканах (занимает > 70% ширины)
    wide_horizontal = sum(1 for r in horizontal_results if r['width_percent'] > LINE_T_JUNCTION_THRESHOLD)
    
    # 2. Если на 3 или 4 вертикальных сканах линия занимает много высоты (> 50%)
    #    это означает что линия идет вертикально (T-образное пересечение)
    tall_vertical = sum(1 for r in vertical_results if r['height_percent'] > 0.5)
    
    # T-пересечение определяется по:
    # - Широкой горизонтальной линии на 2+ уровнях
    # - Или высокой вертикальной линии на 3+ позициях
    if wide_horizontal >= 2 or tall_vertical >= 3:
        result['is_terminate'] = True
        return result
    
    # Линия найдена
    result['detected'] = True
    
    # === ВЫЧИСЛЕНИЕ ПОЗИЦИИ С УЧЕТОМ НАПРАВЛЕНИЯ ДВИЖЕНИЯ ===
    # 
    # КЛЮЧЕВАЯ ИДЕЯ: Робот следует за линией, которая ведет от верхней части кадра к нижней.
    # Для крутых поворотов (90 градусов) важно определить сильное изменение позиции!
    #
    # Алгоритм:
    # 1. Смотрим на позицию линии на 3 горизонтальных линиях (33%, 60%, 85%)
    # 2. Вычисляем максимальный тренд между любыми двумя линиями
    # 3. Для крутых поворотов (|тренд| > 0.5) сильно увеличиваем влияние тренда
    
    # Если хотя бы 2 горизонтальные линии нашли позицию
    if len(detected_horizontal) >= 2:
        # Вычисляем все возможные тренды
        max_trend = 0.0
        max_trend_pair = None
        
        for i in range(len(horizontal_results)):
            for j in range(i + 1, len(horizontal_results)):
                pos_i = horizontal_results[i]['position']
                pos_j = horizontal_results[j]['position']
                
                if pos_i is not None and pos_j is not None:
                    # Тренд от дальней линии к ближней (от меньшего Y к большему Y)
                    trend = pos_j - pos_i
                    if abs(trend) > abs(max_trend):
                        max_trend = trend
                        max_trend_pair = (i, j)
        
        result['direction_trend'] = max_trend
        
        # Берем нижнюю (ближайшую к роботу) линию как базовую позицию
        pos_bottom = None
        for i in range(len(horizontal_results) - 1, -1, -1):
            if horizontal_results[i]['position'] is not None:
                pos_bottom = horizontal_results[i]['position']
                break
        
        if pos_bottom is None:
            # Если нижней нет, берем любую доступную
            pos_bottom = detected_horizontal[0]['position']
        
        # Определяем силу тренда (насколько крутой поворот)
        trend_strength = abs(max_trend)
        
        # Выбираем веса в зависимости от силы тренда и вычисляем позицию
        if trend_strength > TREND_THRESHOLD_SHARP_TURN:
            # ОЧЕНЬ крутой поворот (почти 90 градусов) - тренд доминирует
            result['position'] = calculate_weighted_position(
                pos_bottom, max_trend, WEIGHT_SHARP_TURN_BASE, WEIGHT_SHARP_TURN_TREND)
        elif trend_strength > TREND_THRESHOLD_MEDIUM_TURN:
            # Крутой поворот - сильное влияние тренда
            result['position'] = calculate_weighted_position(
                pos_bottom, max_trend, WEIGHT_MEDIUM_TURN_BASE, WEIGHT_MEDIUM_TURN_TREND)
        elif trend_strength > TREND_THRESHOLD_GENTLE_TURN:
            # Средний поворот - усиленное влияние тренда
            result['position'] = calculate_weighted_position(
                pos_bottom, max_trend, WEIGHT_GENTLE_TURN_BASE, WEIGHT_GENTLE_TURN_TREND)
        else:
            # Плавный поворот или прямая
            result['position'] = calculate_weighted_position(
                pos_bottom, max_trend, WEIGHT_NORMAL_BASE, WEIGHT_NORMAL_TREND)
        
    elif len(detected_horizontal) == 1:
        # Только одна линия найдена - используем её позицию
        result['position'] = detected_horizontal[0]['position']
        result['direction_trend'] = 0.0
    
    return result


def visualize_detection(image_path, result, output_path=None):
    """Визуализация результата детекции с 4 горизонтальными и 4 вертикальными сканирующими линиями"""
    import matplotlib.pyplot as plt
    
    # Загрузка изображения
    img = Image.open(image_path)
    if img.mode != 'L':
        img = img.convert('L')
    if img.size != (LINE_CAMERA_WIDTH, LINE_CAMERA_HEIGHT):
        img = img.resize((LINE_CAMERA_WIDTH, LINE_CAMERA_HEIGHT), Image.Resampling.LANCZOS)
    
    # Применяем трансформации камеры для правильной ориентации
    img_array = apply_camera_transforms(img)
    
    # Применяем нормализацию (как в алгоритме детекции)
    img_normalized = normalize_image(img_array)
    
    # Создание визуализации с 3 панелями
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(18, 5))
    
    # Исходное изображение
    ax1.imshow(img_array, cmap='gray', vmin=0, vmax=255)
    ax1.set_title(f'После трансформаций камеры\n{os.path.basename(image_path)}\nMin: {img_array.min()}, Max: {img_array.max()}')
    ax1.axis('off')
    
    # Нормализованное изображение
    ax2.imshow(img_normalized, cmap='gray', vmin=0, vmax=255)
    ax2.set_title(f'Обработанное (границы + бинаризация)\nMin: {img_normalized.min()}, Max: {img_normalized.max()}')
    ax2.axis('off')
    
    # Изображение с детекцией
    ax3.imshow(img_normalized, cmap='gray')
    
    # === ЧЕТЫРЕ ГОРИЗОНТАЛЬНЫЕ СКАНИРУЮЩИЕ ЛИНИИ ===
    if 'horizontal_scans' in result:
        h_colors = ['cyan', 'yellow', 'orange', 'red']
        h_labels = ['25%', '50%', '75%', '90%']
        
        for i, scan_info in enumerate(result['horizontal_scans']):
            scan_y = scan_info['y']
            color = h_colors[i]
            label = h_labels[i]
            
            # Горизонтальная линия сканирования
            ax3.axhline(y=scan_y, color=color, linestyle='--', linewidth=1.5, 
                       alpha=0.8, label=f'Г-{label}')
            
            # Если на этой линии найдена позиция, отмечаем её
            if scan_info['position'] is not None:
                position_pixel = scan_info['pixel_position']
                ax3.plot(position_pixel, scan_y, 'o', color=color, 
                        markersize=8, markeredgecolor='white', markeredgewidth=1.5)
    
    # === ЧЕТЫРЕ ВЕРТИКАЛЬНЫЕ СКАНИРУЮЩИЕ ЛИНИИ ===
    if 'vertical_scans' in result:
        v_colors = ['cyan', 'lightblue', 'pink', 'magenta']
        v_labels = ['20%', '40%', '60%', '80%']
        
        for i, scan_info in enumerate(result['vertical_scans']):
            scan_x = scan_info['x']
            color = v_colors[i]
            label = v_labels[i]
            
            # Вертикальная линия сканирования
            ax3.axvline(x=scan_x, color=color, linestyle=':', linewidth=1.5, 
                       alpha=0.6, label=f'В-{label}')
            
            # Если на этой линии найдена позиция, отмечаем её
            if scan_info['position'] is not None:
                position_pixel = scan_info['pixel_position']
                ax3.plot(scan_x, position_pixel, 's', color=color, 
                        markersize=6, markeredgecolor='white', markeredgewidth=1)
    
    # === ИТОГОВАЯ ПОЗИЦИЯ ЛИНИИ ===
    if result['detected']:
        position_normalized = result['position']
        position_pixel = (position_normalized + 1.0) * LINE_CAMERA_WIDTH / 2.0
        
        # Вертикальная линия центра
        ax3.axvline(x=position_pixel, color='lime', linewidth=4, 
                   label=f'ЦЕНТР: {position_normalized:.2f}', alpha=0.9)
        
        # Стрелка направления с учетом тренда
        trend = result.get('direction_trend', 0.0)
        
        if abs(trend) > 0.7:
            # Очень крутой поворот (90 градусов)
            if trend > 0:
                direction = '⟹ ПОВОРОТ 90° ВПРАВО'
                color = 'cyan'
            else:
                direction = '⟸ ПОВОРОТ 90° ВЛЕВО'
                color = 'yellow'
        elif abs(trend) > 0.4:
            # Крутой поворот
            if trend > 0:
                direction = '→ КРУТОЙ ВПРАВО'
                color = 'cyan'
            else:
                direction = '← КРУТОЙ ВЛЕВО'
                color = 'yellow'
        elif abs(position_normalized) > 0.15:
            if position_normalized < 0:
                direction = '← ВЛЕВО'
                color = 'yellow'
            else:
                direction = '→ ВПРАВО'
                color = 'cyan'
        else:
            direction = '↑ ПРЯМО'
            color = 'lime'
        
        ax3.text(position_pixel, 15, direction,
                color=color, fontsize=14, fontweight='bold',
                ha='center', va='top',
                bbox=dict(boxstyle='round', facecolor='black', alpha=0.8))
    
    # Информация о результате
    info_text = f"Позиция: {result['position']:.3f}\n"
    info_text += f"Тренд: {result.get('direction_trend', 0.0):.3f}\n"
    info_text += f"Обнаружена: {'ДА' if result['detected'] else 'НЕТ'}\n"
    info_text += f"Ширина: {result['width_percent']*100:.1f}%\n"
    info_text += f"Конец/T: {'ДА' if result['is_terminate'] else 'НЕТ'}"
    
    ax3.text(5, LINE_CAMERA_HEIGHT - 5, info_text, color='white', fontsize=10,
            bbox=dict(boxstyle='round', facecolor='black', alpha=0.85),
            verticalalignment='bottom', fontfamily='monospace')
    
    ax3.set_title('Детекция (4×4 линии)')
    ax3.legend(loc='upper right', fontsize=6, framealpha=0.9, ncol=2)
    ax3.axis('off')
    
    plt.tight_layout()
    
    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"Сохранена визуализация: {output_path}")
    else:
        plt.show()
    
    plt.close()


def test_category(category_path, expected_range, visualize=False):
    """
    Тестирование категории изображений
    
    Args:
        category_path: путь к папке с изображениями
        expected_range: ожидаемый диапазон (min, max) или 'terminate'
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
        
        result = detect_line_position(img_path)
        results.append(result)
        
        # Показываем детали сканирования
        h_scans = result['horizontal_scans']
        print(f"   Позиция: {result['position']:+.3f}")
        print(f"   Обнаружена: {result['detected']}")
        print(f"   Ширина: {result['width_percent']*100:.1f}%")
        print(f"   Тренд: {result['direction_trend']:+.3f}")
        
        # Детали горизонтальных сканов (верхний и нижний)
        # У нас 4 линии: [0]=25%, [1]=50%, [2]=75%, [3]=90%
        pos_top = h_scans[0]['position']  # 25% - самая верхняя (далеко)
        pos_bot = h_scans[-1]['position']  # 90% - самая нижняя (близко к роботу)
        if pos_top is not None and pos_bot is not None:
            print(f"   Сканы: верх={pos_top:+.3f}, низ={pos_bot:+.3f}")
        
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
                # Примечание о возможных проблемах с данными
                if not result['is_terminate'] and result['detected']:
                    if (result['position'] < 0 and min_val > 0):
                        print(f"   ⚠️  ЗАМЕЧАНИЕ: Линия слева, но ожидается справа")
                    elif (result['position'] > 0 and max_val < 0):
                        print(f"   ⚠️  ЗАМЕЧАНИЕ: Линия справа, но ожидается слева")
        
        # Визуализация
        if visualize:
            output_dir = os.path.join('test', 'output', category_name)
            os.makedirs(output_dir, exist_ok=True)
            output_path = os.path.join(output_dir, f'{os.path.splitext(img_file)[0]}_result.png')
            visualize_detection(img_path, result, output_path)
    
    return results


def main():
    """Главная функция тестирования"""
    print("="*60)
    print("ТЕСТ АЛГОРИТМА РАСПОЗНАВАНИЯ ЛИНИИ")
    print("МикРоББокс Лайнер")
    print("="*60)
    
    
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
        results = test_category(cat_path, expected_range, visualize)
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
