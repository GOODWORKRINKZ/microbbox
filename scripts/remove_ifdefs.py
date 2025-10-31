#!/usr/bin/env python3
"""
Скрипт для удаления директив условной компиляции для обязательных фич
Удаляет ПАРНЫЕ #ifdef/#endif для FEATURE_MOTORS, FEATURE_CAMERA, FEATURE_WIFI, FEATURE_OTA_UPDATE
"""
import re

def remove_ifdef_blocks(content, features_to_remove):
    """Удаляет блоки #ifdef для указанных фич, сохраняя содержимое"""
    lines = content.split('\n')
    result = []
    ifdef_stack = []  # Стек открытых директив
    skip_lines = set()  # Номера строк для пропуска
    
    # Первый проход: находим парные #ifdef/#endif
    for i, line in enumerate(lines):
        stripped = line.strip()
        
        # Проверяем #ifdef для удаляемых фич
        for feature in features_to_remove:
            if f'#ifdef {feature}' in stripped or f'#if defined({feature})' in stripped:
                ifdef_stack.append((i, feature))
                skip_lines.add(i)
                break
            # Также проверяем комбинированные условия
            if feature in stripped and ('#ifdef' in stripped or '#if defined' in stripped):
                # Проверяем, что это только наши фичи (нет NEOPIXEL/BUZZER)
                if 'FEATURE_NEOPIXEL' not in stripped and 'FEATURE_BUZZER' not in stripped:
                    ifdef_stack.append((i, feature))
                    skip_lines.add(i)
                    break
        
        # Проверяем #endif
        if '#endif' in stripped and ifdef_stack:
            # Проверяем комментарий в #endif
            comment = stripped.replace('#endif', '').strip()
            if ifdef_stack:
                start_line, feature = ifdef_stack[-1]
                # Проверяем, относится ли этот #endif к нашей директиве
                if feature in comment or not comment or comment.startswith('//'):
                    ifdef_stack.pop()
                    skip_lines.add(i)
    
    # Второй проход: формируем результат без пропускаемых строк
    for i, line in enumerate(lines):
        if i not in skip_lines:
            result.append(line)
    
    return '\n'.join(result)

def process_file(filepath):
    print(f"Обработка {filepath}...")
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Фичи для удаления директив
    features_to_remove = [
        'FEATURE_MOTORS',
        'FEATURE_CAMERA', 
        'FEATURE_WIFI',
        'FEATURE_OTA_UPDATE'
    ]
    
    # Удаляем блоки
    new_content = remove_ifdef_blocks(content, features_to_remove)
    
    # Записываем результат
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(new_content)
    
    print(f"✓ Обработан: {filepath}")

if __name__ == '__main__':
    import sys
    import os
    
    files = [
        'src/MicroBoxRobot.cpp',
        'include/MicroBoxRobot.h'
    ]
    
    for filepath in files:
        if not os.path.exists(filepath):
            print(f"✗ Файл не найден: {filepath}")
            sys.exit(1)
        
        try:
            process_file(filepath)
        except Exception as e:
            print(f"✗ Ошибка при обработке {filepath}: {e}")
            import traceback
            traceback.print_exc()
            sys.exit(1)
    
    print("\n✓ Все директивы условной компиляции для обязательных фич удалены!")
