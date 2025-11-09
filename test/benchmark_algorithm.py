#!/usr/bin/env python3
"""
Бенчмарк алгоритма распознавания линии.

Измеряет:
1. Время выполнения алгоритма на Python (для сравнения)
2. Количество операций (для оценки на ESP32)
3. Потребление памяти
4. Прогноз производительности на ESP32

ESP32 характеристики:
- CPU: Dual-core Xtensa LX6 @ 240 MHz
- Python в 50-100 раз медленнее C++
- Операции с массивами: ~100 раз медленнее чем C++
"""

import sys
import time
import os
from pathlib import Path
import numpy as np
from PIL import Image

# Добавляем путь к модулю с алгоритмом
sys.path.insert(0, str(Path(__file__).parent))

# Константы из алгоритма
LINE_CAMERA_WIDTH = 160
LINE_CAMERA_HEIGHT = 120
LINE_THRESHOLD = 128


def count_operations_simple_algorithm(width=160, height=120):
    """
    Подсчет операций для ПРОСТОГО алгоритма (1 сканирующая линия).
    Это baseline для сравнения.
    
    Операции:
    1. Сканирование одной горизонтальной линии: width итераций
    2. Каждая итерация: 1 сравнение, 1 сложение (если pixel > threshold)
    3. Финальное деление для нормализации
    """
    # Сканирование линии
    scan_operations = width * 2  # 1 чтение пикселя + 1 сравнение
    
    # Подсчет суммы (в среднем половина пикселей проходит порог)
    sum_operations = (width // 2) * 2  # 1 сложение + 1 инкремент счетчика
    
    # Нормализация
    normalize_operations = 3  # деление, умножение, вычитание
    
    total = scan_operations + sum_operations + normalize_operations
    
    return {
        'algorithm': 'Simple (1 scan line)',
        'operations': total,
        'memory_bytes': width * height,  # Размер кадра
        'description': f'{width} пикселей сканирования, базовая нормализация'
    }


def count_operations_complex_algorithm(width=160, height=120):
    """
    Подсчет операций для СЛОЖНОГО алгоритма (4×4 сканирующие линии + тренд).
    Текущий алгоритм в LinerRobot.
    
    Операции:
    1. 4 горизонтальных сканирования (25%, 50%, 75%, 90% высоты)
    2. 4 вертикальных сканирования (20%, 40%, 60%, 80% ширины)
    3. Вычисление тренда между сканами
    4. Взвешенное комбинирование позиции и тренда
    5. Детекция T-пересечений
    """
    # 4 горизонтальных скана
    h_scan_ops = 4 * width * 2  # 4 линии × width пикселей × (чтение + сравнение)
    h_sum_ops = 4 * (width // 2) * 2  # подсчет сумм для каждого скана
    h_normalize = 4 * 3  # нормализация каждого результата
    
    # 4 вертикальных скана
    v_scan_ops = 4 * height * 2  # 4 линии × height пикселей
    v_sum_ops = 4 * (height // 2) * 2
    v_normalize = 4 * 3
    
    # Вычисление трендов (между всеми парами горизонтальных сканов)
    # 4 скана = 6 возможных пар (комбинации C(4,2))
    trend_ops = 6 * 5  # для каждой пары: вычитание, деление, abs, max
    
    # Взвешенное комбинирование (базовая позиция × вес + тренд × вес)
    combine_ops = 4  # 2 умножения + 1 сложение + 1 присваивание
    
    # Проверка T-пересечения (сравнение ширины с порогом)
    t_junction_ops = 2  # деление width_percent, сравнение
    
    total = (h_scan_ops + h_sum_ops + h_normalize + 
             v_scan_ops + v_sum_ops + v_normalize + 
             trend_ops + combine_ops + t_junction_ops)
    
    return {
        'algorithm': 'Complex (4×4 scan lines + trend)',
        'operations': total,
        'memory_bytes': width * height + 4*4 + 20,  # кадр + результаты сканов + переменные
        'description': '4 горизонтальных + 4 вертикальных скана, тренд, взвешивание'
    }


def benchmark_python_execution(image_path, iterations=100):
    """
    Замеряет реальное время выполнения алгоритма на Python.
    
    Args:
        image_path: путь к тестовому изображению
        iterations: количество итераций для усреднения
    
    Returns:
        dict: статистика времени выполнения
    """
    from test_line_detection import detect_line_position
    
    print(f"\n🔬 Бенчмарк Python (усреднение по {iterations} итераций)...")
    
    # Прогрев (чтобы исключить overhead первого запуска)
    detect_line_position(image_path)
    
    # Замеры
    times = []
    for i in range(iterations):
        start = time.perf_counter()
        result = detect_line_position(image_path)
        end = time.perf_counter()
        times.append((end - start) * 1000)  # в миллисекундах
        
        if (i + 1) % 10 == 0:
            print(f"  Итерация {i+1}/{iterations}: {times[-1]:.2f} мс")
    
    avg_time = np.mean(times)
    min_time = np.min(times)
    max_time = np.max(times)
    std_time = np.std(times)
    
    return {
        'iterations': iterations,
        'avg_ms': avg_time,
        'min_ms': min_time,
        'max_ms': max_time,
        'std_ms': std_time,
        'fps_python': 1000 / avg_time if avg_time > 0 else 0
    }


def estimate_esp32_performance(python_stats, operation_counts):
    """
    Оценка производительности на ESP32.
    
    Предположения:
    - C++ в 50-100 раз быстрее Python для численных операций
    - ESP32 @ 240 MHz vs современный CPU @ ~3000 MHz = ~12x медленнее
    - Итого: Python → C++ на ESP32 примерно в 4-8 раз быстрее
    
    Args:
        python_stats: статистика выполнения на Python
        operation_counts: количество операций
    
    Returns:
        dict: оценка для ESP32
    """
    # Консервативная оценка: C++ в 50 раз быстрее, но ESP32 в 12 раз медленнее
    speedup_cpp = 50
    slowdown_esp32 = 12
    
    # Итоговое ускорение
    net_speedup = speedup_cpp / slowdown_esp32  # ~4x
    
    # Пессимистичная оценка (хуже)
    pessimistic_speedup = 3
    
    # Оптимистичная оценка (лучше)
    optimistic_speedup = 8
    
    python_time_ms = python_stats['avg_ms']
    
    estimates = {
        'pessimistic': {
            'time_ms': python_time_ms / pessimistic_speedup,
            'fps': 1000 / (python_time_ms / pessimistic_speedup),
            'description': 'Худший сценарий (неоптимизированный код)'
        },
        'realistic': {
            'time_ms': python_time_ms / net_speedup,
            'fps': 1000 / (python_time_ms / net_speedup),
            'description': 'Реалистичная оценка (умеренная оптимизация)'
        },
        'optimistic': {
            'time_ms': python_time_ms / optimistic_speedup,
            'fps': 1000 / (python_time_ms / optimistic_speedup),
            'description': 'Лучший сценарий (агрессивная оптимизация)'
        }
    }
    
    return estimates


def print_report(simple_ops, complex_ops, python_stats, esp32_estimates):
    """Печатает красивый отчет."""
    print("\n" + "="*80)
    print("📊 ОТЧЕТ О ПРОИЗВОДИТЕЛЬНОСТИ АЛГОРИТМА РАСПОЗНАВАНИЯ ЛИНИИ")
    print("="*80)
    
    print("\n1️⃣  СЛОЖНОСТЬ АЛГОРИТМОВ (подсчет операций)\n")
    print(f"{'Алгоритм':<40} {'Операции':<15} {'Память (байт)':<15}")
    print("-" * 70)
    print(f"{simple_ops['algorithm']:<40} {simple_ops['operations']:<15,} {simple_ops['memory_bytes']:<15,}")
    print(f"  → {simple_ops['description']}")
    print()
    print(f"{complex_ops['algorithm']:<40} {complex_ops['operations']:<15,} {complex_ops['memory_bytes']:<15,}")
    print(f"  → {complex_ops['description']}")
    print()
    
    overhead_ratio = complex_ops['operations'] / simple_ops['operations']
    print(f"⚠️  Сложный алгоритм требует в {overhead_ratio:.1f}x больше операций")
    
    print("\n2️⃣  ПРОИЗВОДИТЕЛЬНОСТЬ НА PYTHON\n")
    print(f"  Среднее время:     {python_stats['avg_ms']:.2f} мс")
    print(f"  Минимальное:       {python_stats['min_ms']:.2f} мс")
    print(f"  Максимальное:      {python_stats['max_ms']:.2f} мс")
    print(f"  Стд. отклонение:   {python_stats['std_ms']:.2f} мс")
    print(f"  FPS (Python):      {python_stats['fps_python']:.1f} кадров/сек")
    
    print("\n3️⃣  ПРОГНОЗ ДЛЯ ESP32 (240 MHz, C++)\n")
    
    for scenario, data in esp32_estimates.items():
        label = scenario.upper()
        print(f"  {label:12} | Время: {data['time_ms']:6.2f} мс | FPS: {data['fps']:5.1f} кадров/сек")
        print(f"               └─ {data['description']}")
        print()
    
    print("="*80)
    print("📌 ВЫВОДЫ:\n")
    
    # Анализ для ESP32
    realistic_fps = esp32_estimates['realistic']['fps']
    pessimistic_fps = esp32_estimates['pessimistic']['fps']
    
    if realistic_fps >= 30:
        verdict = "✅ ОТЛИЧНО - алгоритм работает в реальном времени (>30 FPS)"
    elif realistic_fps >= 20:
        verdict = "✅ ХОРОШО - достаточно для плавного управления (20-30 FPS)"
    elif realistic_fps >= 10:
        verdict = "⚠️  ПРИЕМЛЕМО - работает, но могут быть задержки (10-20 FPS)"
    else:
        verdict = "❌ МЕДЛЕННО - нужна оптимизация (<10 FPS)"
    
    print(f"  {verdict}\n")
    
    print(f"  Реалистичная частота: {realistic_fps:.1f} FPS")
    print(f"  Пессимистичная частота: {pessimistic_fps:.1f} FPS")
    print()
    
    # Рекомендации
    print("💡 РЕКОМЕНДАЦИИ ДЛЯ ОПТИМИЗАЦИИ:\n")
    
    if realistic_fps < 20:
        print("  1. ⚡ Уменьшить количество сканирующих линий (4×4 → 2×2)")
        print("  2. 🎯 Использовать только горизонтальные сканы (убрать вертикальные)")
        print("  3. 📉 Снизить разрешение камеры (160×120 → 96×96)")
        print("  4. 🔧 Использовать целочисленную арифметику вместо float")
    else:
        print("  1. ✅ Текущий алгоритм достаточно быстр для ESP32")
        print("  2. 🎨 Можно добавить дополнительные фичи (сглаживание, фильтрация)")
        print("  3. 📸 Есть запас для увеличения разрешения камеры")
    
    print()
    print("="*80)


def main():
    """Главная функция бенчмарка."""
    print("🔬 БЕНЧМАРК АЛГОРИТМА РАСПОЗНАВАНИЯ ЛИНИИ ДЛЯ ESP32\n")
    
    # Находим тестовое изображение
    output_dir = Path(__file__).parent / 'output'
    test_image = None
    
    # Ищем первый доступный кадр
    frames_dir = output_dir / 'frames'
    if frames_dir.exists():
        frames = sorted(frames_dir.glob('frame_*.jpg'))
        if frames:
            test_image = frames[len(frames) // 2]  # Берем средний кадр
    
    if not test_image or not test_image.exists():
        print("❌ Не найдены тестовые изображения!")
        print("   Запустите generate_track_map.py для генерации кадров")
        return 1
    
    print(f"📸 Тестовое изображение: {test_image.name}\n")
    
    # 1. Подсчет операций
    print("📊 Подсчет количества операций...")
    simple_ops = count_operations_simple_algorithm()
    complex_ops = count_operations_complex_algorithm()
    print(f"  ✓ Простой алгоритм: {simple_ops['operations']:,} операций")
    print(f"  ✓ Сложный алгоритм: {complex_ops['operations']:,} операций")
    
    # 2. Бенчмарк на Python
    python_stats = benchmark_python_execution(str(test_image), iterations=100)
    print(f"\n  ✓ Среднее время (Python): {python_stats['avg_ms']:.2f} мс")
    print(f"  ✓ FPS (Python): {python_stats['fps_python']:.1f} кадров/сек")
    
    # 3. Оценка для ESP32
    print("\n📡 Оценка производительности на ESP32...")
    esp32_estimates = estimate_esp32_performance(python_stats, complex_ops)
    print(f"  ✓ Реалистичная оценка: {esp32_estimates['realistic']['time_ms']:.2f} мс")
    print(f"  ✓ Реалистичный FPS: {esp32_estimates['realistic']['fps']:.1f} кадров/сек")
    
    # 4. Вывод отчета
    print_report(simple_ops, complex_ops, python_stats, esp32_estimates)
    
    return 0


if __name__ == '__main__':
    exit(main())
