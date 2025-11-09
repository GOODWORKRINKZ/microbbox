#!/usr/bin/env python3
"""
Оптимизированный алгоритм детекции линии с использованием сканирующих линий в ROI.

Вместо попиксельного сканирования всей области, используем горизонтальные
сканирующие линии только в зоне высокой эффективности (ROI).

Преимущества:
- Значительно быстрее (анализируем только 10-15% пикселей вместо 60%)
- Достаточно для определения положения линии
- Идеально подходит для ESP32

Метод:
1. Используем ROI Y[11:102], X[16:144]
2. Берем 10-12 горизонтальных сканирующих линий в ROI
3. На каждой линии находим позицию линии
4. Усредняем результаты
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
import time

# Константы
OUTPUT_DIR = Path(__file__).parent / 'output'
OUTPUT_DIR.mkdir(exist_ok=True)

# ROI из предыдущего анализа
ROI_Y_START = 11
ROI_Y_END = 102
ROI_X_START = 16
ROI_X_END = 144

# Параметры сканирующих линий
NUM_SCAN_LINES = 12  # Количество горизонтальных линий для сканирования


def load_image(image_path: str) -> np.ndarray:
    """Загружает изображение в grayscale."""
    img = Image.open(image_path)
    if img.mode != 'L':
        img = img.convert('L')
    return np.array(img, dtype=np.uint8)


def get_scan_lines() -> List[int]:
    """
    Возвращает Y-координаты сканирующих линий в ROI.
    
    Returns:
        Список Y-координат равномерно распределенных в ROI
    """
    # Равномерно распределяем линии в ROI
    scan_lines = np.linspace(ROI_Y_START, ROI_Y_END - 1, NUM_SCAN_LINES, dtype=int)
    return scan_lines.tolist()


def detect_line_on_scanline(white_bg: np.ndarray, foreground: np.ndarray,
                             y: int, threshold: int = 30) -> Tuple[float, bool]:
    """
    Детектирует позицию линии на одной горизонтальной сканирующей линии.
    
    Args:
        white_bg: Фон белого поля
        foreground: Изображение с линией
        y: Y-координата сканирующей линии
        threshold: Пороговое значение
    
    Returns:
        (position, detected) где position в диапазоне [-1, 1], detected - найдена ли линия
    """
    # Извлекаем строку только в ROI
    bg_line = white_bg[y, ROI_X_START:ROI_X_END].astype(np.int16)
    fg_line = foreground[y, ROI_X_START:ROI_X_END].astype(np.int16)
    
    # Вычитание
    diff = bg_line - fg_line
    
    # Бинаризация
    mask = diff > threshold
    
    if not np.any(mask):
        return (0.0, False)  # Линия не найдена
    
    # Находим центр масс линии
    indices = np.where(mask)[0]
    center = np.mean(indices)
    
    # Нормализуем к диапазону [-1, 1]
    # 0 = центр, -1 = левый край ROI, +1 = правый край ROI
    roi_width = ROI_X_END - ROI_X_START
    position = (center - roi_width / 2) / (roi_width / 2)
    
    return (position, True)


def detect_line_optimized(white_bg: np.ndarray, foreground: np.ndarray,
                         threshold: int = 30) -> Dict:
    """
    Оптимизированная детекция линии с использованием сканирующих линий.
    
    Args:
        white_bg: Фон белого поля
        foreground: Изображение с линией
        threshold: Пороговое значение
    
    Returns:
        Словарь с результатами детекции
    """
    scan_lines = get_scan_lines()
    
    positions = []
    detected_lines = []
    
    for y in scan_lines:
        pos, detected = detect_line_on_scanline(white_bg, foreground, y, threshold)
        if detected:
            positions.append(pos)
            detected_lines.append(y)
    
    if not positions:
        return {
            'position': 0.0,
            'detected': False,
            'confidence': 0.0,
            'scan_lines': scan_lines,
            'detected_lines': [],
            'positions': [],
        }
    
    # Средняя позиция
    mean_position = np.mean(positions)
    
    # Уверенность = процент линий, где найдена линия
    confidence = len(positions) / len(scan_lines)
    
    return {
        'position': mean_position,
        'detected': True,
        'confidence': confidence,
        'scan_lines': scan_lines,
        'detected_lines': detected_lines,
        'positions': positions,
    }


def classify_scenario(position: float, confidence: float) -> str:
    """
    Классифицирует сценарий на основе позиции и уверенности.
    
    Args:
        position: Позиция линии [-1, 1]
        confidence: Уверенность [0, 1]
    
    Returns:
        Название сценария
    """
    if confidence < 0.3:
        return "Окончание линии"
    elif position < -0.15:
        return "Поворот влево"
    elif position > 0.15:
        return "Поворот вправо"
    else:
        return "Прямо"


def benchmark_algorithm(white_bg: np.ndarray, test_images: List[np.ndarray],
                       scenario_names: List[str]) -> Dict:
    """
    Тестирует алгоритм на наборе изображений.
    
    Args:
        white_bg: Фон белого поля
        test_images: Список тестовых изображений
        scenario_names: Список имен сценариев
    
    Returns:
        Статистика производительности
    """
    results = []
    timings = []
    
    for fg, name in zip(test_images, scenario_names):
        start_time = time.perf_counter()
        result = detect_line_optimized(white_bg, fg)
        end_time = time.perf_counter()
        
        elapsed_ms = (end_time - start_time) * 1000
        timings.append(elapsed_ms)
        
        result['image_name'] = name
        result['elapsed_ms'] = elapsed_ms
        result['scenario'] = classify_scenario(result['position'], result['confidence'])
        
        results.append(result)
    
    return {
        'results': results,
        'mean_time_ms': np.mean(timings),
        'std_time_ms': np.std(timings),
        'min_time_ms': np.min(timings),
        'max_time_ms': np.max(timings),
    }


def run_full_test(white_bg_path: str, scenario_dirs: Dict[str, str]) -> None:
    """
    Запускает полный тест алгоритма на всех сценариях.
    
    Args:
        white_bg_path: Путь к белому фону
        scenario_dirs: Словарь {название: директория}
    """
    print(f"\n{'='*80}")
    print(f"🚀 ТЕСТИРОВАНИЕ ОПТИМИЗИРОВАННОГО АЛГОРИТМА")
    print(f"{'='*80}")
    
    # Загружаем белый фон
    white_bg = load_image(white_bg_path)
    print(f"\n📂 Белый фон: {Path(white_bg_path).name}")
    
    # Параметры
    print(f"\n⚙️  ПАРАМЕТРЫ АЛГОРИТМА:")
    print(f"   ROI: Y[{ROI_Y_START}:{ROI_Y_END}], X[{ROI_X_START}:{ROI_X_END}]")
    print(f"   Площадь ROI: 60.7% от общей")
    print(f"   Сканирующих линий: {NUM_SCAN_LINES}")
    
    # Вычисляем процент пикселей
    roi_pixels = (ROI_Y_END - ROI_Y_START) * (ROI_X_END - ROI_X_START)
    scan_pixels = NUM_SCAN_LINES * (ROI_X_END - ROI_X_START)
    scan_percent = 100 * scan_pixels / (160 * 120)
    
    print(f"   Анализируемых пикселей: {scan_percent:.1f}% от общей площади")
    print(f"   (вместо 60.7% в полном ROI)")
    
    # Собираем тестовые изображения
    all_test_images = []
    all_scenario_names = []
    scenario_stats = {}
    
    print(f"\n{'='*80}")
    print(f"📊 ЗАГРУЗКА ТЕСТОВЫХ ДАННЫХ")
    print(f"{'='*80}")
    
    for scenario_name, scenario_dir in scenario_dirs.items():
        scenario_path = Path(scenario_dir)
        if not scenario_path.exists():
            continue
        
        images = sorted(scenario_path.glob("*.jpg"))
        if not images:
            continue
        
        print(f"\n📁 {scenario_name}: {len(images)} изображений")
        
        for img_path in images:
            fg = load_image(str(img_path))
            all_test_images.append(fg)
            all_scenario_names.append(f"{scenario_name}/{img_path.name}")
        
        scenario_stats[scenario_name] = len(images)
    
    total_images = len(all_test_images)
    print(f"\n✅ Всего загружено: {total_images} изображений")
    
    # Запускаем тестирование
    print(f"\n{'='*80}")
    print(f"🧪 ЗАПУСК ТЕСТИРОВАНИЯ")
    print(f"{'='*80}\n")
    
    benchmark_result = benchmark_algorithm(white_bg, all_test_images, all_scenario_names)
    
    # Анализ результатов
    print(f"\n{'='*80}")
    print(f"📈 РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ")
    print(f"{'='*80}")
    
    print(f"\n⏱️  ПРОИЗВОДИТЕЛЬНОСТЬ:")
    print(f"   Среднее время: {benchmark_result['mean_time_ms']:.3f} мс")
    print(f"   Std отклонение: {benchmark_result['std_time_ms']:.3f} мс")
    print(f"   Минимум: {benchmark_result['min_time_ms']:.3f} мс")
    print(f"   Максимум: {benchmark_result['max_time_ms']:.3f} мс")
    
    # Эквивалент на ESP32 @ 240 MHz (примерно в 10 раз медленнее чем Python на x86)
    esp32_time_ms = benchmark_result['mean_time_ms'] * 10
    print(f"\n   📱 Оценка для ESP32 @ 240 MHz: ~{esp32_time_ms:.1f} мс")
    print(f"   📊 FPS на ESP32: ~{1000/esp32_time_ms:.1f} кадров/сек")
    
    # Статистика по сценариям
    print(f"\n📊 СТАТИСТИКА ПО СЦЕНАРИЯМ:")
    
    scenario_classifications = {}
    for result in benchmark_result['results']:
        img_scenario = result['image_name'].split('/')[0]
        classified = result['scenario']
        
        if img_scenario not in scenario_classifications:
            scenario_classifications[img_scenario] = {}
        
        if classified not in scenario_classifications[img_scenario]:
            scenario_classifications[img_scenario][classified] = 0
        
        scenario_classifications[img_scenario][classified] += 1
    
    for scenario_name in scenario_dirs:
        if scenario_name not in scenario_classifications:
            continue
        
        print(f"\n   {scenario_name}:")
        total = scenario_stats[scenario_name]
        
        for classified, count in sorted(scenario_classifications[scenario_name].items()):
            percent = 100 * count / total
            print(f"      {classified}: {count}/{total} ({percent:.1f}%)")
    
    # Создаем визуализацию
    create_test_visualization(white_bg, benchmark_result, scenario_dirs)
    
    # Выводы
    print(f"\n{'='*80}")
    print(f"💡 ВЫВОДЫ")
    print(f"{'='*80}")
    
    print(f"\n✅ Алгоритм протестирован на {total_images} изображениях")
    print(f"✅ Среднее время обработки: {benchmark_result['mean_time_ms']:.3f} мс (Python)")
    print(f"✅ Оценка для ESP32: ~{esp32_time_ms:.1f} мс (~{1000/esp32_time_ms:.1f} FPS)")
    print(f"✅ Анализируется всего {scan_percent:.1f}% пикселей")
    
    print(f"\n💻 КОД ДЛЯ ESP32:")
    print(f"```cpp")
    print(f"// Сканирующие линии (Y-координаты)")
    scan_lines = get_scan_lines()
    print(f"const int scan_lines[] = {{{', '.join(map(str, scan_lines))}}};")
    print(f"const int num_scan_lines = {NUM_SCAN_LINES};")
    print(f"")
    print(f"float detect_line_position(uint8_t* white_bg, uint8_t* current) {{")
    print(f"    float sum_position = 0.0;")
    print(f"    int detected_count = 0;")
    print(f"    ")
    print(f"    for (int i = 0; i < num_scan_lines; i++) {{")
    print(f"        int y = scan_lines[i];")
    print(f"        ")
    print(f"        // Сканируем только в ROI")
    print(f"        int line_center = 0;")
    print(f"        int line_pixels = 0;")
    print(f"        ")
    print(f"        for (int x = {ROI_X_START}; x < {ROI_X_END}; x++) {{")
    print(f"            int idx = y * 160 + x;")
    print(f"            int16_t diff = white_bg[idx] - current[idx];")
    print(f"            ")
    print(f"            if (diff > 30) {{")
    print(f"                line_center += (x - {ROI_X_START});")
    print(f"                line_pixels++;")
    print(f"            }}")
    print(f"        }}")
    print(f"        ")
    print(f"        if (line_pixels > 0) {{")
    print(f"            float center = (float)line_center / line_pixels;")
    print(f"            float roi_width = {ROI_X_END - ROI_X_START};")
    print(f"            float position = (center - roi_width/2) / (roi_width/2);")
    print(f"            sum_position += position;")
    print(f"            detected_count++;")
    print(f"        }}")
    print(f"    }}")
    print(f"    ")
    print(f"    if (detected_count == 0) return 0.0;  // Линия не найдена")
    print(f"    return sum_position / detected_count;  // Средняя позиция")
    print(f"}}")
    print(f"```")
    
    print(f"\n{'='*80}\n")


def create_test_visualization(white_bg: np.ndarray, benchmark_result: Dict,
                              scenario_dirs: Dict) -> None:
    """Создает визуализацию результатов тестирования."""
    try:
        results = benchmark_result['results']
        
        # Группируем по сценариям
        scenario_results = {}
        for result in results:
            scenario = result['image_name'].split('/')[0]
            if scenario not in scenario_results:
                scenario_results[scenario] = []
            scenario_results[scenario].append(result)
        
        # Берем по 3 примера из каждого сценария
        num_scenarios = len(scenario_results)
        examples_per_scenario = 3
        
        fig = plt.figure(figsize=(20, 4 * num_scenarios))
        gs = gridspec.GridSpec(num_scenarios, examples_per_scenario + 1, 
                              width_ratios=[1, 1, 1, 0.5])
        
        fig.suptitle('Тестирование оптимизированного алгоритма с сканирующими линиями',
                    fontsize=16, fontweight='bold')
        
        scan_lines = get_scan_lines()
        
        row = 0
        for scenario_name in ['Прямая', 'Влево', 'Вправо', 'Окончание']:
            if scenario_name not in scenario_results:
                continue
            
            examples = scenario_results[scenario_name][:examples_per_scenario]
            
            for col, result in enumerate(examples):
                ax = fig.add_subplot(gs[row, col])
                
                # Загружаем foreground
                img_path = None
                for scenario_dir_name, scenario_dir in scenario_dirs.items():
                    if scenario_dir_name == scenario_name:
                        img_name = result['image_name'].split('/')[1]
                        img_path = Path(scenario_dir) / img_name
                        break
                
                if img_path and img_path.exists():
                    fg = load_image(str(img_path))
                    
                    # Рисуем изображение
                    composite = cv2.cvtColor(fg, cv2.COLOR_GRAY2RGB)
                    
                    # Рисуем ROI
                    cv2.rectangle(composite, 
                                (ROI_X_START, ROI_Y_START), 
                                (ROI_X_END-1, ROI_Y_END-1),
                                (0, 255, 0), 1)
                    
                    # Рисуем сканирующие линии
                    for y in scan_lines:
                        cv2.line(composite, 
                               (ROI_X_START, y), 
                               (ROI_X_END-1, y),
                               (255, 255, 0), 1)
                    
                    # Рисуем детектированные линии
                    for y in result['detected_lines']:
                        cv2.line(composite, 
                               (ROI_X_START, y), 
                               (ROI_X_END-1, y),
                               (255, 0, 0), 2)
                    
                    ax.imshow(composite)
                    
                    title = f"{result['scenario']}\n"
                    title += f"Поз: {result['position']:.2f}, "
                    title += f"Увер: {result['confidence']:.2f}\n"
                    title += f"Время: {result['elapsed_ms']:.2f} мс"
                    
                    ax.set_title(title, fontsize=9)
                    ax.axis('off')
            
            # Статистика сценария
            ax_stats = fig.add_subplot(gs[row, examples_per_scenario])
            ax_stats.axis('off')
            
            positions = [r['position'] for r in scenario_results[scenario_name]]
            confidences = [r['confidence'] for r in scenario_results[scenario_name]]
            times = [r['elapsed_ms'] for r in scenario_results[scenario_name]]
            
            stats_text = f"{scenario_name}\n"
            stats_text += f"───────────\n"
            stats_text += f"Всего: {len(scenario_results[scenario_name])}\n\n"
            stats_text += f"Позиция:\n"
            stats_text += f"  μ={np.mean(positions):.2f}\n"
            stats_text += f"  σ={np.std(positions):.2f}\n\n"
            stats_text += f"Уверенность:\n"
            stats_text += f"  μ={np.mean(confidences):.2f}\n"
            stats_text += f"  σ={np.std(confidences):.2f}\n\n"
            stats_text += f"Время:\n"
            stats_text += f"  μ={np.mean(times):.2f} мс\n"
            
            ax_stats.text(0.1, 0.5, stats_text, fontsize=10, family='monospace',
                        verticalalignment='center')
            
            row += 1
        
        plt.tight_layout()
        
        output_path = OUTPUT_DIR / 'optimized_algorithm_test.png'
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
        description='Тестирование оптимизированного алгоритма с сканирующими линиями',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:

  python3 test_optimized_algorithm.py white_bg.jpg \\
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
    
    run_full_test(args.white_bg, scenarios)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
