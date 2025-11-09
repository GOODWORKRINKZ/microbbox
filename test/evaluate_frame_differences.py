#!/usr/bin/env python3
"""
Скрипт для оценки различий между кадрами пустого поля.

Анализирует изображения, снятые в разных местах поля, и оценивает:
- Стабильность камеры (постоянство яркости, контраста)
- Шум матрицы
- Различия в освещении
- Однородность поверхности поля

Используется для калибровки и оценки качества работы камеры.
"""

import os
import sys
from pathlib import Path
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from typing import List, Dict, Tuple
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
        numpy массив с изображением (grayscale)
    """
    img = Image.open(image_path)
    
    # Конвертируем в grayscale если необходимо
    if img.mode != 'L':
        img = img.convert('L')
    
    return np.array(img, dtype=np.float32)


def calculate_mse(img1: np.ndarray, img2: np.ndarray) -> float:
    """
    Вычисляет среднеквадратичную ошибку (MSE) между двумя изображениями.
    
    Args:
        img1: Первое изображение
        img2: Второе изображение
    
    Returns:
        MSE значение
    """
    return np.mean((img1 - img2) ** 2)


def calculate_mae(img1: np.ndarray, img2: np.ndarray) -> float:
    """
    Вычисляет среднюю абсолютную ошибку (MAE) между двумя изображениями.
    
    Args:
        img1: Первое изображение
        img2: Второе изображение
    
    Returns:
        MAE значение
    """
    return np.mean(np.abs(img1 - img2))


def calculate_ssim(img1: np.ndarray, img2: np.ndarray) -> float:
    """
    Вычисляет индекс структурного сходства (SSIM) между двумя изображениями.
    
    Args:
        img1: Первое изображение
        img2: Второе изображение
    
    Returns:
        SSIM значение (от 0 до 1, где 1 = идентичные)
    """
    # Конвертируем в uint8 для opencv
    img1_uint8 = img1.astype(np.uint8)
    img2_uint8 = img2.astype(np.uint8)
    
    # Используем opencv для вычисления SSIM
    from cv2 import PSNR
    
    # Параметры для SSIM
    C1 = (0.01 * 255) ** 2
    C2 = (0.03 * 255) ** 2
    
    img1_sq = img1 * img1
    img2_sq = img2 * img2
    img1_img2 = img1 * img2
    
    mu1 = cv2.GaussianBlur(img1, (11, 11), 1.5)
    mu2 = cv2.GaussianBlur(img2, (11, 11), 1.5)
    
    mu1_sq = mu1 * mu1
    mu2_sq = mu2 * mu2
    mu1_mu2 = mu1 * mu2
    
    sigma1_sq = cv2.GaussianBlur(img1_sq, (11, 11), 1.5) - mu1_sq
    sigma2_sq = cv2.GaussianBlur(img2_sq, (11, 11), 1.5) - mu2_sq
    sigma12 = cv2.GaussianBlur(img1_img2, (11, 11), 1.5) - mu1_mu2
    
    ssim_map = ((2 * mu1_mu2 + C1) * (2 * sigma12 + C2)) / \
               ((mu1_sq + mu2_sq + C1) * (sigma1_sq + sigma2_sq + C2))
    
    return float(np.mean(ssim_map))


def calculate_histogram_correlation(img1: np.ndarray, img2: np.ndarray) -> float:
    """
    Вычисляет корреляцию гистограмм двух изображений.
    
    Args:
        img1: Первое изображение
        img2: Второе изображение
    
    Returns:
        Коэффициент корреляции (от -1 до 1, где 1 = идентичные распределения)
    """
    hist1 = np.histogram(img1, bins=256, range=(0, 255))[0]
    hist2 = np.histogram(img2, bins=256, range=(0, 255))[0]
    
    # Нормализуем гистограммы
    hist1 = hist1.astype(float) / hist1.sum()
    hist2 = hist2.astype(float) / hist2.sum()
    
    # Вычисляем корреляцию
    correlation = np.corrcoef(hist1, hist2)[0, 1]
    
    return float(correlation)


def analyze_image_stats(img: np.ndarray) -> Dict:
    """
    Анализирует статистические характеристики изображения.
    
    Args:
        img: Изображение
    
    Returns:
        Словарь со статистикой
    """
    return {
        'mean': float(np.mean(img)),
        'std': float(np.std(img)),
        'min': float(np.min(img)),
        'max': float(np.max(img)),
        'median': float(np.median(img)),
        'q25': float(np.percentile(img, 25)),
        'q75': float(np.percentile(img, 75)),
    }


def compare_two_frames(img1_path: str, img2_path: str, 
                       label1: str = "Кадр 1", label2: str = "Кадр 2") -> Dict:
    """
    Сравнивает два кадра и возвращает метрики различия.
    
    Args:
        img1_path: Путь к первому изображению
        img2_path: Путь к второму изображению
        label1: Метка первого изображения
        label2: Метка второго изображения
    
    Returns:
        Словарь с результатами сравнения
    """
    print(f"\n{'='*80}")
    print(f"🔍 Сравнение: {label1} ↔ {label2}")
    print(f"{'='*80}")
    
    # Загружаем изображения
    img1 = load_image(img1_path)
    img2 = load_image(img2_path)
    
    # Проверяем размеры
    if img1.shape != img2.shape:
        print(f"⚠️  ВНИМАНИЕ: Разные размеры изображений!")
        print(f"   {label1}: {img1.shape}")
        print(f"   {label2}: {img2.shape}")
        # Приводим к одному размеру
        min_h = min(img1.shape[0], img2.shape[0])
        min_w = min(img1.shape[1], img2.shape[1])
        img1 = img1[:min_h, :min_w]
        img2 = img2[:min_h, :min_w]
    
    # Вычисляем статистику каждого изображения
    stats1 = analyze_image_stats(img1)
    stats2 = analyze_image_stats(img2)
    
    # Вычисляем метрики различия
    mse = calculate_mse(img1, img2)
    mae = calculate_mae(img1, img2)
    ssim = calculate_ssim(img1, img2)
    hist_corr = calculate_histogram_correlation(img1, img2)
    
    # Вычисляем разностное изображение
    diff_img = np.abs(img1 - img2)
    diff_stats = analyze_image_stats(diff_img)
    
    # Вычисляем процент значительно различающихся пикселей (порог = 20)
    significant_diff_threshold = 20
    significant_diff_percent = 100 * np.sum(diff_img > significant_diff_threshold) / diff_img.size
    
    results = {
        'img1_path': img1_path,
        'img2_path': img2_path,
        'label1': label1,
        'label2': label2,
        'img1': img1,
        'img2': img2,
        'diff_img': diff_img,
        'stats1': stats1,
        'stats2': stats2,
        'diff_stats': diff_stats,
        'mse': mse,
        'mae': mae,
        'ssim': ssim,
        'hist_corr': hist_corr,
        'significant_diff_percent': significant_diff_percent,
    }
    
    # Выводим результаты
    print(f"\n📊 СТАТИСТИКА ИЗОБРАЖЕНИЙ:")
    print(f"\n  {label1}:")
    print(f"    Средняя яркость:  {stats1['mean']:7.2f}")
    print(f"    Станд. отклонение: {stats1['std']:7.2f}")
    print(f"    Диапазон:         {stats1['min']:7.2f} - {stats1['max']:7.2f}")
    print(f"    Медиана:          {stats1['median']:7.2f}")
    
    print(f"\n  {label2}:")
    print(f"    Средняя яркость:  {stats2['mean']:7.2f}")
    print(f"    Станд. отклонение: {stats2['std']:7.2f}")
    print(f"    Диапазон:         {stats2['min']:7.2f} - {stats2['max']:7.2f}")
    print(f"    Медиана:          {stats2['median']:7.2f}")
    
    print(f"\n📏 МЕТРИКИ РАЗЛИЧИЯ:")
    print(f"    MSE (среднекв. ошибка):      {mse:10.2f}")
    print(f"    MAE (средн. абс. ошибка):    {mae:10.2f}")
    print(f"    SSIM (структурное сходство): {ssim:10.4f}  (1.0 = идентичны)")
    print(f"    Корреляция гистограмм:       {hist_corr:10.4f}  (1.0 = идентичны)")
    
    print(f"\n🔬 АНАЛИЗ РАЗЛИЧИЙ:")
    print(f"    Средняя разница пикселей:    {diff_stats['mean']:7.2f}")
    print(f"    Макс. разница пикселей:      {diff_stats['max']:7.2f}")
    print(f"    Пикселей с разницей > 20:    {significant_diff_percent:6.2f}%")
    
    # Интерпретация результатов
    print(f"\n💡 ИНТЕРПРЕТАЦИЯ:")
    
    # SSIM анализ
    if ssim > 0.95:
        print(f"    ✅ Очень высокое сходство (SSIM={ssim:.4f})")
        print(f"       Кадры практически идентичны - отличная стабильность камеры")
    elif ssim > 0.85:
        print(f"    ✅ Высокое сходство (SSIM={ssim:.4f})")
        print(f"       Незначительные различия - хорошая стабильность")
    elif ssim > 0.70:
        print(f"    ⚠️  Умеренное сходство (SSIM={ssim:.4f})")
        print(f"       Заметные различия - возможны изменения освещения/позиции")
    else:
        print(f"    ❌ Низкое сходство (SSIM={ssim:.4f})")
        print(f"       Значительные различия - разные условия съемки")
    
    # MAE анализ
    if mae < 5:
        print(f"    ✅ Минимальная разница яркости (MAE={mae:.2f})")
    elif mae < 15:
        print(f"    ⚠️  Умеренная разница яркости (MAE={mae:.2f})")
    else:
        print(f"    ❌ Большая разница яркости (MAE={mae:.2f})")
    
    # Разница яркости
    brightness_diff = abs(stats1['mean'] - stats2['mean'])
    if brightness_diff < 5:
        print(f"    ✅ Стабильная средняя яркость (Δ={brightness_diff:.2f})")
    elif brightness_diff < 15:
        print(f"    ⚠️  Умеренное изменение яркости (Δ={brightness_diff:.2f})")
    else:
        print(f"    ❌ Значительное изменение яркости (Δ={brightness_diff:.2f})")
    
    print(f"\n{'='*80}\n")
    
    return results


def analyze_multiple_frames(image_paths: List[str], labels: List[str] = None) -> None:
    """
    Анализирует несколько кадров и создает общий отчет.
    
    Args:
        image_paths: Список путей к изображениям
        labels: Список меток для изображений
    """
    if labels is None:
        labels = [f"Кадр {i+1}" for i in range(len(image_paths))]
    
    print(f"\n{'='*80}")
    print(f"📊 АНАЛИЗ СЕРИИ ИЗ {len(image_paths)} КАДРОВ")
    print(f"{'='*80}")
    
    # Загружаем все изображения
    images = []
    stats_list = []
    
    for i, (path, label) in enumerate(zip(image_paths, labels)):
        print(f"\n{i+1}. Загрузка: {label}")
        print(f"   Файл: {Path(path).name}")
        img = load_image(path)
        images.append(img)
        stats = analyze_image_stats(img)
        stats_list.append(stats)
        print(f"   Яркость: {stats['mean']:.2f} ± {stats['std']:.2f}")
    
    # Вычисляем попарные сравнения
    print(f"\n{'='*80}")
    print(f"🔍 ПОПАРНЫЕ СРАВНЕНИЯ")
    print(f"{'='*80}")
    
    comparisons = []
    n = len(images)
    
    for i in range(n):
        for j in range(i + 1, n):
            result = compare_two_frames(
                image_paths[i], image_paths[j],
                labels[i], labels[j]
            )
            comparisons.append(result)
    
    # Общая статистика
    print(f"\n{'='*80}")
    print(f"📈 ОБЩАЯ СТАТИСТИКА ПО ВСЕМ КАДРАМ")
    print(f"{'='*80}")
    
    all_means = [s['mean'] for s in stats_list]
    all_stds = [s['std'] for s in stats_list]
    
    print(f"\nСредняя яркость:")
    print(f"  Минимум:          {min(all_means):7.2f}")
    print(f"  Максимум:         {max(all_means):7.2f}")
    print(f"  Среднее:          {np.mean(all_means):7.2f}")
    print(f"  Разброс (std):    {np.std(all_means):7.2f}")
    print(f"  Диапазон:         {max(all_means) - min(all_means):7.2f}")
    
    print(f"\nШум (станд. откл. внутри кадра):")
    print(f"  Минимум:          {min(all_stds):7.2f}")
    print(f"  Максимум:         {max(all_stds):7.2f}")
    print(f"  Среднее:          {np.mean(all_stds):7.2f}")
    
    # Статистика по сравнениям
    all_ssim = [c['ssim'] for c in comparisons]
    all_mae = [c['mae'] for c in comparisons]
    
    print(f"\nСтруктурное сходство (SSIM):")
    print(f"  Минимум:          {min(all_ssim):7.4f}")
    print(f"  Максимум:         {max(all_ssim):7.4f}")
    print(f"  Среднее:          {np.mean(all_ssim):7.4f}")
    
    print(f"\nСредняя абсолютная ошибка (MAE):")
    print(f"  Минимум:          {min(all_mae):7.2f}")
    print(f"  Максимум:         {max(all_mae):7.2f}")
    print(f"  Среднее:          {np.mean(all_mae):7.2f}")
    
    # Выводы
    print(f"\n{'='*80}")
    print(f"💡 ВЫВОДЫ")
    print(f"{'='*80}")
    
    brightness_variation = np.std(all_means)
    avg_ssim = np.mean(all_ssim)
    avg_mae = np.mean(all_mae)
    
    print(f"\n🎯 Оценка стабильности камеры:\n")
    
    # Яркость
    if brightness_variation < 3:
        print(f"  ✅ Отличная стабильность яркости (σ={brightness_variation:.2f})")
        print(f"     Освещение и экспозиция очень стабильны")
    elif brightness_variation < 8:
        print(f"  ✅ Хорошая стабильность яркости (σ={brightness_variation:.2f})")
        print(f"     Небольшие колебания, в норме для камеры")
    elif brightness_variation < 15:
        print(f"  ⚠️  Умеренная нестабильность яркости (σ={brightness_variation:.2f})")
        print(f"     Рекомендуется проверить освещение или автоэкспозицию")
    else:
        print(f"  ❌ Высокая нестабильность яркости (σ={brightness_variation:.2f})")
        print(f"     Значительные колебания - проверьте настройки камеры")
    
    # SSIM
    if avg_ssim > 0.90:
        print(f"\n  ✅ Высокое структурное сходство (avg SSIM={avg_ssim:.4f})")
        print(f"     Кадры очень похожи - стабильная съемка")
    elif avg_ssim > 0.75:
        print(f"\n  ⚠️  Умеренное структурное сходство (avg SSIM={avg_ssim:.4f})")
        print(f"     Есть различия между кадрами")
    else:
        print(f"\n  ❌ Низкое структурное сходство (avg SSIM={avg_ssim:.4f})")
        print(f"     Кадры сильно отличаются друг от друга")
    
    # MAE
    if avg_mae < 10:
        print(f"\n  ✅ Низкая средняя ошибка (avg MAE={avg_mae:.2f})")
        print(f"     Минимальные попикельные различия")
    elif avg_mae < 20:
        print(f"\n  ⚠️  Умеренная средняя ошибка (avg MAE={avg_mae:.2f})")
        print(f"     Заметные различия, но приемлемые")
    else:
        print(f"\n  ❌ Высокая средняя ошибка (avg MAE={avg_mae:.2f})")
        print(f"     Значительные различия между кадрами")
    
    print(f"\n🎓 Рекомендации:\n")
    
    if avg_ssim > 0.85 and brightness_variation < 8:
        print(f"  ✅ Отличное качество съемки - камера стабильна")
        print(f"  ✅ Условия освещения однородные")
        print(f"  💡 Можно использовать эти кадры для калибровки")
    elif avg_ssim > 0.70:
        print(f"  ⚠️  Приемлемое качество, но есть вариации")
        print(f"  💡 Проверьте стабильность крепления камеры")
        print(f"  💡 Убедитесь в однородности освещения")
    else:
        print(f"  ❌ Качество нестабильное")
        print(f"  💡 Рекомендуется переснять кадры")
        print(f"  💡 Проверьте настройки камеры (экспозиция, баланс белого)")
        print(f"  💡 Убедитесь, что поверхность действительно однородная")
    
    print(f"\n{'='*80}\n")
    
    # Создаем визуализацию
    create_multiple_frames_visualization(images, labels, stats_list, comparisons)


def create_multiple_frames_visualization(images: List[np.ndarray], 
                                         labels: List[str],
                                         stats_list: List[Dict],
                                         comparisons: List[Dict]) -> None:
    """
    Создает визуализацию для анализа нескольких кадров.
    
    Args:
        images: Список изображений
        labels: Список меток
        stats_list: Список статистик
        comparisons: Список результатов сравнений
    """
    try:
        n = len(images)
        
        # Создаем большую фигуру
        fig = plt.figure(figsize=(18, 12))
        gs = gridspec.GridSpec(4, n, height_ratios=[2, 1, 1, 1])
        
        # 1. Показываем все изображения
        for i, (img, label, stats) in enumerate(zip(images, labels, stats_list)):
            ax = fig.add_subplot(gs[0, i])
            ax.imshow(img, cmap='gray', vmin=0, vmax=255)
            ax.set_title(f"{label}\nμ={stats['mean']:.1f}, σ={stats['std']:.1f}", 
                        fontsize=10, fontweight='bold')
            ax.axis('off')
        
        # 2. График средней яркости
        ax_brightness = fig.add_subplot(gs[1, :])
        means = [s['mean'] for s in stats_list]
        stds = [s['std'] for s in stats_list]
        x = range(len(means))
        
        ax_brightness.errorbar(x, means, yerr=stds, marker='o', capsize=5, 
                              linewidth=2, markersize=8, label='Средняя яркость ± σ')
        ax_brightness.axhline(y=np.mean(means), color='r', linestyle='--', 
                             label=f'Общее среднее: {np.mean(means):.2f}', alpha=0.7)
        ax_brightness.set_xlabel('Номер кадра', fontsize=11)
        ax_brightness.set_ylabel('Яркость', fontsize=11)
        ax_brightness.set_title('Распределение яркости по кадрам', fontsize=12, fontweight='bold')
        ax_brightness.set_xticks(x)
        ax_brightness.set_xticklabels([f"{i+1}" for i in x])
        ax_brightness.legend(fontsize=9)
        ax_brightness.grid(True, alpha=0.3)
        
        # 3. Тепловая карта SSIM
        ax_ssim = fig.add_subplot(gs[2, :])
        
        # Создаем матрицу SSIM
        ssim_matrix = np.ones((n, n))
        comp_idx = 0
        for i in range(n):
            for j in range(i + 1, n):
                ssim_val = comparisons[comp_idx]['ssim']
                ssim_matrix[i, j] = ssim_val
                ssim_matrix[j, i] = ssim_val
                comp_idx += 1
        
        im = ax_ssim.imshow(ssim_matrix, cmap='RdYlGn', vmin=0.5, vmax=1.0, aspect='auto')
        ax_ssim.set_xlabel('Номер кадра', fontsize=11)
        ax_ssim.set_ylabel('Номер кадра', fontsize=11)
        ax_ssim.set_title('Матрица структурного сходства (SSIM)', fontsize=12, fontweight='bold')
        ax_ssim.set_xticks(range(n))
        ax_ssim.set_yticks(range(n))
        ax_ssim.set_xticklabels([f"{i+1}" for i in range(n)])
        ax_ssim.set_yticklabels([f"{i+1}" for i in range(n)])
        
        # Добавляем значения в ячейки
        for i in range(n):
            for j in range(n):
                text = ax_ssim.text(j, i, f'{ssim_matrix[i, j]:.3f}',
                                   ha="center", va="center", color="black", fontsize=8)
        
        plt.colorbar(im, ax=ax_ssim, label='SSIM')
        
        # 4. Гистограммы яркости
        ax_hist = fig.add_subplot(gs[3, :])
        
        for i, (img, label) in enumerate(zip(images, labels)):
            hist, bins = np.histogram(img, bins=64, range=(0, 255))
            hist = hist.astype(float) / hist.sum()  # Нормализуем
            ax_hist.plot(bins[:-1], hist, label=label, alpha=0.7, linewidth=2)
        
        ax_hist.set_xlabel('Яркость', fontsize=11)
        ax_hist.set_ylabel('Нормализованная частота', fontsize=11)
        ax_hist.set_title('Распределение яркости (гистограммы)', fontsize=12, fontweight='bold')
        ax_hist.legend(fontsize=9, ncol=min(4, n))
        ax_hist.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        output_path = OUTPUT_DIR / 'frame_differences_analysis.png'
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
        description='Анализ различий между кадрами пустого поля',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:

  1. Сравнить два кадра:
     python3 evaluate_frame_differences.py image1.jpg image2.jpg

  2. Проанализировать всю директорию:
     python3 evaluate_frame_differences.py data/img_calibration/*.jpg

  3. Сравнить несколько конкретных кадров:
     python3 evaluate_frame_differences.py frame1.jpg frame2.jpg frame3.jpg frame4.jpg
        """
    )
    
    parser.add_argument('images', nargs='+', help='Пути к изображениям для анализа')
    
    args = parser.parse_args()
    
    # Проверяем существование файлов
    image_paths = []
    for path in args.images:
        p = Path(path)
        if not p.exists():
            print(f"❌ Файл не найден: {path}")
            continue
        if not p.is_file():
            print(f"❌ Не файл: {path}")
            continue
        image_paths.append(str(p))
    
    if len(image_paths) == 0:
        print("❌ Не найдено ни одного изображения для анализа")
        return 1
    
    # Создаем метки
    labels = [f"Кадр {i+1}: {Path(p).name}" for i, p in enumerate(image_paths)]
    
    print(f"\n{'='*80}")
    print(f"🎯 ОЦЕНКА РАЗЛИЧИЙ МЕЖДУ КАДРАМИ ПУСТОГО ПОЛЯ")
    print(f"{'='*80}")
    print(f"\nВсего кадров для анализа: {len(image_paths)}\n")
    
    for i, (path, label) in enumerate(zip(image_paths, labels), 1):
        print(f"  {i}. {label}")
    
    if len(image_paths) == 1:
        print("\n❌ Нужно минимум 2 изображения для сравнения")
        return 1
    elif len(image_paths) == 2:
        # Простое сравнение двух кадров
        compare_two_frames(image_paths[0], image_paths[1], labels[0], labels[1])
    else:
        # Полный анализ нескольких кадров
        analyze_multiple_frames(image_paths, labels)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
