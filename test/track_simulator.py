#!/usr/bin/env python3
"""
Симулятор прохождения трассы с использованием алгоритма распознавания линии.

Функции:
- Загружает карту трассы (track_map.json)
- Проходит по кадрам, передавая изображения алгоритму
- Применяет статистическую фильтрацию (сглаживание на основе истории)
- Сравнивает результат алгоритма с ожидаемым действием
- Генерирует отчет о точности
"""

import os
import json
import sys
from pathlib import Path
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from PIL import Image

# Добавляем путь к модулю с алгоритмом
sys.path.insert(0, str(Path(__file__).parent))
from test_line_detection import detect_line_position

# Константы
OUTPUT_DIR = Path(__file__).parent / 'output'
TRACK_MAP_FILE = OUTPUT_DIR / 'track_map.json'
SIMULATION_RESULTS_FILE = OUTPUT_DIR / 'simulation_results.json'

# Параметры статистической фильтрации
HISTORY_SIZE = 3  # Учитываем последние N кадров
CONFIDENCE_THRESHOLD = 0.6  # Порог уверенности для изменения направления


class DirectionFilter:
    """
    Статистический фильтр для сглаживания решений на основе истории.
    
    Логика:
    - Если едем прямо, нужно N подтверждений поворота, чтобы начать поворачивать
    - Если уже поворачиваем, продолжаем пока не увидим N кадров прямой линии
    """
    
    def __init__(self, history_size=HISTORY_SIZE):
        self.history = deque(maxlen=history_size)
        self.current_action = 'straight'
    
    def add_detection(self, position, trend, detected, is_terminate):
        """
        Добавляет результат детекции в историю.
        
        Args:
            position: позиция линии (-1.0 ... +1.0)
            trend: тренд направления
            detected: флаг обнаружения линии
            is_terminate: флаг T-пересечения
        """
        if is_terminate:
            action = 'terminate'
        elif not detected:
            action = 'unknown'
        else:
            # Определяем направление на основе позиции и тренда
            combined = position * 0.6 + trend * 0.4
            
            if abs(combined) < 0.15:
                action = 'straight'
            elif combined < 0:
                action = 'left'
            else:
                action = 'right'
        
        self.history.append({
            'position': position,
            'trend': trend,
            'detected': detected,
            'is_terminate': is_terminate,
            'raw_action': action
        })
    
    def get_filtered_action(self):
        """
        Возвращает отфильтрованное действие на основе истории.
        
        Returns:
            str: 'straight', 'left', 'right', 'terminate', или 'unknown'
        """
        if not self.history:
            return 'unknown'
        
        # Считаем частоту каждого действия в истории
        action_counts = {}
        for frame in self.history:
            action = frame['raw_action']
            action_counts[action] = action_counts.get(action, 0) + 1
        
        # Терминирование имеет высокий приоритет
        if action_counts.get('terminate', 0) >= 2:
            self.current_action = 'terminate'
            return 'terminate'
        
        # Неизвестное состояние
        if action_counts.get('unknown', 0) >= len(self.history) - 1:
            return 'unknown'
        
        # Определяем доминирующее действие
        max_count = 0
        dominant_action = 'straight'
        for action, count in action_counts.items():
            if action not in ['unknown', 'terminate'] and count > max_count:
                max_count = count
                dominant_action = action
        
        # Фильтрация: меняем действие только если есть уверенность
        confidence = max_count / len(self.history)
        
        if confidence >= CONFIDENCE_THRESHOLD:
            # Достаточно уверены в новом направлении
            if dominant_action != self.current_action:
                # Смена направления - требуем больше подтверждений
                if max_count >= 2:  # Минимум 2 кадра подряд
                    self.current_action = dominant_action
        
        return self.current_action
    
    def get_statistics(self):
        """Возвращает статистику по истории."""
        if not self.history:
            return {}
        
        return {
            'history_size': len(self.history),
            'current_action': self.current_action,
            'recent_positions': [f['position'] for f in self.history],
            'recent_trends': [f['trend'] for f in self.history]
        }


def load_track_map():
    """Загружает карту трассы из JSON."""
    if not TRACK_MAP_FILE.exists():
        raise FileNotFoundError(
            f"Карта трассы не найдена: {TRACK_MAP_FILE}\n"
            f"Сначала запустите: python3 generate_track_map.py"
        )
    
    with open(TRACK_MAP_FILE, 'r', encoding='utf-8') as f:
        return json.load(f)


def run_simulation(track_map, use_filter=True):
    """
    Запускает симуляцию прохождения трассы.
    
    Args:
        track_map: карта трассы
        use_filter: использовать ли статистический фильтр
    
    Returns:
        list: результаты симуляции для каждого кадра
    """
    frames = track_map['frames']
    results = []
    
    # Инициализируем фильтр
    direction_filter = DirectionFilter(history_size=HISTORY_SIZE) if use_filter else None
    
    print(f"\n🚗 Начинаем симуляцию прохождения трассы...")
    print(f"📊 Всего кадров: {len(frames)}")
    print(f"🔧 Статистический фильтр: {'включен' if use_filter else 'выключен'}\n")
    
    for i, frame in enumerate(frames):
        frame_id = frame['frame_id']
        image_path = OUTPUT_DIR / 'frames' / Path(frame['image']).name
        expected_action = frame['expected_action']
        
        # Запускаем алгоритм распознавания
        try:
            detection = detect_line_position(str(image_path))
            
            # Определяем сырое действие (без фильтра)
            if detection['is_terminate']:
                raw_action = 'terminate'
            elif not detection['detected']:
                raw_action = 'unknown'
            else:
                combined = detection['position'] * 0.6 + detection['direction_trend'] * 0.4
                if abs(combined) < 0.15:
                    raw_action = 'straight'
                elif combined < 0:
                    raw_action = 'left'
                else:
                    raw_action = 'right'
            
            # Применяем фильтр
            if use_filter and direction_filter:
                direction_filter.add_detection(
                    detection['position'],
                    detection['direction_trend'],
                    detection['detected'],
                    detection['is_terminate']
                )
                filtered_action = direction_filter.get_filtered_action()
            else:
                filtered_action = raw_action
            
            # Сравниваем с ожидаемым
            is_correct_raw = (raw_action == expected_action)
            is_correct_filtered = (filtered_action == expected_action)
            
            result = {
                'frame_id': frame_id,
                'image': frame['image'],
                'expected_action': expected_action,
                'raw_action': raw_action,
                'filtered_action': filtered_action,
                'is_correct_raw': is_correct_raw,
                'is_correct_filtered': is_correct_filtered,
                'detection': {
                    'position': detection['position'],
                    'trend': detection['direction_trend'],
                    'detected': detection['detected'],
                    'is_terminate': detection['is_terminate']
                }
            }
            
            results.append(result)
            
            # Прогресс
            if (i + 1) % 10 == 0:
                correct_raw = sum(1 for r in results if r['is_correct_raw'])
                correct_filtered = sum(1 for r in results if r['is_correct_filtered'])
                print(f"  Кадр {i+1}/{len(frames)}: "
                      f"Точность без фильтра={correct_raw/(i+1)*100:.1f}%, "
                      f"с фильтром={correct_filtered/(i+1)*100:.1f}%")
        
        except Exception as e:
            print(f"⚠️ Ошибка на кадре {frame_id}: {e}")
            results.append({
                'frame_id': frame_id,
                'image': frame['image'],
                'expected_action': expected_action,
                'raw_action': 'error',
                'filtered_action': 'error',
                'is_correct_raw': False,
                'is_correct_filtered': False,
                'error': str(e)
            })
    
    return results


def analyze_results(results):
    """Анализирует результаты симуляции."""
    total = len(results)
    
    # Общая точность
    correct_raw = sum(1 for r in results if r['is_correct_raw'])
    correct_filtered = sum(1 for r in results if r['is_correct_filtered'])
    
    accuracy_raw = correct_raw / total * 100 if total > 0 else 0
    accuracy_filtered = correct_filtered / total * 100 if total > 0 else 0
    
    # Точность по категориям
    categories = ['straight', 'left', 'right', 'terminate']
    category_stats = {}
    
    for category in categories:
        category_frames = [r for r in results if r['expected_action'] == category]
        if category_frames:
            correct_raw_cat = sum(1 for r in category_frames if r['is_correct_raw'])
            correct_filt_cat = sum(1 for r in category_frames if r['is_correct_filtered'])
            
            category_stats[category] = {
                'total': len(category_frames),
                'correct_raw': correct_raw_cat,
                'correct_filtered': correct_filt_cat,
                'accuracy_raw': correct_raw_cat / len(category_frames) * 100,
                'accuracy_filtered': correct_filt_cat / len(category_frames) * 100
            }
    
    # Улучшение от фильтра
    improvement = accuracy_filtered - accuracy_raw
    
    return {
        'total_frames': total,
        'correct_raw': correct_raw,
        'correct_filtered': correct_filtered,
        'accuracy_raw': accuracy_raw,
        'accuracy_filtered': accuracy_filtered,
        'improvement': improvement,
        'category_stats': category_stats
    }


def print_report(analysis):
    """Выводит отчет о результатах симуляции."""
    print("\n" + "="*80)
    print("📊 ОТЧЕТ О СИМУЛЯЦИИ")
    print("="*80)
    
    print(f"\n🎯 Общая точность:")
    print(f"  Без фильтра:  {analysis['correct_raw']}/{analysis['total_frames']} "
          f"({analysis['accuracy_raw']:.1f}%)")
    print(f"  С фильтром:   {analysis['correct_filtered']}/{analysis['total_frames']} "
          f"({analysis['accuracy_filtered']:.1f}%)")
    print(f"  Улучшение:    {analysis['improvement']:+.1f}%")
    
    print(f"\n📈 Точность по категориям:")
    print(f"{'Категория':<15} {'Кадров':<8} {'Без фильтра':<15} {'С фильтром':<15} {'Улучшение':<10}")
    print("-" * 80)
    
    for category, stats in analysis['category_stats'].items():
        improvement_cat = stats['accuracy_filtered'] - stats['accuracy_raw']
        print(f"{category.upper():<15} {stats['total']:<8} "
              f"{stats['correct_raw']}/{stats['total']} ({stats['accuracy_raw']:.1f}%)"
              f"{'':>4}"
              f"{stats['correct_filtered']}/{stats['total']} ({stats['accuracy_filtered']:.1f}%)"
              f"{'':>4}"
              f"{improvement_cat:+.1f}%")
    
    print("="*80)


def visualize_simulation(results, analysis):
    """Создает визуализацию результатов симуляции."""
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(20, 8))
    
    # График 1: Сравнение ожидаемых и фактических действий
    colors = {
        'straight': 'green',
        'left': 'blue',
        'right': 'orange',
        'terminate': 'red',
        'unknown': 'gray',
        'error': 'black'
    }
    
    for i, result in enumerate(results):
        expected = result['expected_action']
        filtered = result['filtered_action']
        
        # Ожидаемое действие (верхняя полоса)
        ax1.add_patch(mpatches.Rectangle((i, 0.5), 1, 0.5, 
                                          facecolor=colors.get(expected, 'gray'),
                                          edgecolor='black', linewidth=0.5))
        
        # Фактическое действие (нижняя полоса)
        edge_color = 'green' if result['is_correct_filtered'] else 'red'
        ax1.add_patch(mpatches.Rectangle((i, 0), 1, 0.5,
                                          facecolor=colors.get(filtered, 'gray'),
                                          edgecolor=edge_color, linewidth=2))
    
    ax1.set_xlim(0, len(results))
    ax1.set_ylim(0, 1)
    ax1.set_xlabel('Номер кадра')
    ax1.set_ylabel('Действие')
    ax1.set_title(f'Сравнение: Ожидаемое (верх) vs Фактическое (низ) | '
                  f'Точность: {analysis["accuracy_filtered"]:.1f}%')
    ax1.set_yticks([0.25, 0.75])
    ax1.set_yticklabels(['Фактическое', 'Ожидаемое'])
    
    # Легенда
    legend_elements = [
        mpatches.Patch(facecolor='green', label='STRAIGHT'),
        mpatches.Patch(facecolor='blue', label='LEFT'),
        mpatches.Patch(facecolor='orange', label='RIGHT'),
        mpatches.Patch(facecolor='red', label='TERMINATE'),
        mpatches.Patch(facecolor='white', edgecolor='green', linewidth=2, label='Правильно'),
        mpatches.Patch(facecolor='white', edgecolor='red', linewidth=2, label='Ошибка')
    ]
    ax1.legend(handles=legend_elements, loc='upper right', ncol=6)
    
    # График 2: Точность по категориям
    categories = ['STRAIGHT', 'LEFT', 'RIGHT', 'TERMINATE']
    raw_acc = [analysis['category_stats'][cat.lower()]['accuracy_raw'] for cat in categories]
    filt_acc = [analysis['category_stats'][cat.lower()]['accuracy_filtered'] for cat in categories]
    
    x = range(len(categories))
    width = 0.35
    
    ax2.bar([i - width/2 for i in x], raw_acc, width, label='Без фильтра', color='lightblue')
    ax2.bar([i + width/2 for i in x], filt_acc, width, label='С фильтром', color='darkblue')
    
    ax2.set_xlabel('Категория')
    ax2.set_ylabel('Точность (%)')
    ax2.set_title('Точность распознавания по категориям')
    ax2.set_xticks(x)
    ax2.set_xticklabels(categories)
    ax2.legend()
    ax2.set_ylim(0, 100)
    ax2.grid(axis='y', alpha=0.3)
    
    # Добавляем значения на столбцах
    for i, (raw, filt) in enumerate(zip(raw_acc, filt_acc)):
        ax2.text(i - width/2, raw + 2, f'{raw:.1f}%', ha='center', fontsize=9)
        ax2.text(i + width/2, filt + 2, f'{filt:.1f}%', ha='center', fontsize=9)
    
    plt.tight_layout()
    output_file = OUTPUT_DIR / 'simulation_visualization.png'
    plt.savefig(output_file, dpi=150)
    print(f"\n✅ Визуализация сохранена: {output_file}")
    plt.close()


def save_results(results, analysis):
    """Сохраняет результаты симуляции в JSON."""
    output = {
        'version': '1.0',
        'simulation_results': results,
        'analysis': analysis
    }
    
    with open(SIMULATION_RESULTS_FILE, 'w', encoding='utf-8') as f:
        json.dump(output, f, indent=2, ensure_ascii=False)
    
    print(f"✅ Результаты сохранены: {SIMULATION_RESULTS_FILE}")


if __name__ == '__main__':
    print("🏁 Симулятор прохождения трассы\n")
    
    # Загружаем карту
    try:
        track_map = load_track_map()
        print(f"✅ Карта трассы загружена: {len(track_map['frames'])} кадров")
    except FileNotFoundError as e:
        print(f"❌ {e}")
        sys.exit(1)
    
    # Запускаем симуляцию
    results = run_simulation(track_map, use_filter=True)
    
    # Анализируем результаты
    analysis = analyze_results(results)
    
    # Выводим отчет
    print_report(analysis)
    
    # Сохраняем результаты
    save_results(results, analysis)
    
    # Визуализация
    try:
        visualize_simulation(results, analysis)
    except Exception as e:
        print(f"⚠️ Не удалось создать визуализацию: {e}")
    
    print("\n✅ Симуляция завершена!")
