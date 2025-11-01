#!/usr/bin/env python3
"""
Генератор схемы подключения МикРоББокс
Использует библиотеку schemdraw для создания электрических схем
"""

import schemdraw
from schemdraw import elements as elm
from schemdraw import logic

def create_wiring_diagram():
    """Создает основную схему подключения"""
    
    with schemdraw.Drawing(show=False) as d:
        d.config(fontsize=10, font='sans-serif')
        
        # Заголовок
        d += elm.Label().at((5, 12)).label('МикРоББокс - Схема подключения', fontsize=16, fontweight='bold')
        d += elm.Label().at((5, 11.5)).label('ESP32-CAM с MX1508, NeoPixel и Бузером', fontsize=12)
        
        # ESP32-CAM (центральный элемент)
        esp32 = d += elm.Ic(pins=[
            elm.IcPin(name='5V', side='left', pin='1'),
            elm.IcPin(name='GND', side='left', pin='2'),
            elm.IcPin(name='GPIO2', side='right', pin='3'),
            elm.IcPin(name='GPIO12', side='right', pin='4'),
            elm.IcPin(name='GPIO13', side='right', pin='5'),
            elm.IcPin(name='GPIO14', side='right', pin='6'),
            elm.IcPin(name='GPIO15', side='right', pin='7'),
            elm.IcPin(name='GPIO16', side='bottom', pin='8'),
        ], leadlen=0.5, size=(2, 3)).at((4, 7)).label('ESP32-CAM\nAI Thinker', loc='center')
        
        # Питание от батареи
        d += elm.Label().at((0, 10)).label('LiPo 3.7V', fontsize=10)
        d += elm.Battery().at((0.5, 9.5)).down().label('+', loc='top').label('-', loc='bottom')
        d += elm.Line().right(0.5)
        d += elm.Switch().right(0.5).label('ON/OFF', loc='bottom')
        d += elm.Diode().right(0.5).label('1N5819', loc='bottom')
        
        # Buck converter для 5V
        d += elm.Line().right(0.5)
        buck_node = d.here
        d += elm.Ic(pins=[
            elm.IcPin(name='IN', side='left'),
            elm.IcPin(name='GND', side='bottom'),
            elm.IcPin(name='OUT', side='right')
        ], size=(1, 0.8)).label('Buck\n5V', loc='center')
        d += elm.Capacitor().down(0.8).at(buck_node).label('220µF', loc='right')
        d += elm.Ground()
        
        # Подключение 5V к ESP32
        d += elm.Line().right(1.5).at((buck_node[0] + 1, buck_node[1]))
        d += elm.Capacitor().down(0.8).label('100µF', loc='right')
        gnd_rail = d.here
        d += elm.Ground()
        
        # Линия 5V к ESP32
        d += elm.Line().to(esp32.pins['5V'].start)
        
        # GND от ESP32
        d += elm.Line().at(esp32.pins['GND'].end).down(0.5)
        d += elm.Line().to((gnd_rail[0], esp32.pins['GND'].end[1] - 0.5))
        d += elm.Ground()
        
        # NeoPixel LED цепочка
        led_start = esp32.pins['GPIO2'].end
        d += elm.Line().at(led_start).right(0.5)
        d += elm.Label().label('NeoPixel\nLED #1', fontsize=8)
        d += elm.Line().right(0.5)
        d += elm.Label().label('LED #2', fontsize=8)
        d += elm.Line().right(0.5)
        d += elm.Label().label('LED #3', fontsize=8)
        
        # MX1508 Motor Driver
        mx1508_x = esp32.pins['GPIO12'].end[0] + 2
        mx1508_y = esp32.pins['GPIO14'].end[1]
        
        d += elm.Ic(pins=[
            elm.IcPin(name='IN1', side='left', pin='1'),
            elm.IcPin(name='IN2', side='left', pin='2'),
            elm.IcPin(name='IN3', side='left', pin='3'),
            elm.IcPin(name='IN4', side='left', pin='4'),
            elm.IcPin(name='VM', side='top', pin='5'),
            elm.IcPin(name='GND', side='bottom', pin='6'),
            elm.IcPin(name='OUT1', side='right', pin='7'),
            elm.IcPin(name='OUT2', side='right', pin='8'),
            elm.IcPin(name='OUT3', side='right', pin='9'),
            elm.IcPin(name='OUT4', side='right', pin='10'),
        ], leadlen=0.5, size=(2, 2.5)).at((mx1508_x, mx1508_y)).label('MX1508\nMotor Driver', loc='center')
        
        # Pull-down резисторы для GPIO12-15
        for pin_name in ['GPIO12', 'GPIO13', 'GPIO14', 'GPIO15']:
            pin_start = esp32.pins[pin_name].end
            d += elm.Line().at(pin_start).right(0.3)
            d += elm.Dot()
            node = d.here
            d += elm.Line().right(0.5)
            # Резистор вниз к GND
            d += elm.Resistor().at(node).down(0.6).label('10kΩ', loc='right', fontsize=8)
            d += elm.Ground()
        
        # Бузер
        buzzer_start = esp32.pins['GPIO16'].end
        d += elm.Line().at(buzzer_start).down(0.5)
        d += elm.Ic(pins=[
            elm.IcPin(name='+', side='top'),
            elm.IcPin(name='-', side='bottom')
        ], size=(0.8, 0.6)).label('Buzzer', loc='center')
        d += elm.Line().down(0.3)
        d += elm.Ground()
        
        # Легенда
        d += elm.Label().at((1, 1)).label('Важно:', fontsize=10, fontweight='bold')
        d += elm.Label().at((1, 0.7)).label('• Pull-down резисторы 10kΩ ОБЯЗАТЕЛЬНЫ для GPIO12-15', fontsize=8)
        d += elm.Label().at((1, 0.4)).label('• Конденсаторы развязки: 100µF для ESP32, 220µF для моторов', fontsize=8)
        d += elm.Label().at((1, 0.1)).label('• Диод 1N5819 защищает от обратной полярности', fontsize=8)
        
        d.save('/home/runner/work/microbbox/microbbox/hardware/diagrams/wiring_schematic.svg')
        d.save('/home/runner/work/microbbox/microbbox/hardware/diagrams/wiring_schematic.png', dpi=300)
        
    print("✓ Основная схема подключения создана: wiring_schematic.svg/png")


def create_power_distribution():
    """Создает схему распределения питания"""
    
    with schemdraw.Drawing(show=False) as d:
        d.config(fontsize=10, font='sans-serif')
        
        # Заголовок
        d += elm.Label().at((4, 10)).label('Схема питания МикРоББокс', fontsize=14, fontweight='bold')
        
        # Батарея
        d += elm.Battery().at((1, 8)).down(1).label('LiPo 1S\n3.7V\n500-1000mAh', loc='left', fontsize=9)
        bat_pos = d.here
        
        # Выключатель
        d += elm.Line().right(1)
        d += elm.Switch().right(1).label('Power\nSwitch', loc='top', fontsize=8)
        
        # Защитный диод
        d += elm.Line().right(0.5)
        d += elm.Diode().right(1).label('Schottky\n1N5819', loc='top', fontsize=8)
        d += elm.Line().right(0.5)
        
        # Разветвление питания
        split_node = d.here
        d += elm.Dot()
        
        # Ветка 1: Питание моторов (3.7V)
        d += elm.Line().at(split_node).up(1)
        d += elm.Label().label('3.7V для моторов', fontsize=9)
        d += elm.Line().right(0.5)
        d += elm.Capacitor().down(1).label('220µF\nФильтр', loc='right', fontsize=8)
        d += elm.Line().to((split_node[0] + 0.5, split_node[1]))
        d += elm.Line().right(0.5)
        d += elm.Ic(pins=[
            elm.IcPin(name='VM', side='top'),
            elm.IcPin(name='GND', side='bottom')
        ], size=(1.5, 1)).label('MX1508\nДрайвер', loc='center', fontsize=9)
        d += elm.Line().down(0.3)
        d += elm.Ground()
        
        # Ветка 2: Buck converter для 5V
        d += elm.Line().at(split_node).down(1)
        d += elm.Label().label('3.7V → 5V', fontsize=9)
        d += elm.Line().right(0.5)
        d += elm.Ic(pins=[
            elm.IcPin(name='IN', side='left'),
            elm.IcPin(name='GND', side='bottom'),
            elm.IcPin(name='5V', side='right')
        ], size=(1.2, 0.8)).label('Buck\nConverter', loc='center', fontsize=9)
        buck_gnd = d.here
        d += elm.Line().down(0.3)
        d += elm.Ground()
        
        # 5V разветвление
        d += elm.Line().at((buck_gnd[0] + 1.2, buck_gnd[1])).right(0.5)
        fv_node = d.here
        d += elm.Dot()
        d += elm.Capacitor().down(1).label('100µF\nРазвязка', loc='right', fontsize=8)
        d += elm.Ground()
        
        # ESP32-CAM
        d += elm.Line().at(fv_node).up(0.5)
        d += elm.Label().label('5V', fontsize=8)
        d += elm.Line().right(0.5)
        d += elm.Ic(pins=[
            elm.IcPin(name='5V', side='left'),
            elm.IcPin(name='GND', side='bottom')
        ], size=(1.5, 1.2)).label('ESP32-CAM\n~300mA', loc='center', fontsize=9)
        d += elm.Line().down(0.3)
        d += elm.Ground()
        
        # NeoPixel LEDs
        d += elm.Line().at(fv_node).down(1.5)
        d += elm.Label().label('5V', fontsize=8)
        d += elm.Line().right(0.5)
        d += elm.Ic(pins=[
            elm.IcPin(name='5V', side='left'),
            elm.IcPin(name='GND', side='bottom')
        ], size=(1.5, 0.8)).label('NeoPixel x3\nдо 180mA', loc='center', fontsize=9)
        d += elm.Line().down(0.3)
        d += elm.Ground()
        
        # Общая земля
        d += elm.Line().at((1, bat_pos[1] - 1)).right(7)
        d += elm.Line().down(0.3)
        d += elm.Ground()
        d += elm.Label().at((4.5, bat_pos[1] - 1.5)).label('Общая земля (GND)', fontsize=9, fontweight='bold')
        
        # Легенда токов
        d += elm.Label().at((1, 1.5)).label('Потребление тока:', fontsize=10, fontweight='bold')
        d += elm.Label().at((1, 1.2)).label('• ESP32-CAM: ~300mA (пик до 500mA с WiFi)', fontsize=8)
        d += elm.Label().at((1, 0.9)).label('• NeoPixel x3: до 180mA (60mA каждый на полной яркости)', fontsize=8)
        d += elm.Label().at((1, 0.6)).label('• Моторы x2: до 2A пиковый (по 1A каждый)', fontsize=8)
        d += elm.Label().at((1, 0.3)).label('• ИТОГО: до 2.5A при максимальной нагрузке', fontsize=8, fontweight='bold')
        
        d.save('/home/runner/work/microbbox/microbbox/hardware/diagrams/power_distribution.svg')
        d.save('/home/runner/work/microbbox/microbbox/hardware/diagrams/power_distribution.png', dpi=300)
        
    print("✓ Схема питания создана: power_distribution.svg/png")


def create_pinout_diagram():
    """Создает диаграмму распиновки ESP32-CAM"""
    
    with schemdraw.Drawing(show=False) as d:
        d.config(fontsize=9, font='sans-serif')
        
        # Заголовок
        d += elm.Label().at((4, 11)).label('ESP32-CAM Pinout - МикРоББокс', fontsize=14, fontweight='bold')
        
        # ESP32-CAM как большая микросхема
        esp = d += elm.Ic(pins=[
            # Левая сторона
            elm.IcPin(name='5V', side='left', pin='1'),
            elm.IcPin(name='GND', side='left', pin='2'),
            elm.IcPin(name='GPIO12', side='left', pin='3'),
            elm.IcPin(name='GPIO13', side='left', pin='4'),
            elm.IcPin(name='GPIO15', side='left', pin='5'),
            elm.IcPin(name='GPIO14', side='left', pin='6'),
            elm.IcPin(name='GPIO2', side='left', pin='7'),
            elm.IcPin(name='GPIO4', side='left', pin='8'),
            # Правая сторона
            elm.IcPin(name='GPIO16', side='right', pin='9'),
            elm.IcPin(name='GPIO0', side='right', pin='10'),
            elm.IcPin(name='XCLK', side='right', pin='11'),
            elm.IcPin(name='SIOD', side='right', pin='12'),
            elm.IcPin(name='SIOC', side='right', pin='13'),
            elm.IcPin(name='Y9-Y2', side='right', pin='14'),
            elm.IcPin(name='PCLK', side='right', pin='15'),
            elm.IcPin(name='VSYNC', side='right', pin='16'),
        ], leadlen=1, size=(3, 5)).at((4, 6)).label('ESP32-CAM\nAI Thinker\nOV2640', loc='center', fontsize=11)
        
        # Подписи для левой стороны (наши подключения)
        pins_left = [
            ('5V', 'Питание 5V'),
            ('GND', 'Общая земля'),
            ('GPIO12', 'Motor L FWD (IN1) + 10kΩ↓'),
            ('GPIO13', 'Motor L REV (IN2) + 10kΩ↓'),
            ('GPIO15', 'Motor R FWD (IN3) + 10kΩ↓'),
            ('GPIO14', 'Motor R REV (IN4) + 10kΩ↓'),
            ('GPIO2', 'NeoPixel Data (WS2812B)'),
            ('GPIO4', 'Не используется (вспышка)'),
        ]
        
        y_offset = 0
        for pin_name, description in pins_left:
            pin_pos = esp.pins[pin_name].start
            d += elm.Label().at((pin_pos[0] - 2.5, pin_pos[1])).label(description, fontsize=8, halign='left')
        
        # Подписи для правой стороны (камера и служебные)
        pins_right = [
            ('GPIO16', 'Buzzer (Бузер)'),
            ('GPIO0', 'Camera XCLK'),
            ('XCLK', 'Camera Clock'),
            ('SIOD', 'Camera I2C Data'),
            ('SIOC', 'Camera I2C Clock'),
            ('Y9-Y2', 'Camera Data Pins'),
            ('PCLK', 'Camera Pixel Clock'),
            ('VSYNC', 'Camera VSync'),
        ]
        
        for pin_name, description in pins_right:
            pin_pos = esp.pins[pin_name].end
            d += elm.Label().at((pin_pos[0] + 2.5, pin_pos[1])).label(description, fontsize=8, halign='right')
        
        # Цветовое кодирование
        d += elm.Label().at((1, 1.5)).label('Цветовое кодирование:', fontsize=10, fontweight='bold')
        d += elm.Line().at((1, 1.2)).right(0.3).color('red').linewidth(3)
        d += elm.Label().at((1.5, 1.2)).label('Питание (5V)', fontsize=8, halign='left')
        
        d += elm.Line().at((1, 0.9)).right(0.3).color('black').linewidth(3)
        d += elm.Label().at((1.5, 0.9)).label('Земля (GND)', fontsize=8, halign='left')
        
        d += elm.Line().at((1, 0.6)).right(0.3).color('blue').linewidth(3)
        d += elm.Label().at((1.5, 0.6)).label('Моторы (требуют pull-down!)', fontsize=8, halign='left')
        
        d += elm.Line().at((1, 0.3)).right(0.3).color('green').linewidth(3)
        d += elm.Label().at((1.5, 0.3)).label('Светодиоды и эффекты', fontsize=8, halign='left')
        
        d += elm.Line().at((5, 1.2)).right(0.3).color('gray').linewidth(3)
        d += elm.Label().at((5.5, 1.2)).label('Камера (автоматически)', fontsize=8, halign='left')
        
        d.save('/home/runner/work/microbbox/microbbox/hardware/diagrams/esp32cam_pinout.svg')
        d.save('/home/runner/work/microbbox/microbbox/hardware/diagrams/esp32cam_pinout.png', dpi=300)
        
    print("✓ Диаграмма распиновки создана: esp32cam_pinout.svg/png")


if __name__ == '__main__':
    print("Генерация схем МикРоББокс...")
    print("-" * 50)
    
    try:
        create_wiring_diagram()
        create_power_distribution()
        create_pinout_diagram()
        
        print("-" * 50)
        print("✅ Все схемы успешно созданы!")
        print("\nФайлы сохранены в:")
        print("  hardware/diagrams/wiring_schematic.svg (и .png)")
        print("  hardware/diagrams/power_distribution.svg (и .png)")
        print("  hardware/diagrams/esp32cam_pinout.svg (и .png)")
        
    except Exception as e:
        print(f"❌ Ошибка при создании схем: {e}")
        import traceback
        traceback.print_exc()
