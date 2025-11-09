#!/usr/bin/env python3
"""
Анализ эффективной области сканирования с учетом физических ограничений камеры.

Проблемы:
- Пересвет (overexposure) - яркие области с потерей деталей
- Затемнение (underexposure) - темные области с низким контрастом
- Отражение LED - яркое пятно от светодиода
- Виньетирование - затемнение по краям

Решение: Определить эффективную область (ROI - Region of Interest) для сканирования линии.

Тестирование на всех типах сценариев:
1. Прямая линия (straight)
2. Поворот влево (left)
3. Поворот вправо (right)
4. Окончание линии/T-пересечение (terminate)
"""

import os
import sys
from pathlib import Path
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.patches as patches
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


def analyze_exposure_map(image: np.ndarray, overexposure_thresh: int = 240,
                         underexposure_thresh: int = 20) -> Dict:
    """
    Анализирует карту экспозиции изображения.
    
    Args:
        image: Изображение
        overexposure_thresh: Порог пересвета
        underexposure_thresh: Порог затемнения
    
    Returns:
        Словарь с картами и статистикой
    """
    h, w = image.shape
    
    # Карта пересвета (яркие области)
    overexposed = image >= overexposure_thresh
    overexposed_percent = 100 * np.sum(overexposed) / image.size
    
    # Карта затемнения (темные области)
    underexposed = image <= underexposure_thresh
    underexposed_percent = 100 * np.sum(underexposed) / image.size
    
    # Эффективная область (не пересвечена и не затемнена)
    effective = ~(overexposed | underexposed)
    effective_percent = 100 * np.sum(effective) / image.size
    
    # Вертикальный профиль (по высоте) - средняя яркость по горизонтали
    vertical_profile = np.mean(image, axis=1)
    
    # Горизонтальный профиль (по ширине) - средняя яркость по вертикали
    horizontal_profile = np.mean(image, axis=0)
    
    return {
        'overexposed_map': overexposed,
        'underexposed_map': underexposed,
        'effective_map': effective,
        'overexposed_percent': overexposed_percent,
        'underexposed_percent': underexposed_percent,
        'effective_percent': effective_percent,
        'vertical_profile': vertical_profile,
        'horizontal_profile': horizontal_profile,
    }


def calculate_effective_roi(images: List[np.ndarray], 
                           safety_margin: float = 0.1) -> Tuple[int, int, int, int]:
    """
    Вычисляет эффективную область (ROI) на основе анализа нескольких изображений.
    
    Находит область, которая:
    - Не пересвечена на всех изображениях
    - Не затемнена на всех изображениях
    - Имеет хороший контраст
    
    Args:
        images: Список изображений для анализа
        safety_margin: Запас (отступ от краев эффективной области)
    
    Returns:
        (y_start, y_end, x_start, x_end) - координаты ROI
    """
    h, w = images[0].shape
    
    # Создаем общую карту эффективности (пересечение эффективных областей всех изображений)
    effective_combined = np.ones((h, w), dtype=bool)
    
    for img in images:
        analysis = analyze_exposure_map(img)
        effective_combined &= analysis['effective_map']
    
    # Находим строки и столбцы, где есть эффективные пиксели
    effective_rows = np.any(effective_combined, axis=1)
    effective_cols = np.any(effective_combined, axis=0)
    
    # Находим границы эффективной области
    if np.any(effective_rows):
        y_indices = np.where(effective_rows)[0]
        y_start = y_indices[0]
        y_end = y_indices[-1] + 1
    else:
        # Если нет эффективных строк, используем среднюю часть
        y_start = h // 4
        y_end = 3 * h // 4
    
    if np.any(effective_cols):
        x_indices = np.where(effective_cols)[0]
        x_start = x_indices[0]
        x_end = x_indices[-1] + 1
    else:
        # Если нет эффективных столбцов, используем всю ширину
        x_start = 0
        x_end = w
    
    # Применяем запас безопасности (сужаем область)
    margin_y = int((y_end - y_start) * safety_margin)
    margin_x = int((x_end - x_start) * safety_margin)
    
    y_start = max(0, y_start + margin_y)
    y_end = min(h, y_end - margin_y)
    x_start = max(0, x_start + margin_x)
    x_end = min(w, x_end - margin_x)
    
    return (y_start, y_end, x_start, x_end)


def apply_roi_mask(image: np.ndarray, roi: Tuple[int, int, int, int]) -> np.ndarray:
    """
    Применяет маску ROI к изображению.
    
    Args:
        image: Исходное изображение
        roi: (y_start, y_end, x_start, x_end)
    
    Returns:
        Изображение с обнуленными областями вне ROI
    """
    y_start, y_end, x_start, x_end = roi
    masked = np.zeros_like(image)
    masked[y_start:y_end, x_start:x_end] = image[y_start:y_end, x_start:x_end]
    return masked


def detect_line_in_roi(white_bg: np.ndarray, foreground: np.ndarray,
                       roi: Tuple[int, int, int, int],
                       threshold: int = 30) -> np.ndarray:
    """
    Детекция линии только в эффективной области (ROI).
    
    Args:
        white_bg: Фон белого поля
        foreground: Изображение с линией
        roi: (y_start, y_end, x_start, x_end)
        threshold: Пороговое значение
    
    Returns:
        Маска линии
    """
    y_start, y_end, x_start, x_end = roi
    
    # Применяем простое вычитание только в ROI
    mask = np.zeros_like(white_bg, dtype=np.uint8)
    
    roi_white = white_bg[y_start:y_end, x_start:x_end].astype(np.int16)
    roi_fg = foreground[y_start:y_end, x_start:x_end].astype(np.int16)
    
    diff = roi_white - roi_fg
    roi_mask = (diff > threshold).astype(np.uint8) * 255
    
    mask[y_start:y_end, x_start:x_end] = roi_mask
    
    return mask


def test_on_all_scenarios(white_bg_path: str, scenario_dirs: Dict[str, str]) -> None:
    """
    Тестирует алгоритм на всех сценариях (прямая, влево, вправо, окончание).
    
    Args:
        white_bg_path: Путь к изображению белого фона
        scenario_dirs: Словарь {название: директория}
    """
    print(f"\n{'='*80}")
    print(f"🎯 ТЕСТИРОВАНИЕ АЛГОРИТМА НА ВСЕХ СЦЕНАРИЯХ")
    print(f"{'='*80}")
    
    # Загружаем белый фон
    white_bg = load_image(white_bg_path)
    print(f"\n📂 Белый фон: {Path(white_bg_path).name}")
    print(f"   Размер: {white_bg.shape[1]}×{white_bg.shape[0]} px")
    
    # Собираем все изображения для анализа эффективной области
    print(f"\n🔍 Анализ эффективной области...")
    all_images = [white_bg]
    
    for scenario_name, scenario_dir in scenario_dirs.items():
        scenario_path = Path(scenario_dir)
        if scenario_path.exists():
            images = sorted(scenario_path.glob("*.jpg"))[:5]  # Берем по 5 из каждой категории
            for img_path in images:
                all_images.append(load_image(str(img_path)))
    
    print(f"   Всего изображений для анализа: {len(all_images)}")
    
    # Вычисляем эффективную область
    roi = calculate_effective_roi(all_images, safety_margin=0.1)
    y_start, y_end, x_start, x_end = roi
    
    roi_height = y_end - y_start
    roi_width = x_end - x_start
    roi_percent = 100 * (roi_height * roi_width) / (white_bg.shape[0] * white_bg.shape[1])
    
    print(f"\n✅ ЭФФЕКТИВНАЯ ОБЛАСТЬ (ROI):")
    print(f"   Y: {y_start} - {y_end} (высота: {roi_height} px, {100*roi_height/white_bg.shape[0]:.1f}%)")
    print(f"   X: {x_start} - {x_end} (ширина: {roi_width} px, {100*roi_width/white_bg.shape[1]:.1f}%)")
    print(f"   Площадь ROI: {roi_percent:.1f}% от общей площади")
    
    # Анализ экспозиции белого фона
    bg_analysis = analyze_exposure_map(white_bg)
    print(f"\n📊 Анализ экспозиции белого фона:")
    print(f"   Пересвет: {bg_analysis['overexposed_percent']:.2f}%")
    print(f"   Затемнение: {bg_analysis['underexposed_percent']:.2f}%")
    print(f"   Эффективная область: {bg_analysis['effective_percent']:.2f}%")
    
    # Тестируем на каждом сценарии
    print(f"\n{'='*80}")
    print(f"🧪 ТЕСТИРОВАНИЕ ПО СЦЕНАРИЯМ")
    print(f"{'='*80}")
    
    results = {}
    
    for scenario_name, scenario_dir in scenario_dirs.items():
        scenario_path = Path(scenario_dir)
        if not scenario_path.exists():
            print(f"\n⚠️  {scenario_name}: директория не найдена")
            continue
        
        images = sorted(scenario_path.glob("*.jpg"))
        if not images:
            print(f"\n⚠️  {scenario_name}: нет изображений")
            continue
        
        # Берем несколько примеров
        test_images = images[::len(images)//3 if len(images) > 3 else 1][:3]
        
        print(f"\n📁 {scenario_name.upper()}:")
        print(f"   Всего изображений: {len(images)}")
        print(f"   Тестируем: {len(test_images)}")
        
        scenario_results = []
        
        for img_path in test_images:
            fg = load_image(str(img_path))
            
            # Детекция в ROI
            mask_roi = detect_line_in_roi(white_bg, fg, roi)
            
            # Детекция без ROI (для сравнения)
            diff_full = white_bg.astype(np.int16) - fg.astype(np.int16)
            mask_full = (diff_full > 30).astype(np.uint8) * 255
            
            # Оценка качества
            coverage_roi = 100 * np.sum(mask_roi > 0) / mask_roi.size
            coverage_full = 100 * np.sum(mask_full > 0) / mask_full.size
            
            # Количество компонент в ROI
            num_labels_roi, _, _, _ = cv2.connectedComponentsWithStats(mask_roi, connectivity=8)
            num_components_roi = num_labels_roi - 1
            
            scenario_results.append({
                'image': img_path.name,
                'foreground': fg,
                'mask_roi': mask_roi,
                'mask_full': mask_full,
                'coverage_roi': coverage_roi,
                'coverage_full': coverage_full,
                'components_roi': num_components_roi,
            })
        
        results[scenario_name] = scenario_results
    
    # Создаем визуализацию
    create_roi_visualization(white_bg, roi, results, bg_analysis)
    
    # Выводы
    print(f"\n{'='*80}")
    print(f"💡 ВЫВОДЫ")
    print(f"{'='*80}")
    print(f"\n✅ Эффективная область определена:")
    print(f"   • ROI: Y[{y_start}:{y_end}], X[{x_start}:{x_end}]")
    print(f"   • Площадь: {roi_percent:.1f}% от общей")
    print(f"   • Исключены области с пересветом и затемнением")
    print(f"\n✅ Алгоритм протестирован на всех сценариях:")
    for scenario_name in results:
        print(f"   • {scenario_name}: {len(results[scenario_name])} примеров")
    
    print(f"\n💻 КОД ДЛЯ ESP32:")
    print(f"```cpp")
    print(f"// Эффективная область (ROI)")
    print(f"const int ROI_Y_START = {y_start};")
    print(f"const int ROI_Y_END = {y_end};")
    print(f"const int ROI_X_START = {x_start};")
    print(f"const int ROI_X_END = {x_end};")
    print(f"")
    print(f"// Сканирование только в ROI")
    print(f"for (int y = ROI_Y_START; y < ROI_Y_END; y++) {{")
    print(f"    for (int x = ROI_X_START; x < ROI_X_END; x++) {{")
    print(f"        int i = y * 160 + x;")
    print(f"        int16_t diff = white_bg[i] - current[i];")
    print(f"        mask[i] = (diff > 30) ? 255 : 0;")
    print(f"    }}")
    print(f"}}")
    print(f"```")
    
    print(f"\n{'='*80}\n")


def create_roi_visualization(white_bg: np.ndarray, roi: Tuple[int, int, int, int],
                             results: Dict, bg_analysis: Dict) -> None:
    """Создает визуализацию эффективной области и результатов."""
    try:
        y_start, y_end, x_start, x_end = roi
        
        # Подсчитываем количество примеров
        total_examples = sum(len(v) for v in results.values())
        num_scenarios = len(results)
        
        fig = plt.figure(figsize=(20, 4 + 3 * num_scenarios))
        gs = gridspec.GridSpec(2 + num_scenarios, 5, height_ratios=[2, 1] + [2] * num_scenarios)
        
        fig.suptitle('Анализ эффективной области (ROI) и тестирование на всех сценариях',
                    fontsize=16, fontweight='bold')
        
        # Строка 1: Анализ белого фона
        ax_bg = fig.add_subplot(gs[0, 0])
        ax_bg.imshow(white_bg, cmap='gray', vmin=0, vmax=255)
        rect = patches.Rectangle((x_start, y_start), x_end - x_start, y_end - y_start,
                                 linewidth=2, edgecolor='green', facecolor='none')
        ax_bg.add_patch(rect)
        ax_bg.set_title('Белый фон\n+ ROI (зеленый)', fontsize=10, fontweight='bold')
        ax_bg.axis('off')
        
        # Карта пересвета
        ax_over = fig.add_subplot(gs[0, 1])
        ax_over.imshow(bg_analysis['overexposed_map'], cmap='Reds', vmin=0, vmax=1)
        ax_over.set_title(f'Пересвет\n{bg_analysis["overexposed_percent"]:.1f}%', fontsize=10)
        ax_over.axis('off')
        
        # Карта затемнения
        ax_under = fig.add_subplot(gs[0, 2])
        ax_under.imshow(bg_analysis['underexposed_map'], cmap='Blues', vmin=0, vmax=1)
        ax_under.set_title(f'Затемнение\n{bg_analysis["underexposed_percent"]:.1f}%', fontsize=10)
        ax_under.axis('off')
        
        # Эффективная область
        ax_eff = fig.add_subplot(gs[0, 3])
        ax_eff.imshow(bg_analysis['effective_map'], cmap='Greens', vmin=0, vmax=1)
        ax_eff.set_title(f'Эффективная\n{bg_analysis["effective_percent"]:.1f}%', fontsize=10)
        ax_eff.axis('off')
        
        # ROI с белым фоном
        ax_roi = fig.add_subplot(gs[0, 4])
        roi_img = apply_roi_mask(white_bg, roi)
        ax_roi.imshow(roi_img, cmap='gray', vmin=0, vmax=255)
        ax_roi.set_title(f'ROI область\n{100*(y_end-y_start)*(x_end-x_start)/(white_bg.size):.1f}%', fontsize=10)
        ax_roi.axis('off')
        
        # Строка 2: Профили
        ax_vprofile = fig.add_subplot(gs[1, :2])
        ax_vprofile.plot(bg_analysis['vertical_profile'], range(len(bg_analysis['vertical_profile'])), 'b-', linewidth=2)
        ax_vprofile.axhline(y=y_start, color='green', linestyle='--', label='ROI границы')
        ax_vprofile.axhline(y=y_end, color='green', linestyle='--')
        ax_vprofile.axhspan(y_start, y_end, alpha=0.2, color='green')
        ax_vprofile.set_xlabel('Средняя яркость', fontsize=9)
        ax_vprofile.set_ylabel('Y (высота)', fontsize=9)
        ax_vprofile.set_title('Вертикальный профиль яркости', fontsize=10, fontweight='bold')
        ax_vprofile.legend(fontsize=8)
        ax_vprofile.grid(True, alpha=0.3)
        ax_vprofile.invert_yaxis()
        
        ax_hprofile = fig.add_subplot(gs[1, 2:])
        ax_hprofile.plot(bg_analysis['horizontal_profile'], 'r-', linewidth=2)
        ax_hprofile.axvline(x=x_start, color='green', linestyle='--', label='ROI границы')
        ax_hprofile.axvline(x=x_end, color='green', linestyle='--')
        ax_hprofile.axvspan(x_start, x_end, alpha=0.2, color='green')
        ax_hprofile.set_xlabel('X (ширина)', fontsize=9)
        ax_hprofile.set_ylabel('Средняя яркость', fontsize=9)
        ax_hprofile.set_title('Горизонтальный профиль яркости', fontsize=10, fontweight='bold')
        ax_hprofile.legend(fontsize=8)
        ax_hprofile.grid(True, alpha=0.3)
        
        # Строки 3+: Результаты по сценариям
        row = 2
        for scenario_name, scenario_results in results.items():
            for i, result in enumerate(scenario_results[:5]):  # Максимум 5 примеров
                ax = fig.add_subplot(gs[row, i])
                
                # Композит: foreground с маской ROI
                composite = cv2.cvtColor(result['foreground'], cv2.COLOR_GRAY2RGB)
                mask = result['mask_roi']
                composite[mask > 0] = [255, 0, 0]
                
                # Рисуем границу ROI
                composite[y_start, x_start:x_end] = [0, 255, 0]
                composite[y_end-1, x_start:x_end] = [0, 255, 0]
                composite[y_start:y_end, x_start] = [0, 255, 0]
                composite[y_start:y_end, x_end-1] = [0, 255, 0]
                
                ax.imshow(composite)
                ax.set_title(f"{scenario_name}\n{result['image'][:15]}\nROI: {result['coverage_roi']:.1f}%",
                           fontsize=8)
                ax.axis('off')
            
            row += 1
        
        plt.tight_layout()
        
        output_path = OUTPUT_DIR / 'roi_analysis_all_scenarios.png'
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
        description='Анализ эффективной области и тестирование на всех сценариях',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:

  1. Анализ с автопоиском директорий:
     python3 test_effective_roi.py white_bg.jpg

  2. С указанием директорий:
     python3 test_effective_roi.py white_bg.jpg \\
         --straight data/img_straight \\
         --left data/img_left \\
         --right data/img_right \\
         --terminate data/img_terminate
        """
    )
    
    parser.add_argument('white_bg', help='Путь к изображению белого фона')
    parser.add_argument('--straight', default='data/img_straight', help='Директория с прямой линией')
    parser.add_argument('--left', default='data/img_left', help='Директория с поворотом влево')
    parser.add_argument('--right', default='data/img_right', help='Директория с поворотом вправо')
    parser.add_argument('--terminate', default='data/img_terminate', help='Директория с окончанием линии')
    
    args = parser.parse_args()
    
    # Проверяем белый фон
    if not Path(args.white_bg).exists():
        print(f"❌ Файл не найден: {args.white_bg}")
        return 1
    
    # Собираем сценарии
    scenarios = {
        'Прямая': args.straight,
        'Влево': args.left,
        'Вправо': args.right,
        'Окончание': args.terminate,
    }
    
    test_on_all_scenarios(args.white_bg, scenarios)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
