#!/usr/bin/env python3
"""
Улучшенный скрипт для выделения линии с учетом отражения от светодиода.

Использует три типа калибровочных изображений:
1. Белое поле (white background)
2. Черное поле (black background) 
3. Изображение с линией (foreground with line)

Проблема: отражение от светодиода камеры создает яркое пятно на изображении.

Решение: Используем разностный метод с двумя фонами для компенсации отражения.

Метод:
1. white_bg - фон белого поля (с отражением светодиода)
2. black_bg - фон черного поля (с отражением светодиода)
3. foreground - текущее изображение с линией (с отражением светодиода)

Алгоритм:
  - Отражение присутствует на всех трех изображениях в одном месте
  - При вычитании отражение компенсируется
  - Линия выделяется чисто без артефактов от светодиода
"""

import os
import sys
from pathlib import Path
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from typing import Tuple, Dict, List
import cv2

# Константы
OUTPUT_DIR = Path(__file__).parent / 'output'
OUTPUT_DIR.mkdir(exist_ok=True)


def load_image(image_path: str) -> np.ndarray:
    """Загружает изображение в grayscale."""
    img = Image.open(image_path)
    if img.mode != 'L':
        img = img.convert('L')
    return np.array(img, dtype=np.uint8)


def method_dual_background_subtraction(white_bg: np.ndarray, black_bg: np.ndarray,
                                       foreground: np.ndarray, threshold: int = 30) -> np.ndarray:
    """
    Метод двойного вычитания фона для компенсации отражения от светодиода.
    
    Идея: Отражение светодиода присутствует на всех изображениях.
    При правильном вычитании оно компенсируется, остается только линия.
    
    Формула для черной линии на белом фоне:
    mask = (white_bg - foreground) - (white_bg - black_bg) * k
    
    где k - коэффициент компенсации
    
    Args:
        white_bg: Фон белого поля
        black_bg: Фон черного поля
        foreground: Изображение с линией
        threshold: Пороговое значение
    
    Returns:
        Бинарная маска линии
    """
    # Вычитание 1: белый фон - текущее изображение (дает линию + разницу в отражении)
    diff1 = white_bg.astype(np.int16) - foreground.astype(np.int16)
    
    # Вычитание 2: белый фон - черный фон (дает разницу между белым и черным полем)
    diff2 = white_bg.astype(np.int16) - black_bg.astype(np.int16)
    
    # Нормализуем diff2 чтобы использовать как компенсацию
    # Компенсация отражения: вычитаем долю от diff2
    compensated = diff1 - (diff2 * 0.3).astype(np.int16)
    
    # Обнуляем отрицательные значения
    compensated[compensated < 0] = 0
    
    # Применяем порог
    mask = (compensated > threshold).astype(np.uint8) * 255
    
    return mask


def method_normalized_difference(white_bg: np.ndarray, black_bg: np.ndarray,
                                 foreground: np.ndarray, threshold: float = 0.15) -> np.ndarray:
    """
    Нормализованная разность с учетом динамического диапазона.
    
    Формула: (white - fg) / (white - black)
    
    Преимущество: инвариантен к отражению светодиода.
    
    Args:
        white_bg: Фон белого поля
        black_bg: Фон черного поля
        foreground: Изображение с линией
        threshold: Пороговое значение (0-1)
    
    Returns:
        Бинарная маска линии
    """
    # Избегаем деления на ноль
    white_f = white_bg.astype(np.float32)
    black_f = black_bg.astype(np.float32)
    fg_f = foreground.astype(np.float32)
    
    # Динамический диапазон
    dynamic_range = white_f - black_f + 1e-6  # Добавляем epsilon
    
    # Нормализованная разность
    normalized = (white_f - fg_f) / dynamic_range
    
    # Обрезаем значения
    normalized = np.clip(normalized, 0, 1)
    
    # Применяем порог
    mask = (normalized > threshold).astype(np.uint8) * 255
    
    return mask


def method_adaptive_dual_threshold(white_bg: np.ndarray, black_bg: np.ndarray,
                                   foreground: np.ndarray) -> np.ndarray:
    """
    Адаптивный двойной порог на основе локального динамического диапазона.
    
    Args:
        white_bg: Фон белого поля
        black_bg: Фон черного поля
        foreground: Изображение с линией
    
    Returns:
        Бинарная маска линии
    """
    # Вычисляем локальный динамический диапазон
    dynamic_range = white_bg.astype(np.float32) - black_bg.astype(np.float32)
    
    # Вычисляем разность
    diff = white_bg.astype(np.float32) - foreground.astype(np.float32)
    
    # Адаптивный порог: 30% от динамического диапазона
    threshold_map = dynamic_range * 0.3
    
    # Применяем адаптивный порог
    mask = (diff > threshold_map).astype(np.uint8) * 255
    
    return mask


def method_ratio_based(white_bg: np.ndarray, black_bg: np.ndarray,
                       foreground: np.ndarray, threshold: float = 0.85) -> np.ndarray:
    """
    Метод на основе отношения яркости.
    
    Идея: Линия имеет меньшую яркость относительно white_bg.
    ratio = foreground / white_bg
    
    Args:
        white_bg: Фон белого поля
        black_bg: Фон черного поля
        foreground: Изображение с линией
        threshold: Пороговое значение (0-1)
    
    Returns:
        Бинарная маска линии
    """
    # Избегаем деления на ноль
    white_f = white_bg.astype(np.float32) + 1e-6
    fg_f = foreground.astype(np.float32)
    
    # Отношение яркости
    ratio = fg_f / white_f
    
    # Линия темнее, поэтому ratio < 1
    # Применяем порог
    mask = (ratio < threshold).astype(np.uint8) * 255
    
    return mask


def method_reference_white_black(white_bg: np.ndarray, black_bg: np.ndarray,
                                 foreground: np.ndarray, line_threshold: float = 0.4) -> np.ndarray:
    """
    Метод с использованием white и black как опорных точек.
    
    Идея: Нормализуем foreground относительно известных white и black значений.
    
    normalized = (foreground - black) / (white - black)
    
    Для белого фона: normalized ≈ 1.0
    Для черной линии: normalized ≈ 0.0-0.3
    
    Args:
        white_bg: Фон белого поля
        black_bg: Фон черного поля
        foreground: Изображение с линией
        line_threshold: Порог для обнаружения линии (0-1)
    
    Returns:
        Бинарная маска линии
    """
    # Преобразуем в float
    white_f = white_bg.astype(np.float32)
    black_f = black_bg.astype(np.float32)
    fg_f = foreground.astype(np.float32)
    
    # Динамический диапазон
    dynamic_range = white_f - black_f + 1e-6
    
    # Нормализация
    normalized = (fg_f - black_f) / dynamic_range
    
    # Обрезаем
    normalized = np.clip(normalized, 0, 1)
    
    # Черная линия даст низкие значения
    mask = (normalized < line_threshold).astype(np.uint8) * 255
    
    return mask


def evaluate_mask(mask: np.ndarray) -> Dict:
    """Оценивает качество маски линии."""
    coverage_percent = 100 * np.sum(mask > 0) / mask.size
    
    num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(mask, connectivity=8)
    num_components = num_labels - 1
    
    if num_components > 0:
        component_sizes = stats[1:, cv2.CC_STAT_AREA]
        largest_component_size = np.max(component_sizes)
        largest_component_percent = 100 * largest_component_size / mask.size
    else:
        largest_component_size = 0
        largest_component_percent = 0
    
    compactness = largest_component_percent / coverage_percent if coverage_percent > 0 else 0
    
    return {
        'coverage_percent': coverage_percent,
        'num_components': num_components,
        'largest_component_size': largest_component_size,
        'largest_component_percent': largest_component_percent,
        'compactness': compactness,
    }


def compare_methods_with_led_compensation(white_bg_path: str, black_bg_path: str,
                                          foreground_path: str) -> None:
    """
    Сравнивает методы выделения линии с компенсацией отражения светодиода.
    
    Args:
        white_bg_path: Путь к изображению белого поля
        black_bg_path: Путь к изображению черного поля
        foreground_path: Путь к изображению с линией
    """
    print(f"\n{'='*80}")
    print(f"🎯 СРАВНЕНИЕ МЕТОДОВ С КОМПЕНСАЦИЕЙ ОТРАЖЕНИЯ СВЕТОДИОДА")
    print(f"{'='*80}")
    print(f"\n📂 Исходные данные:")
    print(f"  Белое поле:  {Path(white_bg_path).name}")
    print(f"  Черное поле: {Path(black_bg_path).name}")
    print(f"  С линией:    {Path(foreground_path).name}")
    
    # Загружаем изображения
    white_bg = load_image(white_bg_path)
    black_bg = load_image(black_bg_path)
    foreground = load_image(foreground_path)
    
    print(f"\n📊 Размеры: {white_bg.shape[1]}×{white_bg.shape[0]} px")
    
    # Анализ отражения светодиода
    print(f"\n🔍 АНАЛИЗ ОТРАЖЕНИЯ СВЕТОДИОДА:")
    white_max = np.max(white_bg)
    black_max = np.max(black_bg)
    fg_max = np.max(foreground)
    
    print(f"  Макс. яркость белого поля:  {white_max}")
    print(f"  Макс. яркость черного поля: {black_max}")
    print(f"  Макс. яркость с линией:     {fg_max}")
    
    if black_max > 200:
        print(f"  ⚠️  ОБНАРУЖЕНО яркое отражение на черном поле ({black_max})")
        print(f"  💡 Используем методы с компенсацией отражения")
    
    # Применяем методы
    print(f"\n{'='*80}")
    print(f"🔬 ПРИМЕНЕНИЕ МЕТОДОВ")
    print(f"{'='*80}")
    
    methods = {
        'Двойное вычитание фона': lambda: method_dual_background_subtraction(white_bg, black_bg, foreground),
        'Нормализованная разность': lambda: method_normalized_difference(white_bg, black_bg, foreground),
        'Адаптивный двойной порог': lambda: method_adaptive_dual_threshold(white_bg, black_bg, foreground),
        'На основе отношения': lambda: method_ratio_based(white_bg, black_bg, foreground),
        'Опорные точки white/black': lambda: method_reference_white_black(white_bg, black_bg, foreground),
        # Для сравнения: простое вычитание (без компенсации)
        'Простое вычитание (без компенсации)': lambda: (white_bg.astype(np.int16) - foreground.astype(np.int16) > 30).astype(np.uint8) * 255,
    }
    
    results = {}
    
    for name, method_func in methods.items():
        print(f"\n{name}:")
        try:
            mask = method_func()
            metrics = evaluate_mask(mask)
            
            results[name] = {
                'mask': mask,
                'metrics': metrics,
            }
            
            print(f"  ✅ Покрытие: {metrics['coverage_percent']:.2f}%")
            print(f"  ✅ Компонент: {metrics['num_components']}")
            print(f"  ✅ Компактность: {metrics['compactness']:.3f}")
            
        except Exception as e:
            print(f"  ❌ Ошибка: {e}")
            import traceback
            traceback.print_exc()
    
    # Анализ и рейтинг
    print(f"\n{'='*80}")
    print(f"📊 СРАВНИТЕЛЬНЫЙ АНАЛИЗ")
    print(f"{'='*80}")
    
    print(f"\n{'Метод':<40} {'Покрытие':>10} {'Компонент':>10} {'Компактн.':>10}")
    print('─'*80)
    
    for name, result in results.items():
        m = result['metrics']
        print(f"{name:<40} {m['coverage_percent']:>9.2f}% {m['num_components']:>10} {m['compactness']:>10.3f}")
    
    # Рейтинг
    scores = {}
    for name, result in results.items():
        m = result['metrics']
        compactness_score = m['compactness']
        coverage_optimal = 15.0
        coverage_score = 1.0 - abs(m['coverage_percent'] - coverage_optimal) / 50.0
        coverage_score = max(0, coverage_score)
        component_score = 1.0 / (m['num_components'] + 1)
        total_score = compactness_score * 0.5 + coverage_score * 0.3 + component_score * 0.2
        scores[name] = total_score
    
    sorted_methods = sorted(scores.items(), key=lambda x: x[1], reverse=True)
    
    print(f"\n{'='*80}")
    print(f"🏆 РЕЙТИНГ МЕТОДОВ")
    print(f"{'='*80}\n")
    
    for i, (name, score) in enumerate(sorted_methods, 1):
        emoji = ['🥇', '🥈', '🥉', '4️⃣', '5️⃣', '6️⃣'][min(i-1, 5)]
        print(f"  {emoji} {name:<40} Балл: {score:.3f}")
    
    best_method = sorted_methods[0][0]
    print(f"\n💡 ЛУЧШИЙ МЕТОД: {best_method}")
    print(f"\n   Этот метод лучше всего компенсирует отражение от светодиода.")
    
    # Создаем визуализацию
    create_led_compensation_visualization(white_bg, black_bg, foreground, results, best_method)
    
    print(f"\n{'='*80}\n")


def create_led_compensation_visualization(white_bg: np.ndarray, black_bg: np.ndarray,
                                          foreground: np.ndarray, results: Dict,
                                          best_method: str) -> None:
    """Создает визуализацию методов с компенсацией отражения."""
    try:
        num_methods = len(results)
        
        fig = plt.figure(figsize=(20, 14))
        gs = gridspec.GridSpec(4, num_methods, height_ratios=[1, 1, 1, 0.6])
        
        fig.suptitle('Сравнение методов выделения линии с компенсацией отражения светодиода',
                    fontsize=16, fontweight='bold')
        
        # Строка 1: Foreground с наложенными масками
        for i, (name, result) in enumerate(results.items()):
            ax = fig.add_subplot(gs[0, i])
            composite = cv2.cvtColor(foreground, cv2.COLOR_GRAY2RGB)
            mask = result['mask']
            composite[mask > 0] = [255, 0, 0]
            ax.imshow(composite)
            if name == best_method:
                title = f"🏆 {name}\n(Лучший)"
                ax.set_title(title, fontsize=9, fontweight='bold', color='green')
            else:
                ax.set_title(name, fontsize=9)
            ax.axis('off')
        
        # Строка 2: Чистые маски
        for i, (name, result) in enumerate(results.items()):
            ax = fig.add_subplot(gs[1, i])
            mask = result['mask']
            ax.imshow(mask, cmap='gray', vmin=0, vmax=255)
            m = result['metrics']
            title = f"Покр: {m['coverage_percent']:.1f}%\nКомп: {m['num_components']}\nКомпакт: {m['compactness']:.3f}"
            ax.set_title(title, fontsize=8)
            ax.axis('off')
        
        # Строка 3: Профили яркости (покажем отражение светодиода)
        center_y = white_bg.shape[0] // 2
        for i, (name, result) in enumerate(results.items()):
            ax = fig.add_subplot(gs[2, i])
            
            # Горизонтальный профиль через центр
            white_profile = white_bg[center_y, :]
            black_profile = black_bg[center_y, :]
            fg_profile = foreground[center_y, :]
            
            ax.plot(white_profile, label='Белое', alpha=0.7, linewidth=2)
            ax.plot(black_profile, label='Черное', alpha=0.7, linewidth=2)
            ax.plot(fg_profile, label='С линией', alpha=0.7, linewidth=2)
            
            # Отмечаем пик (отражение светодиода)
            max_idx = np.argmax(white_profile)
            ax.axvline(x=max_idx, color='red', linestyle='--', alpha=0.5, label='Отражение LED')
            
            ax.set_title('Профиль яркости', fontsize=8)
            ax.legend(fontsize=6, loc='upper right')
            ax.grid(True, alpha=0.3)
            ax.set_ylim(0, 255)
        
        # Строка 4: Исходные изображения
        ax_white = fig.add_subplot(gs[3, :num_methods//3])
        ax_white.imshow(white_bg, cmap='gray')
        ax_white.set_title('White background\n(с отражением LED)', fontsize=10)
        ax_white.axis('off')
        
        ax_black = fig.add_subplot(gs[3, num_methods//3:2*num_methods//3])
        ax_black.imshow(black_bg, cmap='gray')
        ax_black.set_title('Black background\n(с отражением LED)', fontsize=10)
        ax_black.axis('off')
        
        ax_fg = fig.add_subplot(gs[3, 2*num_methods//3:])
        ax_fg.imshow(foreground, cmap='gray')
        ax_fg.set_title('Foreground\n(с линией и отражением LED)', fontsize=10)
        ax_fg.axis('off')
        
        plt.tight_layout()
        
        output_path = OUTPUT_DIR / 'led_compensation_comparison.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"\n📊 Визуализация сохранена: {output_path}")
        plt.close()
        
    except Exception as e:
        print(f"⚠️  Не удалось создать визуализацию: {e}")
        import traceback
        traceback.print_exc()


def main():
    """Основная функция."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Выделение линии с компенсацией отражения светодиода',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:

  1. С тремя изображениями:
     python3 extract_line_led_compensation.py white.jpg black.jpg foreground.jpg

  2. С калибровочными кадрами:
     python3 extract_line_led_compensation.py \\
         data/img_calibration/calibration_*.jpg \\
         data/img_calibration/black/calibration_*.jpg \\
         data/img_straight/straight_*.jpg
        """
    )
    
    parser.add_argument('white_bg', help='Путь к изображению белого поля')
    parser.add_argument('black_bg', help='Путь к изображению черного поля')
    parser.add_argument('foreground', help='Путь к изображению с линией')
    
    args = parser.parse_args()
    
    # Проверяем файлы
    for path, name in [(args.white_bg, 'white_bg'), (args.black_bg, 'black_bg'), (args.foreground, 'foreground')]:
        p = Path(path)
        if not p.exists():
            print(f"❌ Файл не найден: {path}")
            return 1
    
    compare_methods_with_led_compensation(args.white_bg, args.black_bg, args.foreground)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
