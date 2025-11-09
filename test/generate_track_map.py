#!/usr/bin/env python3
"""
Генератор карты трассы с черной линией (алгоритм Гамильтонова пути).

Создает:
1. Генерирует изображение трассы (3000×3000 пикселей) с черной линией на белом фоне
2. Линия состоит только из сегментов под углом 90° (вверх, вниз, влево, вправо)
3. Линия НИКОГДА не пересекает сама себя (гарантия алгоритма)
4. Извлекает последовательные кадры 160×120, двигаясь вдоль линии
5. JSON-файл с метаданными (правильные ответы для каждого кадра)
"""

import os
import json
import random
import math
from pathlib import Path
from PIL import Image, ImageDraw
import numpy as np

# Константы
OUTPUT_DIR = Path(__file__).parent / 'output'
TRACK_MAP_FILE = OUTPUT_DIR / 'track_map.json'
TRACK_MAP_IMAGE = OUTPUT_DIR / 'track_map_full.png'
TRACK_PREVIEW_IMAGE = OUTPUT_DIR / 'track_map_preview.png'

# Параметры трассы
TRACK_SIZE = 3000  # Размер поля трассы (пиксели)
LINE_WIDTH = 20    # Ширина черной линии (примерно 20 мм)

# Параметры камеры робота
CAMERA_WIDTH = 160
CAMERA_HEIGHT = 120

# Параметры генерации сетки
GRID_CELL_SIZE = 150  # Размер ячейки сетки (пикселей)
GRID_SIZE = TRACK_SIZE // GRID_CELL_SIZE  # Количество ячеек в сетке


def generate_hamiltonian_path():
    """
    Генерирует Гамильтонов путь на сетке (змейка).
    Гарантирует, что путь не пересекает сам себя и покрывает все ячейки.
    
    Returns:
        list: Список точек [(grid_x, grid_y), ...] в координатах сетки
    """
    path = []
    
    # Простой алгоритм "змейка" - проходим по всем ячейкам зигзагом
    # Это гарантирует отсутствие пересечений
    for y in range(GRID_SIZE):
        if y % 2 == 0:
            # Четные строки: слева направо
            for x in range(GRID_SIZE):
                path.append((x, y))
        else:
            # Нечетные строки: справа налево
            for x in range(GRID_SIZE - 1, -1, -1):
                path.append((x, y))
    
    return path


def add_random_variations(grid_path, variation_prob=0.3):
    """
    Добавляет случайные вариации к змейке, сохраняя отсутствие пересечений.
    
    Args:
        grid_path: Базовый Гамильтонов путь
        variation_prob: Вероятность добавления дополнительного сегмента
    
    Returns:
        list: Модифицированный путь с вариациями
    """
    # Создаем множество посещенных ячеек для быстрой проверки
    visited = set(grid_path)
    
    # Направления движения (вверх, вниз, влево, вправо)
    directions = [(0, -1), (0, 1), (-1, 0), (1, 0)]
    
    result_path = [grid_path[0]]
    
    for i in range(1, len(grid_path)):
        current = result_path[-1]
        target = grid_path[i]
        
        # С некоторой вероятностью пробуем добавить промежуточный сегмент
        if random.random() < variation_prob and i < len(grid_path) - 1:
            # Пробуем найти соседнюю непосещенную ячейку
            for dx, dy in random.sample(directions, len(directions)):
                neighbor_x = current[0] + dx
                neighbor_y = current[1] + dy
                
                # Проверяем границы и что ячейка не посещена ранее в текущем пути
                if (0 <= neighbor_x < GRID_SIZE and 
                    0 <= neighbor_y < GRID_SIZE and
                    (neighbor_x, neighbor_y) not in visited and
                    (neighbor_x, neighbor_y) != target):
                    
                    # Проверяем, можем ли мы вернуться к основному пути
                    # (есть ли путь от neighbor к target)
                    if can_reach(neighbor_x, neighbor_y, target[0], target[1]):
                        result_path.append((neighbor_x, neighbor_y))
                        visited.add((neighbor_x, neighbor_y))
                        break
        
        result_path.append(target)
    
    return result_path


def can_reach(from_x, from_y, to_x, to_y):
    """
    Простая проверка: можно ли достичь целевой ячейки (манхэттенское расстояние).
    """
    return abs(to_x - from_x) + abs(to_y - from_y) <= 3


def grid_to_pixel_path(grid_path):
    """
    Конвертирует путь в координатах сетки в пиксельные координаты.
    Добавляет интерполяцию для плавных линий.
    
    Args:
        grid_path: Путь в координатах сетки [(grid_x, grid_y), ...]
    
    Returns:
        list: Путь в пиксельных координатах [(x, y), ...]
    """
    pixel_path = []
    
    # Отступ от краев
    margin = GRID_CELL_SIZE // 2
    
    for i, (gx, gy) in enumerate(grid_path):
        # Центр текущей ячейки
        center_x = margin + gx * GRID_CELL_SIZE + GRID_CELL_SIZE // 2
        center_y = margin + gy * GRID_CELL_SIZE + GRID_CELL_SIZE // 2
        
        pixel_path.append((center_x, center_y))
        
        # Добавляем промежуточные точки для плавности (между текущей и следующей ячейкой)
        if i < len(grid_path) - 1:
            next_gx, next_gy = grid_path[i + 1]
            next_center_x = margin + next_gx * GRID_CELL_SIZE + GRID_CELL_SIZE // 2
            next_center_y = margin + next_gy * GRID_CELL_SIZE + GRID_CELL_SIZE // 2
            
            # Направление движения (только 90° углы!)
            if next_gx != gx:  # Горизонтальное движение
                step = 1 if next_gx > gx else -1
                for dx in range(step * 10, (next_center_x - center_x), step * 10):
                    pixel_path.append((center_x + dx, center_y))
            elif next_gy != gy:  # Вертикальное движение
                step = 1 if next_gy > gy else -1
                for dy in range(step * 10, (next_center_y - center_y), step * 10):
                    pixel_path.append((center_x, center_y + dy))
    
    return pixel_path


def generate_line_path():
    """
    Генерирует путь черной линии на трассе используя Гамильтонов путь.
    Гарантирует отсутствие самопересечений и строгие углы 90°.
    
    Returns:
        list: Список точек [(x, y), ...] пути линии в пиксельных координатах
    """
    print("Генерация Гамильтонова пути на сетке...")
    grid_path = generate_hamiltonian_path()
    print(f"  ✓ Базовый путь: {len(grid_path)} ячеек")
    
    print("Добавление случайных вариаций...")
    # Пока используем базовый путь без вариаций для гарантии работоспособности
    # varied_path = add_random_variations(grid_path, variation_prob=0.2)
    varied_path = grid_path
    print(f"  ✓ Итоговый путь: {len(varied_path)} ячеек")
    
    print("Конвертация в пиксельные координаты...")
    pixel_path = grid_to_pixel_path(varied_path)
    print(f"  ✓ Пиксельный путь: {len(pixel_path)} точек")
    
    return pixel_path




def draw_track_map(path):
    """
    Рисует карту трассы с черной линией на белом фоне.
    
    Args:
        path: Список точек пути
    
    Returns:
        PIL.Image: Изображение трассы
    """
    # Создаем белое поле
    track_image = Image.new('RGB', (TRACK_SIZE, TRACK_SIZE), (255, 255, 255))
    draw = ImageDraw.Draw(track_image)
    
    # Рисуем черную линию
    if len(path) > 1:
        draw.line(path, fill=(0, 0, 0), width=LINE_WIDTH, joint='curve')
    
    # Добавляем маркеры старта и финиша
    start_x, start_y = path[0]
    end_x, end_y = path[-1]
    
    # Стартовая точка (зеленый круг)
    marker_size = 40
    draw.ellipse([start_x - marker_size, start_y - marker_size, 
                  start_x + marker_size, start_y + marker_size], 
                 fill=(0, 255, 0), outline=(0, 128, 0), width=5)
    
    # Финишная точка (красный круг)
    draw.ellipse([end_x - marker_size, end_y - marker_size, 
                  end_x + marker_size, end_y + marker_size], 
                 fill=(255, 0, 0), outline=(128, 0, 0), width=5)
    
    return track_image


def extract_camera_frames(track_image, path, step_size=30):
    """
    Извлекает кадры камеры, двигаясь вдоль пути.
    
    Args:
        track_image: Полное изображение трассы
        path: Путь линии
        step_size: Шаг между кадрами (пиксели)
    
    Returns:
        list: Список кадров с метаданными
    """
    frames = []
    frame_id = 0
    
    # Интерполируем путь для плавного движения
    interpolated_path = []
    for i in range(len(path) - 1):
        x1, y1 = path[i]
        x2, y2 = path[i + 1]
        
        distance = math.sqrt((x2 - x1)**2 + (y2 - y1)**2)
        num_steps = max(int(distance / step_size), 1)
        
        for j in range(num_steps):
            t = j / num_steps
            x = x1 + (x2 - x1) * t
            y = y1 + (y2 - y1) * t
            
            # Вычисляем направление движения
            if i < len(path) - 1:
                dx = x2 - x1
                dy = y2 - y1
                angle = math.degrees(math.atan2(dy, dx))
            else:
                angle = 0
            
            interpolated_path.append((x, y, angle))
    
    # Добавляем последнюю точку
    if len(path) > 1:
        x1, y1 = path[-2]
        x2, y2 = path[-1]
        angle = math.degrees(math.atan2(y2 - y1, x2 - x1))
        interpolated_path.append((x2, y2, angle))
    
    print(f"📍 Интерполированный путь: {len(interpolated_path)} точек")
    
    # Извлекаем кадры
    for cx, cy, direction_angle in interpolated_path:
        # Камера находится в точке (cx, cy) и смотрит по направлению direction_angle
        # Нужно извлечь область И ПОВЕРНУТЬ её так, чтобы линия шла снизу вверх
        
        # Увеличенная область для поворота (чтобы после поворота не было обрезки)
        extract_size = max(CAMERA_WIDTH, CAMERA_HEIGHT) * 2
        
        left = int(cx - extract_size // 2)
        top = int(cy - extract_size // 2)
        right = left + extract_size
        bottom = top + extract_size
        
        # Проверяем границы
        if left < 0 or top < 0 or right > TRACK_SIZE or bottom > TRACK_SIZE:
            continue
        
        # Извлекаем увеличенный кадр
        large_frame = track_image.crop((left, top, right, bottom))
        
        # КЛЮЧЕВОЙ МОМЕНТ: Поворачиваем изображение так, чтобы направление движения
        # было "вверх" (т.е. линия идет от низа кадра к верху)
        # direction_angle: 0° = вправо, 90° = вверх, 180° = влево, 270° = вниз
        # Нам нужно повернуть на (90 - direction_angle), чтобы направление стало вверх
        rotation_angle = 90 - direction_angle
        rotated_frame = large_frame.rotate(rotation_angle, expand=False, fillcolor=(255, 255, 255))
        
        # Вырезаем центральную часть нужного размера
        center_x = rotated_frame.width // 2
        center_y = rotated_frame.height // 2
        
        crop_left = center_x - CAMERA_WIDTH // 2
        crop_top = center_y - CAMERA_HEIGHT // 2
        crop_right = crop_left + CAMERA_WIDTH
        crop_bottom = crop_top + CAMERA_HEIGHT
        
        frame_img = rotated_frame.crop((crop_left, crop_top, crop_right, crop_bottom))
        
        # Анализируем кадр для определения категории
        expected_action = analyze_frame(frame_img)
        
        # Сохраняем кадр
        frame_path = OUTPUT_DIR / 'frames' / f'frame_{frame_id:04d}.jpg'
        frame_path.parent.mkdir(parents=True, exist_ok=True)
        frame_img.save(frame_path, quality=90)
        
        frames.append({
            'frame_id': frame_id,
            'position': {'x': int(cx), 'y': int(cy)},
            'direction_angle': float(direction_angle),
            'image': str(frame_path.relative_to(OUTPUT_DIR.parent)),
            'expected_action': expected_action
        })
        
        frame_id += 1
    
    return frames


def analyze_frame(frame_img):
    """
    Анализирует кадр для определения ожидаемого действия.
    
    Args:
        frame_img: PIL.Image кадра
    
    Returns:
        str: 'straight', 'left', 'right', или 'terminate'
    """
    # Конвертируем в numpy array
    img_array = np.array(frame_img.convert('L'))  # Grayscale
    
    # Ищем черные пиксели (линия)
    threshold = 128
    line_pixels = img_array < threshold
    
    if np.sum(line_pixels) < 50:
        return 'terminate'  # Линия не найдена или слишком мало пикселей
    
    # Разделяем на три горизонтальные секции (верх, середина, низ)
    height = img_array.shape[0]
    top_section = line_pixels[:height//3, :]
    mid_section = line_pixels[height//3:2*height//3, :]
    bottom_section = line_pixels[2*height//3:, :]
    
    # Вычисляем центр масс линии в каждой секции
    def center_of_mass(section):
        if np.sum(section) == 0:
            return None
        y_coords, x_coords = np.where(section)
        return np.mean(x_coords) if len(x_coords) > 0 else None
    
    top_center = center_of_mass(top_section)
    mid_center = center_of_mass(mid_section)
    bottom_center = center_of_mass(bottom_section)
    
    # Определяем тренд движения линии
    width = img_array.shape[1]
    center = width // 2
    
    # Если есть центры в двух секциях, вычисляем тренд
    if bottom_center is not None and top_center is not None:
        trend = (top_center - bottom_center) / width
        
        if abs(trend) < 0.15:
            return 'straight'
        elif trend > 0:
            return 'left'  # Линия уходит влево (вверх кадра)
        else:
            return 'right'  # Линия уходит вправо (вверх кадра)
    
    # Если есть только нижний центр, смотрим на положение
    if bottom_center is not None:
        offset = (bottom_center - center) / width
        
        if abs(offset) < 0.2:
            return 'straight'
        elif offset < 0:
            return 'left'
        else:
            return 'right'
    
    return 'straight'


def save_track_map(frames, track_image):
    """Сохраняет карту трассы и метаданные."""
    OUTPUT_DIR.mkdir(exist_ok=True)
    
    # Сохраняем полное изображение трассы
    track_image.save(TRACK_MAP_IMAGE)
    print(f"✅ Полная карта трассы: {TRACK_MAP_IMAGE}")
    
    # Создаем превью (уменьшенное изображение)
    preview = track_image.copy()
    preview.thumbnail((800, 800))
    preview.save(TRACK_PREVIEW_IMAGE)
    print(f"✅ Превью карты: {TRACK_PREVIEW_IMAGE}")
    
    # Сохраняем метаданные
    track_map = {
        'version': '2.0',
        'description': 'Карта трассы с сгенерированной черной линией на белом фоне',
        'track_size': TRACK_SIZE,
        'line_width': LINE_WIDTH,
        'camera_resolution': {'width': CAMERA_WIDTH, 'height': CAMERA_HEIGHT},
        'total_frames': len(frames),
        'frames': frames,
        'statistics': {
            'straight': sum(1 for f in frames if f['expected_action'] == 'straight'),
            'left': sum(1 for f in frames if f['expected_action'] == 'left'),
            'right': sum(1 for f in frames if f['expected_action'] == 'right'),
            'terminate': sum(1 for f in frames if f['expected_action'] == 'terminate')
        }
    }
    
    with open(TRACK_MAP_FILE, 'w', encoding='utf-8') as f:
        json.dump(track_map, f, indent=2, ensure_ascii=False)
    
    print(f"\n✅ Метаданные сохранены: {TRACK_MAP_FILE}")
    print(f"📊 Статистика кадров:")
    print(f"  - Всего кадров: {track_map['total_frames']}")
    print(f"  - STRAIGHT: {track_map['statistics']['straight']}")
    print(f"  - LEFT: {track_map['statistics']['left']}")
    print(f"  - RIGHT: {track_map['statistics']['right']}")
    print(f"  - TERMINATE: {track_map['statistics']['terminate']}")
    
    return track_map


if __name__ == '__main__':
    print("🏁 Генерация карты трассы с черной линией...\n")
    
    print(f"📐 Параметры трассы:")
    print(f"  - Размер поля: {TRACK_SIZE}×{TRACK_SIZE} пикселей")
    print(f"  - Ширина линии: {LINE_WIDTH} пикселей (~20 мм)")
    print(f"  - Разрешение камеры: {CAMERA_WIDTH}×{CAMERA_HEIGHT}\n")
    
    # Генерируем путь линии
    print("🎨 Генерация пути линии...")
    path = generate_line_path()
    print(f"✅ Путь создан: {len(path)} ключевых точек")
    
    # Рисуем карту трассы
    print("\n🖼️ Рисование карты трассы...")
    track_image = draw_track_map(path)
    print(f"✅ Карта нарисована")
    
    # Извлекаем кадры камеры
    print("\n📹 Извлечение кадров камеры вдоль пути...")
    frames = extract_camera_frames(track_image, path, step_size=25)
    print(f"✅ Извлечено кадров: {len(frames)}")
    
    # Сохраняем
    print("\n💾 Сохранение...")
    track_map = save_track_map(frames, track_image)
    
    print("\n✅ Готово! Используйте track_simulator.py для прохождения трассы.")
    print(f"   Полная карта: {TRACK_MAP_IMAGE}")
    print(f"   Превью: {TRACK_PREVIEW_IMAGE}")
    print(f"   Метаданные: {TRACK_MAP_FILE}")
