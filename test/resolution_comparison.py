#!/usr/bin/env python3
"""
Сравнение производительности при разных разрешениях.

Анализируем влияние разрешения на скорость работы алгоритма на ESP32.
"""

def analyze_resolution(width, height, name):
    """Анализирует производительность для заданного разрешения."""
    print(f"\n{'='*80}")
    print(f"📐 РАЗРЕШЕНИЕ: {width}×{height} ({name})")
    print('='*80)
    
    total_pixels = width * height
    print(f"\nОбщий размер изображения: {total_pixels:,} байт")
    
    # Позиции сканирующих линий
    scan_y = [
        height * 25 // 100,  # 25%
        height * 50 // 100,  # 50%
        height * 75 // 100,  # 75%
        height * 90 // 100,  # 90%
    ]
    
    scan_x = [
        width * 20 // 100,   # 20%
        width * 40 // 100,   # 40%
        width * 60 // 100,   # 60%
        width * 80 // 100,   # 80%
    ]
    
    # Операции
    h_reads = 4 * width    # 4 горизонтальные линии
    v_reads = 4 * height   # 4 вертикальные линии
    total_reads = h_reads + v_reads
    
    # Сравнения с порогом
    comparisons = total_reads  # По одному сравнению на пиксель
    
    # Аккумуляция (только для пикселей линии, ~10% от общего)
    accumulations = int(total_reads * 0.10 * 2)  # += операций для x и count
    
    # Вычисления результата
    calculations = (
        4 * 3 +   # Нормализация 4 позиций (деление, умножение, вычитание)
        4 * 5 +   # Вычисление 4 трендов
        3 +       # Поиск максимума
        2 +       # Комбинирование позиции и тренда
        1         # Проверка T-пересечения
    )
    
    total_ops = comparisons + accumulations + calculations
    
    print(f"\n📊 ОПЕРАЦИИ:")
    print(f"  Чтений памяти:           {total_reads:6,} байт")
    print(f"  Сравнений (< threshold): {comparisons:6,}")
    print(f"  Аккумуляций (+=):        {accumulations:6,}")
    print(f"  Вычислений (итого):      {calculations:6,}")
    print(f"  {'─'*40}")
    print(f"  ВСЕГО операций:          {total_ops:6,}")
    
    # Оценка для ESP32
    print(f"\n⚡ ПРОГНОЗ ДЛЯ ESP32 (240 MHz, оптимизированный C++):\n")
    
    # Предположения для ESP32:
    # - Чтение из L1 кэша: ~1-2 цикла
    # - Сравнение: 1 цикл
    # - Аккумуляция: 2-3 цикла
    # - Вычисления: 5-10 циклов каждое
    
    # Пессимистичный сценарий (плохой кэш, много промахов)
    cycles_pessimistic = (
        total_reads * 10 +      # Промахи кэша, чтение из RAM
        comparisons * 2 +       # Сравнение + переход
        accumulations * 4 +     # Аккумуляция с условием
        calculations * 15       # Сложные вычисления с делением
    )
    
    # Реалистичный сценарий (хороший кэш, оптимизация)
    cycles_realistic = (
        total_reads * 3 +       # Хорошая локальность кэша
        comparisons * 1 +       # Простое сравнение
        accumulations * 3 +     # Оптимизированная аккумуляция
        calculations * 10       # Умеренно сложные вычисления
    )
    
    # Оптимистичный сценарий (идеальный кэш, агрессивная оптимизация)
    cycles_optimistic = (
        total_reads * 2 +       # Все в кэше
        comparisons * 1 +       # Быстрое сравнение
        accumulations * 2 +     # Оптимальная аккумуляция
        calculations * 8        # Целочисленная арифметика
    )
    
    # Переводим циклы в время (240 MHz = 240,000,000 циклов/сек)
    cpu_freq = 240_000_000
    
    time_pessimistic_ms = (cycles_pessimistic / cpu_freq) * 1000
    time_realistic_ms = (cycles_realistic / cpu_freq) * 1000
    time_optimistic_ms = (cycles_optimistic / cpu_freq) * 1000
    
    fps_pessimistic = 1000 / time_pessimistic_ms
    fps_realistic = 1000 / time_realistic_ms
    fps_optimistic = 1000 / time_optimistic_ms
    
    print(f"  Сценарий           Циклы      Время (мс)    FPS")
    print(f"  {'─'*55}")
    print(f"  Пессимистичный   {cycles_pessimistic:8,}    {time_pessimistic_ms:6.2f}      {fps_pessimistic:5.1f}")
    print(f"  Реалистичный     {cycles_realistic:8,}    {time_realistic_ms:6.2f}      {fps_realistic:5.1f}")
    print(f"  Оптимистичный    {cycles_optimistic:8,}    {time_optimistic_ms:6.2f}      {fps_optimistic:5.1f}")
    
    # Вердикт
    if fps_realistic >= 30:
        verdict = "✅ ОТЛИЧНО - комфортное управление в реальном времени!"
    elif fps_realistic >= 25:
        verdict = "✅ ХОРОШО - плавное управление"
    elif fps_realistic >= 20:
        verdict = "⚠️  ПРИЕМЛЕМО - работает, но могут быть задержки"
    elif fps_realistic >= 15:
        verdict = "⚠️  ГРАНИЦА - минимально приемлемая скорость"
    else:
        verdict = "❌ МЕДЛЕННО - нужна дополнительная оптимизация"
    
    print(f"\n  {verdict}")
    
    return {
        'width': width,
        'height': height,
        'name': name,
        'total_pixels': total_pixels,
        'total_ops': total_ops,
        'total_reads': total_reads,
        'fps_pessimistic': fps_pessimistic,
        'fps_realistic': fps_realistic,
        'fps_optimistic': fps_optimistic,
        'time_realistic_ms': time_realistic_ms,
    }


def print_comparison(results):
    """Печатает сравнительную таблицу."""
    print(f"\n{'='*80}")
    print("📊 СРАВНЕНИЕ РАЗРЕШЕНИЙ")
    print('='*80)
    
    print(f"\n{'Разрешение':<15} {'Пиксели':>10} {'Операций':>12} {'FPS (реал.)':>12} {'Время (мс)':>12}")
    print('─'*80)
    
    for r in results:
        print(f"{r['name']:<15} {r['total_pixels']:>10,} {r['total_ops']:>12,} {r['fps_realistic']:>12.1f} {r['time_realistic_ms']:>12.2f}")
    
    # Сравнение с базовым разрешением (160×120)
    base = results[0]  # 160×120
    
    print(f"\n{'='*80}")
    print("⚡ УСКОРЕНИЕ ОТНОСИТЕЛЬНО 160×120:")
    print('='*80)
    
    for r in results[1:]:
        speedup_ops = base['total_ops'] / r['total_ops']
        speedup_fps = r['fps_realistic'] / base['fps_realistic']
        speedup_pixels = base['total_pixels'] / r['total_pixels']
        
        print(f"\n  {r['name']}:")
        print(f"    Пикселей:  {speedup_pixels:.2f}x меньше")
        print(f"    Операций:  {speedup_ops:.2f}x меньше")
        print(f"    FPS:       {speedup_fps:.2f}x быстрее")
        print(f"    Прирост:   +{r['fps_realistic'] - base['fps_realistic']:.1f} FPS")
    
    # Рекомендации
    print(f"\n{'='*80}")
    print("✅ РЕКОМЕНДАЦИИ:")
    print('='*80)
    
    print("\n  📐 ВЫБОР РАЗРЕШЕНИЯ:\n")
    
    for r in results:
        if r['fps_realistic'] >= 30:
            quality = "высокое качество, плавное управление"
        elif r['fps_realistic'] >= 25:
            quality = "хорошее качество, стабильная работа"
        elif r['fps_realistic'] >= 20:
            quality = "приемлемое качество, возможны задержки"
        else:
            quality = "низкое качество, медленная работа"
        
        if r['fps_realistic'] >= 30:
            emoji = "🥇"
        elif r['fps_realistic'] >= 25:
            emoji = "🥈"
        elif r['fps_realistic'] >= 20:
            emoji = "🥉"
        else:
            emoji = "⚠️"
        
        print(f"    {emoji} {r['name']:<12} → {r['fps_realistic']:>5.1f} FPS  ({quality})")
    
    # Итоговая рекомендация
    best = max(results, key=lambda x: x['fps_realistic'] if x['fps_realistic'] < 60 else 0)
    
    print(f"\n  🎯 ОПТИМАЛЬНЫЙ ВЫБОР: {best['name']}")
    print(f"     • {best['fps_realistic']:.1f} FPS - достаточно для плавного управления")
    print(f"     • {best['total_pixels']:,} байт - экономия памяти")
    print(f"     • {best['time_realistic_ms']:.2f} мс на кадр - быстрая обработка")
    
    print(f"\n{'='*80}")


def main():
    """Главная функция."""
    print("\n" + "="*80)
    print("🔬 АНАЛИЗ ВЛИЯНИЯ РАЗРЕШЕНИЯ НА ПРОИЗВОДИТЕЛЬНОСТЬ")
    print("="*80)
    
    # Тестируем разные разрешения
    resolutions = [
        (160, 120, "160×120 (QQVGA)"),
        (128, 96,  "128×96"),
        (96, 96,   "96×96 (квадрат)"),
        (80, 60,   "80×60"),
        (64, 64,   "64×64 (минимум)"),
    ]
    
    results = []
    for width, height, name in resolutions:
        result = analyze_resolution(width, height, name)
        results.append(result)
    
    print_comparison(results)
    
    return 0


if __name__ == '__main__':
    exit(main())
