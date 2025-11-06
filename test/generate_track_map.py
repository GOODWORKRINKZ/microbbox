#!/usr/bin/env python3
"""
Генератор карты трассы из датасета изображений.

Создает:
1. JSON-файл с метаданными трассы
2. Визуальную карту трассы (сшитые изображения в одну картинку)

Функции:
- Последовательность изображений (путь по трассе)
- Правильные ответы для каждого кадра (straight/left/right/terminate)
- Сшивка изображений в единую визуальную карту
"""

import os
import json
import random
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import numpy as np

# Константы
DATA_DIR = Path(__file__).parent.parent / 'data'
OUTPUT_DIR = Path(__file__).parent / 'output'
TRACK_MAP_FILE = OUTPUT_DIR / 'track_map.json'
TRACK_MAP_IMAGE = OUTPUT_DIR / 'track_map_stitched.png'

# Параметры сшивки
IMAGE_WIDTH = 160
IMAGE_HEIGHT = 120
OVERLAP_ROWS = 20  # Количество строк для плавного перехода

def load_images_from_category(category_name):
    """Загружает список изображений из категории."""
    category_dir = DATA_DIR / category_name
    if not category_dir.exists():
        return []
    
    images = []
    for img_file in sorted(category_dir.glob('*.jpg')) + sorted(category_dir.glob('*.jpeg')):
        images.append({
            'path': str(img_file.relative_to(DATA_DIR.parent)),
            'filename': img_file.name,
            'category': category_name
        })
    return images


def generate_track_sequence():
    """
    Генерирует реалистичную последовательность кадров трассы.
    
    Логика:
    - Прямая линия: может перейти в любое направление
    - Поворот: после поворота обычно идет прямая линия
    - Terminate: конечная точка, после нее новый старт
    """
    
    # Загружаем все изображения по категориям
    straight_images = load_images_from_category('img_straight')
    left_images = load_images_from_category('img_left')
    right_images = load_images_from_category('img_right')
    terminate_images = load_images_from_category('img_terminate')
    
    print(f"📊 Загружено изображений:")
    print(f"  - STRAIGHT: {len(straight_images)}")
    print(f"  - LEFT: {len(left_images)}")
    print(f"  - RIGHT: {len(right_images)}")
    print(f"  - TERMINATE: {len(terminate_images)}")
    
    # Создаем последовательность кадров
    track_sequence = []
    current_state = 'straight'
    
    # Параметры генерации
    MIN_STRAIGHT_BEFORE_TURN = 2
    MAX_STRAIGHT_BEFORE_TURN = 5
    MIN_TURN_FRAMES = 1
    MAX_TURN_FRAMES = 3
    
    # Копии для случайного выбора без повторений
    available = {
        'straight': straight_images.copy(),
        'left': left_images.copy(),
        'right': right_images.copy(),
        'terminate': terminate_images.copy()
    }
    
    def get_random_image(category):
        """Получает случайное изображение из категории без повторений."""
        if not available[category]:
            # Если закончились, перезагружаем
            available[category] = load_images_from_category(f'img_{category}').copy()
            random.shuffle(available[category])
        
        return available[category].pop()
    
    # Генерируем трассу
    frame_id = 0
    segments_count = 0
    max_segments = 20  # Максимум сегментов на трассе
    
    while segments_count < max_segments:
        if current_state == 'straight':
            # Прямой участок
            num_frames = random.randint(MIN_STRAIGHT_BEFORE_TURN, MAX_STRAIGHT_BEFORE_TURN)
            for _ in range(num_frames):
                img = get_random_image('straight')
                track_sequence.append({
                    'frame_id': frame_id,
                    'image': img['path'],
                    'expected_action': 'straight',
                    'category': 'img_straight',
                    'segment_id': segments_count
                })
                frame_id += 1
            
            # Решаем, что будет дальше
            next_options = ['left', 'right', 'terminate', 'straight']
            weights = [0.3, 0.3, 0.1, 0.3]  # Веса вероятностей
            current_state = random.choices(next_options, weights=weights)[0]
            segments_count += 1
            
        elif current_state in ['left', 'right']:
            # Поворот
            num_frames = random.randint(MIN_TURN_FRAMES, MAX_TURN_FRAMES)
            for _ in range(num_frames):
                img = get_random_image(current_state)
                track_sequence.append({
                    'frame_id': frame_id,
                    'image': img['path'],
                    'expected_action': current_state,
                    'category': f'img_{current_state}',
                    'segment_id': segments_count
                })
                frame_id += 1
            
            # После поворота обычно идет прямая
            current_state = 'straight'
            segments_count += 1
            
        elif current_state == 'terminate':
            # T-пересечение или конец линии
            img = get_random_image('terminate')
            track_sequence.append({
                'frame_id': frame_id,
                'image': img['path'],
                'expected_action': 'terminate',
                'category': 'img_terminate',
                'segment_id': segments_count
            })
            frame_id += 1
            
            # После terminate начинаем новый сегмент с прямой
            current_state = 'straight'
            segments_count += 1
    
    return track_sequence


def save_track_map(track_sequence):
    """Сохраняет карту трассы в JSON."""
    OUTPUT_DIR.mkdir(exist_ok=True)
    
    track_map = {
        'version': '1.0',
        'description': 'Карта трассы для тестирования алгоритма распознавания линии',
        'total_frames': len(track_sequence),
        'frames': track_sequence,
        'statistics': {
            'straight': sum(1 for f in track_sequence if f['expected_action'] == 'straight'),
            'left': sum(1 for f in track_sequence if f['expected_action'] == 'left'),
            'right': sum(1 for f in track_sequence if f['expected_action'] == 'right'),
            'terminate': sum(1 for f in track_sequence if f['expected_action'] == 'terminate')
        }
    }
    
    with open(TRACK_MAP_FILE, 'w', encoding='utf-8') as f:
        json.dump(track_map, f, indent=2, ensure_ascii=False)
    
    print(f"\n✅ Карта трассы сохранена: {TRACK_MAP_FILE}")
    print(f"📊 Статистика:")
    print(f"  - Всего кадров: {track_map['total_frames']}")
    print(f"  - STRAIGHT: {track_map['statistics']['straight']}")
    print(f"  - LEFT: {track_map['statistics']['left']}")
    print(f"  - RIGHT: {track_map['statistics']['right']}")
    print(f"  - TERMINATE: {track_map['statistics']['terminate']}")
    
    return track_map


def stitch_images_smooth(image_paths, labels):
    """
    Сшивает изображения в одну длинную картинку с плавными переходами.
    
    Args:
        image_paths: список путей к изображениям
        labels: список меток (straight/left/right/terminate)
    
    Returns:
        PIL.Image: сшитое изображение
    """
    if not image_paths:
        return None
    
    # Загружаем все изображения
    images = []
    for img_path in image_paths:
        try:
            img = Image.open(img_path).convert('RGB')
            images.append(img)
        except Exception as e:
            print(f"⚠️ Не удалось загрузить {img_path}: {e}")
            # Создаем черное изображение вместо отсутствующего
            images.append(Image.new('RGB', (IMAGE_WIDTH, IMAGE_HEIGHT), (0, 0, 0)))
    
    if not images:
        return None
    
    # Вычисляем размеры итогового изображения
    # Каждое изображение добавляет (HEIGHT - OVERLAP_ROWS) высоты
    total_height = IMAGE_HEIGHT + (len(images) - 1) * (IMAGE_HEIGHT - OVERLAP_ROWS)
    total_width = IMAGE_WIDTH
    
    # Создаем результирующее изображение
    result = Image.new('RGB', (total_width, total_height), (255, 255, 255))
    
    # Цвета для меток
    label_colors = {
        'straight': (0, 255, 0),      # Зеленый
        'left': (0, 100, 255),         # Синий
        'right': (255, 150, 0),        # Оранжевый
        'terminate': (255, 0, 0)       # Красный
    }
    
    # Сшиваем изображения с плавными переходами
    current_y = 0
    
    for i, (img, label) in enumerate(zip(images, labels)):
        if i == 0:
            # Первое изображение вставляем целиком
            result.paste(img, (0, 0))
            current_y = IMAGE_HEIGHT - OVERLAP_ROWS
        else:
            # Для остальных делаем плавный переход
            # Берем нижнюю часть предыдущего изображения
            prev_img = images[i - 1]
            prev_bottom = np.array(prev_img.crop((0, IMAGE_HEIGHT - OVERLAP_ROWS, IMAGE_WIDTH, IMAGE_HEIGHT)))
            
            # Берем верхнюю часть текущего изображения
            curr_top = np.array(img.crop((0, 0, IMAGE_WIDTH, OVERLAP_ROWS)))
            
            # Создаем плавный переход
            blended = np.zeros_like(prev_bottom)
            for row in range(OVERLAP_ROWS):
                alpha = row / OVERLAP_ROWS  # От 0 до 1
                blended[row] = (prev_bottom[row] * (1 - alpha) + curr_top[row] * alpha).astype(np.uint8)
            
            # Вставляем переход
            blend_img = Image.fromarray(blended)
            result.paste(blend_img, (0, current_y))
            
            # Вставляем остальную часть текущего изображения
            curr_rest = img.crop((0, OVERLAP_ROWS, IMAGE_WIDTH, IMAGE_HEIGHT))
            result.paste(curr_rest, (0, current_y + OVERLAP_ROWS))
            
            current_y += IMAGE_HEIGHT - OVERLAP_ROWS
        
        # Добавляем цветную метку сбоку
        draw = ImageDraw.Draw(result)
        y_pos = current_y - (IMAGE_HEIGHT - OVERLAP_ROWS) // 2
        color = label_colors.get(label, (128, 128, 128))
        
        # Рисуем цветную полоску
        draw.rectangle([IMAGE_WIDTH - 10, max(0, y_pos - 30), IMAGE_WIDTH, y_pos + 30], 
                      fill=color, outline=(0, 0, 0), width=1)
    
    return result


def visualize_track_map(track_map):
    """Создает визуальную карту трассы путем сшивки изображений."""
    frames = track_map['frames']
    
    print(f"\n🖼️ Создание визуальной карты трассы (сшивка {len(frames)} изображений)...")
    
    # Собираем пути к изображениям и метки
    image_paths = []
    labels = []
    
    for frame in frames:
        img_path = Path(__file__).parent.parent / frame['image']
        image_paths.append(img_path)
        labels.append(frame['expected_action'])
    
    # Сшиваем изображения
    stitched_image = stitch_images_smooth(image_paths, labels)
    
    if stitched_image:
        # Добавляем легенду
        legend_height = 80
        final_image = Image.new('RGB', 
                                (stitched_image.width + 200, stitched_image.height + legend_height),
                                (255, 255, 255))
        final_image.paste(stitched_image, (0, legend_height))
        
        # Рисуем заголовок и легенду
        draw = ImageDraw.Draw(final_image)
        
        # Заголовок
        try:
            font_large = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 20)
            font_small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14)
        except:
            font_large = ImageFont.load_default()
            font_small = ImageFont.load_default()
        
        draw.text((10, 10), f"Карта трассы: {len(frames)} кадров", fill=(0, 0, 0), font=font_large)
        
        # Легенда
        legend_items = [
            ("STRAIGHT", (0, 255, 0)),
            ("LEFT", (0, 100, 255)),
            ("RIGHT", (255, 150, 0)),
            ("TERMINATE", (255, 0, 0))
        ]
        
        x_start = 10
        y_legend = 45
        for label, color in legend_items:
            draw.rectangle([x_start, y_legend, x_start + 20, y_legend + 20], 
                          fill=color, outline=(0, 0, 0), width=1)
            draw.text((x_start + 30, y_legend + 3), label, fill=(0, 0, 0), font=font_small)
            x_start += 150
        
        # Статистика справа
        stats_x = stitched_image.width + 10
        draw.text((stats_x, legend_height + 20), "Статистика:", fill=(0, 0, 0), font=font_large)
        
        stats_text = [
            f"Всего: {len(frames)}",
            f"STRAIGHT: {track_map['statistics']['straight']}",
            f"LEFT: {track_map['statistics']['left']}",
            f"RIGHT: {track_map['statistics']['right']}",
            f"TERMINATE: {track_map['statistics']['terminate']}"
        ]
        
        y_pos = legend_height + 50
        for text in stats_text:
            draw.text((stats_x, y_pos), text, fill=(0, 0, 0), font=font_small)
            y_pos += 25
        
        # Сохраняем
        final_image.save(TRACK_MAP_IMAGE)
        print(f"✅ Визуальная карта сохранена: {TRACK_MAP_IMAGE}")
        print(f"   Размер: {final_image.width}x{final_image.height} пикселей")
    else:
        print(f"⚠️ Не удалось создать визуальную карту")


if __name__ == '__main__':
    print("🏁 Генерация карты трассы из датасета...\n")
    
    # Генерируем последовательность
    track_sequence = generate_track_sequence()
    
    # Сохраняем карту
    track_map = save_track_map(track_sequence)
    
    # Визуализация
    try:
        visualize_track_map(track_map)
    except Exception as e:
        print(f"⚠️ Не удалось создать визуализацию: {e}")
    
    print("\n✅ Готово! Используйте track_simulator.py для прохождения трассы.")
