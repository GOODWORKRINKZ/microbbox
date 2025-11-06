#!/usr/bin/env python3
"""
Скрипт для захвата кадров со стрима камеры робота
Сохраняет кадры в указанную категорию для тестирования алгоритма детекции линии
"""

import time
import argparse
from pathlib import Path
from datetime import datetime
import requests
import numpy as np
from PIL import Image
import io

def capture_frames(stream_url, output_dir, num_frames=50, interval=1.0, category='straight'):
    """
    Захват кадров со стрима камеры
    
    Args:
        stream_url: URL стрима (например, http://192.168.1.125:81/stream)
        output_dir: базовая директория для сохранения (например, data/)
        num_frames: количество кадров для захвата
        interval: интервал между кадрами в секундах
        category: категория изображений (straight, left, right, terminate)
    """
    # Создаем директорию для категории
    category_map = {
        'straight': 'img_straight',
        'left': 'img_left',
        'right': 'img_right',
        'terminate': 'img_terminate'
    }
    
    folder_name = category_map.get(category, f'img_{category}')
    save_dir = Path(output_dir) / folder_name
    save_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Захват {num_frames} кадров со стрима: {stream_url}")
    print(f"Интервал между кадрами: {interval} сек")
    print(f"Сохранение в: {save_dir}")
    print(f"Категория: {category}")
    print("\nПодключение к стриму...")
    
    try:
        # Открываем стрим с timeout
        response = requests.get(stream_url, stream=True, timeout=5)
        response.raise_for_status()
    except requests.exceptions.RequestException as e:
        print(f"ОШИБКА: Не удалось подключиться к стриму {stream_url}")
        print(f"Причина: {e}")
        print("Проверьте:")
        print("  1. Робот включен и доступен")
        print("  2. URL стрима правильный")
        print("  3. Стрим работает (откройте в браузере)")
        return False
    
    print("✓ Тест подключения успешен")
    print(f"\nНачинаю захват кадров. Нажмите Ctrl+C для прерывания...")
    print(f"Перемещайте робота во время захвата!\n")
    
    captured_count = 0
    start_time = time.time()
    
    try:
        for i in range(num_frames):
            try:
                # Для каждого кадра открываем новое соединение
                response = requests.get(stream_url, stream=True, timeout=5)
                response.raise_for_status()
                
                # Буфер для парсинга MJPEG потока
                bytes_buffer = b''
                frame_captured = False
                
                # Читаем данные из потока пока не найдем полный JPEG кадр
                for chunk in response.iter_content(chunk_size=4096):
                    bytes_buffer += chunk
                    
                    # Ищем границы JPEG (SOI и EOI маркеры)
                    a = bytes_buffer.find(b'\xff\xd8')  # JPEG start
                    b = bytes_buffer.find(b'\xff\xd9')  # JPEG end
                    
                    if a != -1 and b != -1:
                        # Извлекаем JPEG изображение
                        jpg = bytes_buffer[a:b+2]
                        
                        try:
                            # Декодируем JPEG
                            img = Image.open(io.BytesIO(jpg))
                            
                            # Генерируем имя файла с timestamp
                            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]  # миллисекунды
                            filename = f"{category}_{timestamp}_{i+1:03d}.jpg"
                            filepath = save_dir / filename
                            
                            # Сохраняем кадр
                            img.save(filepath, 'JPEG')
                            
                            captured_count += 1
                            elapsed = time.time() - start_time
                            remaining = num_frames - captured_count
                            eta = remaining * interval if captured_count > 0 else 0
                            
                            print(f"[{captured_count}/{num_frames}] Сохранен: {filename} | "
                                  f"Прошло: {elapsed:.1f}с | Осталось: ~{eta:.0f}с")
                            
                            frame_captured = True
                            break
                            
                        except Exception as e:
                            print(f"Ошибка декодирования кадра: {e}")
                            continue
                
                response.close()
                
                if not frame_captured:
                    print(f"\nПредупреждение: Не удалось прочитать кадр {i+1}")
                
                # Ждем перед следующим кадром
                if i < num_frames - 1:  # не ждем после последнего кадра
                    time.sleep(interval)
                    
            except requests.exceptions.RequestException as e:
                print(f"\nОшибка подключения для кадра {i+1}: {e}")
                time.sleep(0.5)  # Небольшая пауза перед следующей попыткой
                continue
    
    except KeyboardInterrupt:
        print(f"\n\n⚠ Прервано пользователем")
    
    except Exception as e:
        print(f"\n\nОШИБКА: {e}")
    
    total_time = time.time() - start_time
    print(f"\n{'='*60}")
    print(f"Захвачено кадров: {captured_count} из {num_frames}")
    print(f"Общее время: {total_time:.1f} секунд")
    print(f"Сохранено в: {save_dir}")
    print(f"{'='*60}\n")
    
    return captured_count > 0


def main():
    parser = argparse.ArgumentParser(
        description='Захват кадров со стрима камеры робота для тестирования',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:

  # Захватить 50 кадров для прямой линии (по умолчанию)
  python capture_frames.py

  # Захватить 30 кадров для поворота налево
  python capture_frames.py --category left --num 30

  # Захватить 20 кадров с интервалом 2 секунды
  python capture_frames.py --interval 2.0 --num 20

  # Использовать другой URL стрима
  python capture_frames.py --url http://192.168.1.100:81/stream

Категории:
  straight   - линия прямо (робот едет по центру)
  left       - линия слева (робот должен повернуть налево)
  right      - линия справа (робот должен повернуть направо)
  terminate  - T-пересечение или конец линии
        """
    )
    
    parser.add_argument(
        '--url',
        type=str,
        default='http://192.168.1.125:81/stream',
        help='URL стрима камеры (по умолчанию: http://192.168.1.125:81/stream)'
    )
    
    parser.add_argument(
        '--output',
        type=str,
        default='data',
        help='Директория для сохранения кадров (по умолчанию: data/)'
    )
    
    parser.add_argument(
        '--num',
        type=int,
        default=50,
        help='Количество кадров для захвата (по умолчанию: 50)'
    )
    
    parser.add_argument(
        '--interval',
        type=float,
        default=1.0,
        help='Интервал между кадрами в секундах (по умолчанию: 1.0)'
    )
    
    parser.add_argument(
        '--category',
        type=str,
        choices=['straight', 'left', 'right', 'terminate'],
        default='straight',
        help='Категория изображений (по умолчанию: straight)'
    )
    
    args = parser.parse_args()
    
    # Проверяем наличие необходимых библиотек
    try:
        import requests
        from PIL import Image
    except ImportError as e:
        print(f"ОШИБКА: Требуются библиотеки requests и pillow")
        print("Установите их командой:")
        print("  pip install requests pillow")
        return 1
    
    # Запускаем захват
    success = capture_frames(
        stream_url=args.url,
        output_dir=args.output,
        num_frames=args.num,
        interval=args.interval,
        category=args.category
    )
    
    if success:
        print("✓ Захват кадров завершен успешно!")
        print(f"\nТеперь можно запустить тесты:")
        print(f"  python test\\test_line_detection.py")
        return 0
    else:
        print("✗ Захват кадров завершен с ошибками")
        return 1


if __name__ == '__main__':
    exit(main())
