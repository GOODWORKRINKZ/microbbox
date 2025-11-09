#!/usr/bin/env python3
"""
Скрипт для выделения линии методом вычитания фона (background subtraction).

Сравнивает различные методы выделения линии используя:
- Кадр БЕЗ линии (пустое поле, background)
- Кадр С линией (foreground)

Реализованные методы:
1. Простое вычитание (difference)
2. Абсолютная разница (absolute difference)
3. Адаптивное пороговое значение (adaptive threshold)
4. Морфологические операции (opening, closing)
5. Метод на основе стандартного отклонения

Результат: бинарная маска линии для дальнейшей обработки.
"""

import os
import sys
from pathlib import Path
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from typing import Tuple, Dict
import cv2

# Константы
OUTPUT_DIR = Path(__file__).parent / 'output'
OUTPUT_DIR.mkdir(exist_ok=True)


def load_image(image_path: str) -> np.ndarray:
    """
    Загружает изображение и конвертирует в numpy массив.
    
    Args:
        image_path: Путь к изображению
    
    Returns:
        numpy массив с изображением (grayscale, uint8)
    """
    img = Image.open(image_path)
    
    # Конвертируем в grayscale если необходимо
    if img.mode != 'L':
        img = img.convert('L')
    
    return np.array(img, dtype=np.uint8)


def method_simple_subtraction(background: np.ndarray, foreground: np.ndarray, 
                              threshold: int = 20) -> np.ndarray:
    """
    Метод 1: Простое вычитание с порогом.
    
    Вычитает background из foreground и применяет пороговое значение.
    Подходит, когда линия темнее фона.
    
    Args:
        background: Изображение без линии
        foreground: Изображение с линией
        threshold: Пороговое значение для бинаризации
    
    Returns:
        Бинарная маска линии
    """
    # Вычитаем background из foreground (линия должна быть темнее)
    diff = foreground.astype(np.int16) - background.astype(np.int16)
    
    # Инвертируем если линия темная (отрицательные значения)
    diff = -diff  # Темная линия дает отрицательные значения
    
    # Обнуляем положительные (это шум)
    diff[diff < 0] = 0
    
    # Применяем порог
    mask = (diff > threshold).astype(np.uint8) * 255
    
    return mask


def method_absolute_difference(background: np.ndarray, foreground: np.ndarray, 
                               threshold: int = 20) -> np.ndarray:
    """
    Метод 2: Абсолютная разница.
    
    Вычисляет абсолютную разницу между кадрами.
    Универсальный метод, работает для светлых и темных линий.
    
    Args:
        background: Изображение без линии
        foreground: Изображение с линией
        threshold: Пороговое значение для бинаризации
    
    Returns:
        Бинарная маска линии
    """
    # Абсолютная разница
    diff = cv2.absdiff(background, foreground)
    
    # Применяем порог
    _, mask = cv2.threshold(diff, threshold, 255, cv2.THRESH_BINARY)
    
    return mask


def method_adaptive_threshold(background: np.ndarray, foreground: np.ndarray, 
                              block_size: int = 11, C: int = 2) -> np.ndarray:
    """
    Метод 3: Адаптивное пороговое значение.
    
    Использует адаптивную бинаризацию после вычитания фона.
    Подходит для неоднородного освещения.
    
    Args:
        background: Изображение без линии
        foreground: Изображение с линией
        block_size: Размер блока для адаптивной бинаризации (нечетный)
        C: Константа вычитания
    
    Returns:
        Бинарная маска линии
    """
    # Абсолютная разница
    diff = cv2.absdiff(background, foreground)
    
    # Адаптивная бинаризация
    mask = cv2.adaptiveThreshold(
        diff, 255, 
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
        cv2.THRESH_BINARY, 
        block_size, 
        -C  # Инвертируем порог для темных линий
    )
    
    return mask


def method_morphological(background: np.ndarray, foreground: np.ndarray, 
                        threshold: int = 20, kernel_size: int = 3) -> np.ndarray:
    """
    Метод 4: Вычитание с морфологическими операциями.
    
    После вычитания применяет морфологические операции для улучшения маски:
    - Opening: удаляет мелкий шум
    - Closing: заполняет дыры в линии
    
    Args:
        background: Изображение без линии
        foreground: Изображение с линией
        threshold: Пороговое значение для бинаризации
        kernel_size: Размер морфологического ядра
    
    Returns:
        Бинарная маска линии
    """
    # Абсолютная разница
    diff = cv2.absdiff(background, foreground)
    
    # Применяем порог
    _, mask = cv2.threshold(diff, threshold, 255, cv2.THRESH_BINARY)
    
    # Морфологические операции
    kernel = np.ones((kernel_size, kernel_size), np.uint8)
    
    # Opening: удаляет мелкий шум (эрозия + дилатация)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
    
    # Closing: заполняет дыры (дилатация + эрозия)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    
    return mask


def method_stddev_based(background: np.ndarray, foreground: np.ndarray, 
                       num_std: float = 2.0) -> np.ndarray:
    """
    Метод 5: На основе стандартного отклонения.
    
    Вычисляет статистическую значимость различия:
    - Если разница превышает num_std * std(background), это линия
    
    Адаптивный метод, учитывает шум фона.
    
    Args:
        background: Изображение без линии
        foreground: Изображение с линией
        num_std: Количество стандартных отклонений для порога
    
    Returns:
        Бинарная маска линии
    """
    # Вычисляем разницу
    diff = cv2.absdiff(background, foreground)
    
    # Вычисляем стандартное отклонение background (шум)
    std_bg = np.std(background)
    
    # Порог = num_std * стандартное отклонение
    threshold = num_std * std_bg
    
    # Применяем порог
    mask = (diff > threshold).astype(np.uint8) * 255
    
    return mask


def method_clahe_enhanced(background: np.ndarray, foreground: np.ndarray,
                         threshold: int = 20) -> np.ndarray:
    """
    Метод 6: С CLAHE (Contrast Limited Adaptive Histogram Equalization).
    
    Применяет CLAHE к разнице для улучшения контраста перед бинаризацией.
    Полезно при низком контрасте линии.
    
    Args:
        background: Изображение без линии
        foreground: Изображение с линией
        threshold: Пороговое значение для бинаризации
    
    Returns:
        Бинарная маска линии
    """
    # Абсолютная разница
    diff = cv2.absdiff(background, foreground)
    
    # Применяем CLAHE для улучшения контраста
    clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8))
    diff_enhanced = clahe.apply(diff)
    
    # Применяем порог
    _, mask = cv2.threshold(diff_enhanced, threshold, 255, cv2.THRESH_BINARY)
    
    return mask


def evaluate_mask(mask: np.ndarray, foreground: np.ndarray) -> Dict:
    """
    Оценивает качество маски линии.
    
    Args:
        mask: Бинарная маска линии
        foreground: Оригинальное изображение с линией
    
    Returns:
        Словарь с метриками качества
    """
    # Процент покрытия
    coverage_percent = 100 * np.sum(mask > 0) / mask.size
    
    # Количество связных компонент (меньше = лучше, идеально = 1)
    num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(mask, connectivity=8)
    num_components = num_labels - 1  # Минус фон
    
    # Размер наибольшей компоненты
    if num_components > 0:
        component_sizes = stats[1:, cv2.CC_STAT_AREA]  # Пропускаем фон
        largest_component_size = np.max(component_sizes)
        largest_component_percent = 100 * largest_component_size / mask.size
    else:
        largest_component_size = 0
        largest_component_percent = 0
    
    # Компактность (должна быть одна связная компонента)
    compactness = largest_component_percent / coverage_percent if coverage_percent > 0 else 0
    
    return {
        'coverage_percent': coverage_percent,
        'num_components': num_components,
        'largest_component_size': largest_component_size,
        'largest_component_percent': largest_component_percent,
        'compactness': compactness,  # 1.0 = идеально (одна компонента)
    }


def compare_methods(background_path: str, foreground_path: str) -> None:
    """
    Сравнивает все методы выделения линии.
    
    Args:
        background_path: Путь к изображению без линии
        foreground_path: Путь к изображению с линией
    """
    print(f"\n{'='*80}")
    print(f"🎯 СРАВНЕНИЕ МЕТОДОВ ВЫДЕЛЕНИЯ ЛИНИИ")
    print(f"{'='*80}")
    print(f"\n📂 Исходные данные:")
    print(f"  Background (без линии): {Path(background_path).name}")
    print(f"  Foreground (с линией):  {Path(foreground_path).name}")
    
    # Загружаем изображения
    background = load_image(background_path)
    foreground = load_image(foreground_path)
    
    print(f"\n📊 Размеры: {background.shape[1]}×{background.shape[0]} px")
    
    # Проверяем размеры
    if background.shape != foreground.shape:
        print(f"\n⚠️  ВНИМАНИЕ: Разные размеры изображений!")
        print(f"   Background: {background.shape}")
        print(f"   Foreground: {foreground.shape}")
        return
    
    # Применяем все методы
    print(f"\n{'='*80}")
    print(f"🔬 ПРИМЕНЕНИЕ МЕТОДОВ")
    print(f"{'='*80}")
    
    methods = {
        'Простое вычитание': lambda: method_simple_subtraction(background, foreground, threshold=20),
        'Абсолютная разница': lambda: method_absolute_difference(background, foreground, threshold=20),
        'Адаптивный порог': lambda: method_adaptive_threshold(background, foreground, block_size=11, C=2),
        'Морфологические операции': lambda: method_morphological(background, foreground, threshold=20, kernel_size=3),
        'На основе std отклонения': lambda: method_stddev_based(background, foreground, num_std=2.0),
        'CLAHE-улучшенный': lambda: method_clahe_enhanced(background, foreground, threshold=20),
    }
    
    results = {}
    
    for name, method_func in methods.items():
        print(f"\n{name}:")
        try:
            mask = method_func()
            metrics = evaluate_mask(mask, foreground)
            
            results[name] = {
                'mask': mask,
                'metrics': metrics,
            }
            
            print(f"  ✅ Покрытие: {metrics['coverage_percent']:.2f}%")
            print(f"  ✅ Компонент: {metrics['num_components']}")
            print(f"  ✅ Крупнейшая компонента: {metrics['largest_component_percent']:.2f}%")
            print(f"  ✅ Компактность: {metrics['compactness']:.3f} (1.0 = идеально)")
            
        except Exception as e:
            print(f"  ❌ Ошибка: {e}")
            import traceback
            traceback.print_exc()
    
    # Анализируем результаты
    print(f"\n{'='*80}")
    print(f"📊 СРАВНИТЕЛЬНЫЙ АНАЛИЗ")
    print(f"{'='*80}")
    
    print(f"\n{'Метод':<30} {'Покрытие':>10} {'Компонент':>10} {'Компактн.':>10}")
    print('─'*80)
    
    for name, result in results.items():
        m = result['metrics']
        print(f"{name:<30} {m['coverage_percent']:>9.2f}% {m['num_components']:>10} {m['compactness']:>10.3f}")
    
    # Выбираем лучший метод
    print(f"\n{'='*80}")
    print(f"🏆 РЕКОМЕНДАЦИИ")
    print(f"{'='*80}")
    
    # Критерии оценки:
    # 1. Компактность (близка к 1.0)
    # 2. Адекватное покрытие (5-30% для типичной линии)
    # 3. Минимум компонент (в идеале 1)
    
    scores = {}
    for name, result in results.items():
        m = result['metrics']
        
        # Балл за компактность (чем ближе к 1, тем лучше)
        compactness_score = m['compactness']
        
        # Балл за покрытие (оптимально 10-25%)
        coverage_optimal = 15.0
        coverage_score = 1.0 - abs(m['coverage_percent'] - coverage_optimal) / 50.0
        coverage_score = max(0, coverage_score)
        
        # Балл за количество компонент (меньше = лучше)
        component_score = 1.0 / (m['num_components'] + 1)
        
        # Общий балл (взвешенная сумма)
        total_score = (
            compactness_score * 0.5 +
            coverage_score * 0.3 +
            component_score * 0.2
        )
        
        scores[name] = total_score
    
    # Сортируем по баллу
    sorted_methods = sorted(scores.items(), key=lambda x: x[1], reverse=True)
    
    print(f"\n🥇 Рейтинг методов (по качеству маски):\n")
    
    for i, (name, score) in enumerate(sorted_methods, 1):
        emoji = ['🥇', '🥈', '🥉', '4️⃣', '5️⃣', '6️⃣'][i-1]
        print(f"  {emoji} {name:<30} Балл: {score:.3f}")
    
    # Рекомендация
    best_method = sorted_methods[0][0]
    print(f"\n💡 ЛУЧШИЙ МЕТОД: {best_method}")
    print(f"\n   Этот метод дает наиболее чистую и компактную маску линии.")
    
    # Создаем визуализацию
    create_comparison_visualization(background, foreground, results, best_method)
    
    print(f"\n{'='*80}\n")


def create_comparison_visualization(background: np.ndarray, foreground: np.ndarray,
                                   results: Dict, best_method: str) -> None:
    """
    Создает визуализацию сравнения методов.
    
    Args:
        background: Изображение без линии
        foreground: Изображение с линией
        results: Результаты применения методов
        best_method: Название лучшего метода
    """
    try:
        num_methods = len(results)
        
        # Создаем фигуру
        fig = plt.figure(figsize=(20, 12))
        gs = gridspec.GridSpec(3, num_methods, height_ratios=[1, 1, 0.5])
        
        # Заголовок
        fig.suptitle('Сравнение методов выделения линии методом вычитания фона', 
                    fontsize=16, fontweight='bold')
        
        # Первая строка: исходные изображения + маски
        for i, (name, result) in enumerate(results.items()):
            ax = fig.add_subplot(gs[0, i])
            
            # Создаем композит: оригинал с наложенной маской
            composite = cv2.cvtColor(foreground, cv2.COLOR_GRAY2RGB)
            mask = result['mask']
            
            # Накладываем маску красным цветом
            composite[mask > 0] = [255, 0, 0]
            
            ax.imshow(composite)
            
            # Подсветка лучшего метода
            if name == best_method:
                title = f"🏆 {name}\n(Лучший метод)"
                ax.set_title(title, fontsize=11, fontweight='bold', color='green')
            else:
                ax.set_title(name, fontsize=11)
            
            ax.axis('off')
        
        # Вторая строка: только маски
        for i, (name, result) in enumerate(results.items()):
            ax = fig.add_subplot(gs[1, i])
            mask = result['mask']
            ax.imshow(mask, cmap='gray', vmin=0, vmax=255)
            
            m = result['metrics']
            title = f"Покрытие: {m['coverage_percent']:.1f}%\nКомпонент: {m['num_components']}\nКомпактн: {m['compactness']:.3f}"
            ax.set_title(title, fontsize=9)
            ax.axis('off')
        
        # Третья строка: исходные изображения для справки
        ax_bg = fig.add_subplot(gs[2, :num_methods//2])
        ax_bg.imshow(background, cmap='gray')
        ax_bg.set_title('Background (без линии)', fontsize=10)
        ax_bg.axis('off')
        
        ax_fg = fig.add_subplot(gs[2, num_methods//2:])
        ax_fg.imshow(foreground, cmap='gray')
        ax_fg.set_title('Foreground (с линией)', fontsize=10)
        ax_fg.axis('off')
        
        plt.tight_layout()
        
        output_path = OUTPUT_DIR / 'line_extraction_comparison.png'
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
        description='Сравнение методов выделения линии методом вычитания фона',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:

  1. Базовое сравнение:
     python3 extract_line_background_subtraction.py background.jpg foreground.jpg

  2. С калибровочными кадрами:
     python3 extract_line_background_subtraction.py \\
         ../data/img_calibration/calibration_*.jpg \\
         ../data/img_straight/straight_*.jpg
        """
    )
    
    parser.add_argument('background', help='Путь к изображению БЕЗ линии (background)')
    parser.add_argument('foreground', help='Путь к изображению С линией (foreground)')
    
    args = parser.parse_args()
    
    # Проверяем существование файлов
    bg_path = Path(args.background)
    fg_path = Path(args.foreground)
    
    if not bg_path.exists():
        print(f"❌ Файл не найден: {args.background}")
        return 1
    
    if not fg_path.exists():
        print(f"❌ Файл не найден: {args.foreground}")
        return 1
    
    # Запускаем сравнение
    compare_methods(str(bg_path), str(fg_path))
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
