#!/usr/bin/env python3
"""
Улучшенный алгоритм детекции линии с детекцией границ

Алгоритм:
1. Сравнивает калибровочные сканлайны с текущим кадром
2. Если разница в пределах погрешности -> белое поле (нет линии)
3. Если разница значительная -> есть линия
4. Для каждой сканирующей линии находит границы линии (минимальная ширина)
5. Находит центры линии на каждой сканлайне
6. По центрам определяет направление (прямо/влево/вправо/обрыв)

Цель: 100% точность на тестовом датасете
"""

import numpy as np
import cv2
import glob
import os
import sys
from pathlib import Path
import time
import matplotlib.pyplot as plt
import matplotlib.patches as patches

# Константы
ROI_Y_START = 11
ROI_Y_END = 102
ROI_X_START = 16
ROI_X_END = 144

# Минимальная ширина линии (в пикселях)
MIN_LINE_WIDTH = 5
MAX_LINE_WIDTH = 60  # Максимальная ширина линии

# Порог для детекции (разница от фона)
DETECTION_THRESHOLD = 30

# Порог для определения "белое поле" (среднее отклонение от калибровки)
WHITE_FIELD_THRESHOLD = 10

# Количество сканирующих линий
NUM_SCAN_LINES = 12

def compute_scan_lines(roi_y_start, roi_y_end, num_lines):
    """Вычислить Y-координаты сканирующих линий"""
    return np.linspace(roi_y_start, roi_y_end - 1, num_lines, dtype=int)

def detect_line_edges_on_scanline(white_bg_row, current_row, threshold, min_width, max_width):
    """
    Детекция границ линии на одной сканирующей линии
    
    Возвращает список сегментов линии: [(left, right), ...]
    """
    diff = white_bg_row.astype(np.int16) - current_row.astype(np.int16)
    mask = (diff > threshold).astype(np.uint8)
    
    # Находим связные компоненты (сегменты линии)
    segments = []
    in_segment = False
    segment_start = 0
    
    for i in range(len(mask)):
        if mask[i] == 1 and not in_segment:
            # Начало сегмента
            in_segment = True
            segment_start = i
        elif mask[i] == 0 and in_segment:
            # Конец сегмента
            in_segment = False
            width = i - segment_start
            if min_width <= width <= max_width:
                segments.append((segment_start, i - 1))
        
    # Проверить последний сегмент
    if in_segment:
        width = len(mask) - segment_start
        if min_width <= width <= max_width:
            segments.append((segment_start, len(mask) - 1))
    
    return segments

def compute_segment_center(left, right):
    """Вычислить центр сегмента"""
    return (left + right) / 2.0

def classify_scenario(centers, confidences, roi_width, segments_per_line):
    """
    Классификация сценария на основе центров линии на сканлайнах
    
    Returns: (scenario, position, confidence)
        scenario: "white_field", "straight", "left", "right", "terminate"
        position: normalized position [-1, 1]
        confidence: confidence score [0, 1]
    """
    if len(centers) == 0:
        return "white_field", 0.0, 0.0
    
    # Средняя уверенность
    avg_confidence = np.mean(confidences)
    
    # Если уверенность очень низкая -> белое поле
    if avg_confidence < 0.25:
        return "white_field", 0.0, avg_confidence
    
    # Вычислить среднюю позицию
    avg_position = np.mean(centers)
    
    # Нормализовать к [-1, 1]
    normalized_position = (avg_position - roi_width / 2) / (roi_width / 2)
    
    # Детекция окончания линии (совсем мало детектированных сканлайнов)
    detection_ratio = len(centers) / NUM_SCAN_LINES
    if detection_ratio < 0.4:
        return "terminate", normalized_position, avg_confidence
    
    # СНАЧАЛА детекция поворота (приоритет над terminate)
    # Детекция поворота по изменению позиции от НИЖНИХ к ВЕРХНИМ сканлайнам
    # В перспективе камеры:
    # - нижние сканлайны (ближе к роботу) - начало массива centers
    # - верхние сканлайны (дальше от робота) - конец массива centers
    position_drift = 0.0
    if len(centers) >= 4:
        # Берем первую и последнюю треть
        n_third = max(len(centers) // 3, 1)
        bottom_avg = np.mean(centers[:n_third])   # Ближние сканлайны
        top_avg = np.mean(centers[-n_third:])     # Дальние сканлайны
        
        # Дрейф = изменение позиции от ближних к дальним
        # Если дрейф > 0: линия уходит ВПРАВО (от робота) -> робот поворачивает ВЛЕВО
        # Если дрейф < 0: линия уходит ВЛЕВО (от робота) -> робот поворачивает ВПРАВО
        position_drift = (top_avg - bottom_avg) / roi_width
        
        # Пороги для детекции поворота
        if position_drift > 0.08:
            return "left", normalized_position, avg_confidence
        elif position_drift < -0.08:
            return "right", normalized_position, avg_confidence
    
    # ПОТОМ детекция окончания линии/T-пересечения
    # Считаем сколько сканлайнов детектировано в верхней и нижней половине
    n_half = NUM_SCAN_LINES // 2
    top_detected = sum(1 for segs in segments_per_line[:n_half] if len(segs) > 0)
    bottom_detected = sum(1 for segs in segments_per_line[n_half:] if len(segs) > 0)
    
    # Если нижние детектируются намного лучше верхних И нет поворота -> terminate
    detection_diff = bottom_detected - top_detected
    if detection_diff >= 3 and bottom_detected >= 5 and abs(position_drift) < 0.08:
        return "terminate", normalized_position, avg_confidence
    
    # Если нет явного поворота -> прямо
    return "straight", normalized_position, avg_confidence

def process_image(white_bg, current_img, scan_lines, threshold, min_width, max_width):
    """
    Обработка одного изображения
    
    Returns: (scenario, position, confidence, centers, segments_per_line)
    """
    roi_width = ROI_X_END - ROI_X_START
    
    centers = []
    confidences = []
    segments_per_line = []
    
    for y in scan_lines:
        # Извлечь строки
        white_bg_row = white_bg[y, ROI_X_START:ROI_X_END]
        current_row = current_img[y, ROI_X_START:ROI_X_END]
        
        # Детекция границ линии
        segments = detect_line_edges_on_scanline(
            white_bg_row, current_row, threshold, min_width, max_width
        )
        
        segments_per_line.append(segments)
        
        # Если нашли сегменты
        if len(segments) > 0:
            # Берем самый большой сегмент (основная линия)
            largest_segment = max(segments, key=lambda s: s[1] - s[0])
            center = compute_segment_center(largest_segment[0], largest_segment[1])
            centers.append(center)
            
            # Уверенность = ширина сегмента / максимальная ширина
            width = largest_segment[1] - largest_segment[0] + 1
            confidence = min(width / max_width, 1.0)
            confidences.append(confidence)
    
    # Классификация
    scenario, position, confidence = classify_scenario(centers, confidences, roi_width, segments_per_line)
    
    return scenario, position, confidence, centers, segments_per_line

def load_image_grayscale(path):
    """Загрузить изображение в градациях серого"""
    img = cv2.imread(path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        raise ValueError(f"Не удалось загрузить изображение: {path}")
    return img

def test_on_dataset(white_bg_images, test_images, expected_scenario, scan_lines):
    """
    Тестирование на датасете
    
    Returns: (accuracy, results)
    """
    # Вычислить средний фон
    white_bg = np.mean([load_image_grayscale(p) for p in white_bg_images], axis=0).astype(np.uint8)
    
    results = []
    correct = 0
    total = len(test_images)
    
    for img_path in test_images:
        current_img = load_image_grayscale(img_path)
        
        scenario, position, confidence, centers, segments = process_image(
            white_bg, current_img, scan_lines, 
            DETECTION_THRESHOLD, MIN_LINE_WIDTH, MAX_LINE_WIDTH
        )
        
        is_correct = (scenario == expected_scenario)
        if is_correct:
            correct += 1
        
        results.append({
            'path': img_path,
            'scenario': scenario,
            'expected': expected_scenario,
            'position': position,
            'confidence': confidence,
            'centers': centers,
            'segments': segments,
            'correct': is_correct
        })
    
    accuracy = correct / total if total > 0 else 0
    return accuracy, results

def visualize_results(white_bg, test_images, results, output_path, max_examples=3):
    """
    Визуализация результатов
    """
    n_examples = min(len(test_images), max_examples)
    
    fig, axes = plt.subplots(n_examples, 1, figsize=(12, 4 * n_examples))
    if n_examples == 1:
        axes = [axes]
    
    scan_lines = compute_scan_lines(ROI_Y_START, ROI_Y_END, NUM_SCAN_LINES)
    
    for idx in range(n_examples):
        img_path = test_images[idx]
        result = results[idx]
        
        current_img = load_image_grayscale(img_path)
        
        # Конвертировать в RGB для визуализации
        img_rgb = cv2.cvtColor(current_img, cv2.COLOR_GRAY2RGB)
        
        # Нарисовать ROI
        cv2.rectangle(img_rgb, (ROI_X_START, ROI_Y_START), (ROI_X_END, ROI_Y_END), (0, 255, 0), 1)
        
        # Нарисовать сканирующие линии
        for y in scan_lines:
            cv2.line(img_rgb, (ROI_X_START, y), (ROI_X_END, y), (255, 255, 0), 1)
        
        # Нарисовать детектированные сегменты
        for i, (y, segments) in enumerate(zip(scan_lines, result['segments'])):
            for seg_left, seg_right in segments:
                x_left = ROI_X_START + seg_left
                x_right = ROI_X_START + seg_right
                cv2.line(img_rgb, (x_left, y), (x_right, y), (255, 0, 0), 2)
        
        # Нарисовать центры
        for i, center in enumerate(result['centers']):
            y = scan_lines[i] if i < len(scan_lines) else scan_lines[-1]
            x = int(ROI_X_START + center)
            cv2.circle(img_rgb, (x, y), 3, (0, 0, 255), -1)
        
        # Показать изображение
        axes[idx].imshow(img_rgb)
        axes[idx].axis('off')
        
        # Заголовок
        status = "✓" if result['correct'] else "✗"
        title = f"{status} {os.path.basename(img_path)}\n"
        title += f"Детектировано: {result['scenario']} | Ожидалось: {result['expected']}\n"
        title += f"Позиция: {result['position']:.3f} | Уверенность: {result['confidence']:.3f} | Центров: {len(result['centers'])}"
        axes[idx].set_title(title, fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"   Визуализация сохранена: {output_path}")

def main():
    print("=" * 80)
    print("🧪 ТЕСТИРОВАНИЕ УЛУЧШЕННОГО АЛГОРИТМА С ДЕТЕКЦИЕЙ ГРАНИЦ ЛИНИИ")
    print("=" * 80)
    
    # Параметры
    print("\n⚙️  ПАРАМЕТРЫ АЛГОРИТМА:")
    print(f"   ROI: Y[{ROI_Y_START}:{ROI_Y_END}], X[{ROI_X_START}:{ROI_X_END}]")
    print(f"   Сканирующих линий: {NUM_SCAN_LINES}")
    print(f"   Минимальная ширина линии: {MIN_LINE_WIDTH} пикселей")
    print(f"   Максимальная ширина линии: {MAX_LINE_WIDTH} пикселей")
    print(f"   Порог детекции: {DETECTION_THRESHOLD}")
    
    # Путь к данным
    script_dir = Path(__file__).parent
    data_dir = script_dir.parent / "data"
    output_dir = script_dir / "output"
    output_dir.mkdir(exist_ok=True)
    
    # Загрузка калибровочных изображений (белое поле)
    calib_pattern = str(data_dir / "img_calibration" / "calibration_*.jpg")
    calib_images = sorted(glob.glob(calib_pattern))
    
    if len(calib_images) == 0:
        print(f"\n❌ ОШИБКА: Не найдены калибровочные изображения: {calib_pattern}")
        return 1
    
    print(f"\n📊 КАЛИБРОВКА:")
    print(f"   Загружено {len(calib_images)} изображений белого поля")
    
    # Вычислить сканирующие линии
    scan_lines = compute_scan_lines(ROI_Y_START, ROI_Y_END, NUM_SCAN_LINES)
    print(f"   Сканирующие линии (Y): {scan_lines.tolist()}")
    
    # Тестовые датасеты
    datasets = [
        ("straight", "img_straight", "Прямая линия"),
        ("left", "img_left", "Поворот влево"),
        ("right", "img_right", "Поворот вправо"),
        ("terminate", "img_terminate", "Окончание линии"),
    ]
    
    print("\n" + "=" * 80)
    print("📈 РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ")
    print("=" * 80)
    
    total_images = 0
    total_correct = 0
    all_results = {}
    
    for scenario, folder, description in datasets:
        pattern = str(data_dir / folder / "*.jpg")
        test_images = sorted(glob.glob(pattern))
        
        if len(test_images) == 0:
            print(f"\n⚠️  {description}: Изображения не найдены")
            continue
        
        print(f"\n📂 {description}:")
        print(f"   Изображений: {len(test_images)}")
        
        # Тестирование
        accuracy, results = test_on_dataset(calib_images, test_images, scenario, scan_lines)
        
        # Статистика
        scenarios_detected = {}
        for r in results:
            detected = r['scenario']
            scenarios_detected[detected] = scenarios_detected.get(detected, 0) + 1
        
        print(f"   Точность: {accuracy * 100:.1f}%")
        print(f"   Детектировано как:")
        for det_scenario, count in sorted(scenarios_detected.items()):
            pct = count / len(test_images) * 100
            symbol = "✓" if det_scenario == scenario else "✗"
            print(f"      {symbol} {det_scenario}: {count} ({pct:.1f}%)")
        
        # Сохранить результаты
        all_results[scenario] = {
            'accuracy': accuracy,
            'results': results,
            'images': test_images
        }
        
        total_images += len(test_images)
        total_correct += int(accuracy * len(test_images))
        
        # Визуализация
        output_path = output_dir / f"edge_detection_{scenario}.png"
        visualize_results(
            np.mean([load_image_grayscale(p) for p in calib_images], axis=0).astype(np.uint8),
            test_images, results, output_path, max_examples=3
        )
    
    # Общая статистика
    print("\n" + "=" * 80)
    print("📊 ОБЩАЯ СТАТИСТИКА")
    print("=" * 80)
    print(f"   Всего изображений: {total_images}")
    print(f"   Правильно классифицировано: {total_correct}")
    print(f"   Общая точность: {total_correct / total_images * 100:.1f}%")
    
    # Анализ ошибок
    print("\n" + "=" * 80)
    print("🔍 АНАЛИЗ ОШИБОК")
    print("=" * 80)
    
    for scenario, data in all_results.items():
        errors = [r for r in data['results'] if not r['correct']]
        if len(errors) > 0:
            print(f"\n{scenario.upper()}:")
            for err in errors[:5]:  # Показать первые 5 ошибок
                print(f"   ✗ {os.path.basename(err['path'])}")
                print(f"      Детектировано: {err['scenario']} (позиция: {err['position']:.3f}, уверенность: {err['confidence']:.3f})")
    
    print("\n" + "=" * 80)
    print("✅ ТЕСТИРОВАНИЕ ЗАВЕРШЕНО")
    print("=" * 80)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
