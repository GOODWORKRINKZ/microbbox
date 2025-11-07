#!/usr/bin/env python3
"""
Скрипт для сравнения распознавания линии при разной высоте камеры.

Анализирует изображения:
- Стандартная высота камеры
- Камера поднята на +2 см (увеличенный обзор)

Цель: определить, улучшает ли увеличение высоты камеры распознавание линии.
"""

import os
import sys
from pathlib import Path
from PIL import Image
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

# Добавляем путь к модулю с алгоритмом
sys.path.insert(0, str(Path(__file__).parent))

# Импортируем алгоритм распознавания
from test_line_detection import detect_line_position

# Константы
OUTPUT_DIR = Path(__file__).parent / 'output'
OUTPUT_DIR.mkdir(exist_ok=True)


def analyze_image(image_path, label=""):
    """
    Анализирует изображение с помощью алгоритма распознавания.
    
    Args:
        image_path: Путь к изображению
        label: Метка для вывода
    
    Returns:
        dict: Результаты анализа
    """
    print(f"\n{'='*80}")
    print(f"🔍 Анализ: {label}")
    print(f"📁 Файл: {image_path}")
    print(f"{'='*80}")
    
    try:
        result = detect_line_position(str(image_path))
        
        print(f"\n📊 Результаты распознавания:")
        print(f"  ✅ Линия обнаружена: {result['detected']}")
        print(f"  📍 Позиция: {result['position']:.3f}")
        print(f"     (< 0 = слева, > 0 = справа, ≈ 0 = по центру)")
        print(f"  📈 Тренд направления: {result['direction_trend']:.3f}")
        print(f"  📏 Ширина линии: {result['width_percent']:.1f}%")
        
        # Определяем действие робота
        pos = result['position']
        trend = result['direction_trend']
        
        if abs(pos) < 0.15 and abs(trend) < 0.2:
            action = "STRAIGHT (прямо)"
        elif pos < -0.15 or trend < -0.2:
            action = "LEFT (влево)"
        elif pos > 0.15 or trend > 0.2:
            action = "RIGHT (вправо)"
        else:
            action = "STRAIGHT (прямо, небольшая коррекция)"
        
        print(f"\n🤖 Действие робота: {action}")
        
        # Анализируем горизонтальные сканы
        print(f"\n🔬 Горизонтальные сканы (позиция линии на разных высотах):")
        h_scans = result.get('horizontal_scans', [])
        heights = ['25% (далеко)', '50%', '75%', '90% (близко)']
        for i, (height, scan) in enumerate(zip(heights, h_scans)):
            fill = scan.get('fill_percent', 0)
            pos_scan = scan.get('position', 0)
            print(f"  • {height}: позиция={pos_scan:+.3f}, заполнение={fill:.1f}%")
        
        # Анализируем вертикальные сканы
        print(f"\n📊 Вертикальные сканы (высота линии в разных позициях):")
        v_scans = result.get('vertical_scans', [])
        positions = ['20% (слева)', '40%', '60%', '80% (справа)']
        for i, (pos_label, scan) in enumerate(zip(positions, v_scans)):
            fill = scan.get('fill_percent', 0)
            print(f"  • {pos_label}: заполнение={fill:.1f}%")
        
        print(f"\n{'='*80}\n")
        
        return result
        
    except Exception as e:
        print(f"❌ Ошибка при анализе: {e}")
        import traceback
        traceback.print_exc()
        return None


def compare_heights(standard_path, raised_path):
    """
    Сравнивает распознавание на двух высотах камеры.
    
    Args:
        standard_path: Путь к изображению со стандартной высоты
        raised_path: Путь к изображению с поднятой камеры (+2см)
    """
    print("\n" + "="*80)
    print("🎯 СРАВНЕНИЕ ВЫСОТЫ КАМЕРЫ")
    print("="*80)
    print(f"📷 Стандартная высота: {standard_path}")
    print(f"📷 Поднята на +2см: {raised_path}")
    print("="*80 + "\n")
    
    # Анализируем оба изображения
    result_standard = analyze_image(standard_path, "Стандартная высота камеры")
    result_raised = analyze_image(raised_path, "Камера поднята на +2см")
    
    if result_standard is None or result_raised is None:
        print("❌ Не удалось проанализировать изображения")
        return
    
    # Сравниваем результаты
    print("\n" + "="*80)
    print("📊 СРАВНИТЕЛЬНЫЙ АНАЛИЗ")
    print("="*80)
    
    print("\n1️⃣ Обнаружение линии:")
    print(f"  Стандартная: {'✅ Обнаружена' if result_standard['detected'] else '❌ Не обнаружена'}")
    print(f"  Поднятая:    {'✅ Обнаружена' if result_raised['detected'] else '❌ Не обнаружена'}")
    
    print("\n2️⃣ Позиция линии:")
    pos_std = result_standard['position']
    pos_raised = result_raised['position']
    print(f"  Стандартная: {pos_std:+.3f}")
    print(f"  Поднятая:    {pos_raised:+.3f}")
    print(f"  Разница:     {abs(pos_raised - pos_std):.3f}")
    
    print("\n3️⃣ Тренд направления:")
    trend_std = result_standard['direction_trend']
    trend_raised = result_raised['direction_trend']
    print(f"  Стандартная: {trend_std:+.3f}")
    print(f"  Поднятая:    {trend_raised:+.3f}")
    print(f"  Разница:     {abs(trend_raised - trend_std):.3f}")
    
    print("\n4️⃣ Ширина линии:")
    width_std = result_standard['width_percent']
    width_raised = result_raised['width_percent']
    print(f"  Стандартная: {width_std:.1f}%")
    print(f"  Поднятая:    {width_raised:.1f}%")
    print(f"  Изменение:   {width_raised - width_std:+.1f}%")
    
    # Создаем визуализацию
    create_comparison_visualization(standard_path, raised_path, 
                                   result_standard, result_raised)
    
    # Выводы
    print("\n" + "="*80)
    print("💡 ВЫВОДЫ")
    print("="*80)
    
    improvements = []
    concerns = []
    
    # Анализ ширины линии
    if width_raised > width_std * 1.1:
        improvements.append(f"✅ Ширина линии увеличена на {width_raised - width_std:.1f}% - лучший обзор")
    elif width_raised < width_std * 0.9:
        concerns.append(f"⚠️ Ширина линии уменьшена на {width_std - width_raised:.1f}% - может быть сложнее обнаружить")
    
    # Анализ стабильности позиции
    pos_diff = abs(pos_raised - pos_std)
    if pos_diff < 0.05:
        improvements.append("✅ Позиция линии стабильна - хорошая согласованность")
    elif pos_diff > 0.2:
        concerns.append(f"⚠️ Позиция линии сильно отличается ({pos_diff:.3f}) - может влиять на управление")
    
    # Анализ тренда
    trend_diff = abs(trend_raised - trend_std)
    if trend_diff < 0.1:
        improvements.append("✅ Тренд направления согласован - стабильное предсказание")
    elif trend_diff > 0.3:
        concerns.append(f"⚠️ Тренд направления сильно отличается ({trend_diff:.3f})")
    
    # Выводим результаты
    if improvements:
        print("\n🎯 Преимущества:")
        for imp in improvements:
            print(f"  {imp}")
    
    if concerns:
        print("\n⚠️ Потенциальные проблемы:")
        for con in concerns:
            print(f"  {con}")
    
    # Общая рекомендация
    print("\n🏆 РЕКОМЕНДАЦИЯ:")
    if len(improvements) > len(concerns):
        print("  ✅ Поднятие камеры на +2см УЛУЧШАЕТ распознавание")
        print("  💡 Рекомендуется использовать увеличенную высоту")
    elif len(concerns) > len(improvements):
        print("  ⚠️ Поднятие камеры на +2см может УХУДШИТЬ распознавание")
        print("  💡 Рекомендуется оставить стандартную высоту или протестировать больше")
    else:
        print("  🤔 Результаты смешанные - требуется дополнительное тестирование")
        print("  💡 Протестируйте на разных участках трассы")
    
    print("="*80 + "\n")


def create_comparison_visualization(path1, path2, result1, result2):
    """
    Создает визуализацию сравнения двух изображений.
    """
    try:
        fig = plt.figure(figsize=(16, 10))
        gs = gridspec.GridSpec(3, 2, height_ratios=[2, 1, 1])
        
        # Загружаем изображения
        img1 = Image.open(path1)
        img2 = Image.open(path2)
        
        # Оригинальные изображения
        ax1 = fig.add_subplot(gs[0, 0])
        ax1.imshow(img1)
        ax1.set_title(f"Стандартная высота\nPos: {result1['position']:.3f}, Trend: {result1['direction_trend']:.3f}", 
                     fontsize=12, fontweight='bold')
        ax1.axis('off')
        
        ax2 = fig.add_subplot(gs[0, 1])
        ax2.imshow(img2)
        ax2.set_title(f"Камера +2см выше\nPos: {result2['position']:.3f}, Trend: {result2['direction_trend']:.3f}", 
                     fontsize=12, fontweight='bold')
        ax2.axis('off')
        
        # График горизонтальных сканов
        ax3 = fig.add_subplot(gs[1, :])
        heights = [25, 50, 75, 90]
        h_scans1 = result1.get('horizontal_scans', [])
        h_scans2 = result2.get('horizontal_scans', [])
        
        pos1 = [scan.get('position', 0) for scan in h_scans1]
        pos2 = [scan.get('position', 0) for scan in h_scans2]
        
        ax3.plot(heights, pos1, 'o-', label='Стандартная', linewidth=2, markersize=8)
        ax3.plot(heights, pos2, 's-', label='Поднятая +2см', linewidth=2, markersize=8)
        ax3.axhline(y=0, color='gray', linestyle='--', alpha=0.5)
        ax3.set_xlabel('Высота скана (%)', fontsize=11)
        ax3.set_ylabel('Позиция линии', fontsize=11)
        ax3.set_title('Позиция линии на разных высотах', fontsize=12, fontweight='bold')
        ax3.legend(fontsize=10)
        ax3.grid(True, alpha=0.3)
        
        # График вертикальных сканов
        ax4 = fig.add_subplot(gs[2, :])
        positions = [20, 40, 60, 80]
        v_scans1 = result1.get('vertical_scans', [])
        v_scans2 = result2.get('vertical_scans', [])
        
        fill1 = [scan.get('fill_percent', 0) for scan in v_scans1]
        fill2 = [scan.get('fill_percent', 0) for scan in v_scans2]
        
        width = 8
        ax4.bar([p - width/2 for p in positions], fill1, width=width, label='Стандартная', alpha=0.8)
        ax4.bar([p + width/2 for p in positions], fill2, width=width, label='Поднятая +2см', alpha=0.8)
        ax4.set_xlabel('Позиция скана (% от ширины)', fontsize=11)
        ax4.set_ylabel('Заполнение (%)', fontsize=11)
        ax4.set_title('Высота линии в разных позициях (вертикальные сканы)', fontsize=12, fontweight='bold')
        ax4.legend(fontsize=10)
        ax4.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        
        output_path = OUTPUT_DIR / 'camera_height_comparison.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"\n📊 Визуализация сохранена: {output_path}")
        plt.close()
        
    except Exception as e:
        print(f"⚠️ Не удалось создать визуализацию: {e}")


def main():
    """Основная функция."""
    import sys
    
    if len(sys.argv) < 3:
        print("❌ Использование:")
        print(f"  {sys.argv[0]} <стандартное_изображение> <изображение_+2см>")
        print("\nПример:")
        print(f"  {sys.argv[0]} data/img_straight/straight1.jpg data/img_straight/straight1+2sm.jpg")
        sys.exit(1)
    
    standard_path = Path(sys.argv[1])
    raised_path = Path(sys.argv[2])
    
    if not standard_path.exists():
        print(f"❌ Файл не найден: {standard_path}")
        sys.exit(1)
    
    if not raised_path.exists():
        print(f"❌ Файл не найден: {raised_path}")
        sys.exit(1)
    
    compare_heights(standard_path, raised_path)


if __name__ == '__main__':
    main()
