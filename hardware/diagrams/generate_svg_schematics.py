#!/usr/bin/env python3
"""
Генератор схемы подключения МикРоББокс в формате SVG
Создает простую и понятную схему соединений
"""

def create_svg_wiring():
    """Создает SVG схему подключения"""
    
    svg = '''<?xml version="1.0" encoding="UTF-8"?>
<svg width="1200" height="800" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <style>
      .title { font: bold 24px sans-serif; }
      .subtitle { font: 16px sans-serif; }
      .label { font: 12px sans-serif; }
      .small { font: 10px sans-serif; }
      .wire { stroke: #333; stroke-width: 2; fill: none; }
      .wire-power { stroke: #d32f2f; stroke-width: 3; fill: none; }
      .wire-gnd { stroke: #000; stroke-width: 3; fill: none; }
      .wire-signal { stroke: #1976d2; stroke-width: 2; fill: none; }
      .component { fill: #e3f2fd; stroke: #1976d2; stroke-width: 2; }
      .esp32 { fill: #fff3e0; stroke: #f57c00; stroke-width: 3; }
      .resistor { fill: #fff9c4; stroke: #f57f17; stroke-width: 2; }
      .motor { fill: #e8f5e9; stroke: #388e3c; stroke-width: 2; }
      .led { fill: #fce4ec; stroke: #c2185b; stroke-width: 2; }
    </style>
  </defs>
  
  <!-- Заголовок -->
  <text x="600" y="40" text-anchor="middle" class="title">МикРоББокс - Схема подключения</text>
  <text x="600" y="65" text-anchor="middle" class="subtitle">ESP32-CAM с MX1508, NeoPixel LED и Бузером</text>
  
  <!-- ESP32-CAM (центр) -->
  <rect x="450" y="300" width="300" height="200" class="esp32" rx="5"/>
  <text x="600" y="340" text-anchor="middle" class="label" font-weight="bold">ESP32-CAM</text>
  <text x="600" y="360" text-anchor="middle" class="small">AI Thinker</text>
  
  <!-- Пины ESP32 слева -->
  <text x="460" y="395" class="small">5V</text>
  <circle cx="445" cy="390" r="4" fill="#d32f2f"/>
  
  <text x="460" y="420" class="small">GND</text>
  <circle cx="445" cy="415" r="4" fill="#000"/>
  
  <text x="460" y="445" class="small">GPIO2</text>
  <circle cx="445" cy="440" r="4" fill="#1976d2"/>
  
  <text x="460" y="470" class="small">GPIO16</text>
  <circle cx="445" cy="465" r="4" fill="#1976d2"/>
  
  <!-- Пины ESP32 справа -->
  <text x="730" y="395" class="small" text-anchor="end">GPIO12</text>
  <circle cx="755" cy="390" r="4" fill="#1976d2"/>
  
  <text x="730" y="420" class="small" text-anchor="end">GPIO13</text>
  <circle cx="755" cy="415" r="4" fill="#1976d2"/>
  
  <text x="730" y="445" class="small" text-anchor="end">GPIO14</text>
  <circle cx="755" cy="440" r="4" fill="#1976d2"/>
  
  <text x="730" y="470" class="small" text-anchor="end">GPIO15</text>
  <circle cx="755" cy="465" r="4" fill="#1976d2"/>
  
  <!-- Питание (сверху) -->
  <rect x="500" y="100" width="200" height="80" class="component" rx="5"/>
  <text x="600" y="130" text-anchor="middle" class="label">Источник питания</text>
  <text x="600" y="150" text-anchor="middle" class="small">LiPo 1S 3.7V → Buck 5V</text>
  <text x="600" y="165" text-anchor="middle" class="small">+ защита + развязка</text>
  
  <!-- Линии питания -->
  <line x1="600" y1="180" x2="600" y2="250" class="wire-power" marker-end="url(#arrowred)"/>
  <text x="610" y="220" class="small">5V</text>
  <line x1="600" y1="250" x2="445" y2="390" class="wire-power"/>
  
  <!-- GND -->
  <line x1="500" y1="180" x2="500" y2="250" class="wire-gnd"/>
  <line x1="500" y1="250" x2="445" y2="415" class="wire-gnd"/>
  <text x="490" y="220" class="small" text-anchor="end">GND</text>
  
  <!-- MX1508 Motor Driver (справа) -->
  <rect x="850" y="300" width="180" height="180" class="component" rx="5"/>
  <text x="940" y="330" text-anchor="middle" class="label" font-weight="bold">MX1508</text>
  <text x="940" y="345" text-anchor="middle" class="small">Motor Driver</text>
  
  <!-- Входы MX1508 -->
  <text x="855" y="375" class="small">IN1</text>
  <circle cx="845" cy="370" r="4" fill="#1976d2"/>
  <text x="855" y="400" class="small">IN2</text>
  <circle cx="845" cy="395" r="4" fill="#1976d2"/>
  <text x="855" y="425" class="small">IN3</text>
  <circle cx="845" cy="420" r="4" fill="#1976d2"/>
  <text x="855" y="450" class="small">IN4</text>
  <circle cx="845" cy="445" r="4" fill="#1976d2"/>
  
  <!-- Выходы MX1508 к моторам -->
  <text x="1020" y="375" class="small" text-anchor="end">OUT1</text>
  <circle cx="1035" cy="370" r="4" fill="#388e3c"/>
  <text x="1020" y="400" class="small" text-anchor="end">OUT2</text>
  <circle cx="1035" cy="395" r="4" fill="#388e3c"/>
  <text x="1020" y="425" class="small" text-anchor="end">OUT3</text>
  <circle cx="1035" cy="420" r="4" fill="#388e3c"/>
  <text x="1020" y="450" class="small" text-anchor="end">OUT4</text>
  <circle cx="1035" cy="445" r="4" fill="#388e3c"/>
  
  <!-- Pull-down резисторы -->
  <!-- GPIO12 к IN1 -->
  <line x1="755" y1="390" x2="800" y2="390" class="wire-signal"/>
  <circle cx="800" cy="390" r="3" fill="#333"/>
  <line x1="800" y1="390" x2="845" y2="370" class="wire-signal"/>
  <rect x="795" y="395" width="10" height="30" class="resistor"/>
  <text x="810" y="415" class="small">10kΩ</text>
  <line x1="800" y1="425" x2="800" y2="440" class="wire-gnd"/>
  <line x1="795" y1="440" x2="805" y2="440" class="wire-gnd"/>
  
  <!-- GPIO13 к IN2 -->
  <line x1="755" y1="415" x2="790" y2="415" class="wire-signal"/>
  <circle cx="790" cy="415" r="3" fill="#333"/>
  <line x1="790" y1="415" x2="845" y2="395" class="wire-signal"/>
  <rect x="785" y="420" width="10" height="30" class="resistor"/>
  <text x="800" y="440" class="small">10kΩ</text>
  <line x1="790" y1="450" x2="790" y2="465" class="wire-gnd"/>
  <line x1="785" y1="465" x2="795" y2="465" class="wire-gnd"/>
  
  <!-- GPIO14 к IN4 -->
  <line x1="755" y1="440" x2="810" y2="440" class="wire-signal"/>
  <circle cx="810" cy="440" r="3" fill="#333"/>
  <line x1="810" y1="440" x2="845" y2="445" class="wire-signal"/>
  <rect x="805" y="445" width="10" height="30" class="resistor"/>
  <text x="820" y="465" class="small">10kΩ</text>
  <line x1="810" y1="475" x2="810" y2="490" class="wire-gnd"/>
  <line x1="805" y1="490" x2="815" y2="490" class="wire-gnd"/>
  
  <!-- GPIO15 к IN3 -->
  <line x1="755" y1="465" x2="820" y2="465" class="wire-signal"/>
  <circle cx="820" cy="465" r="3" fill="#333"/>
  <line x1="820" y1="465" x2="845" y2="420" class="wire-signal"/>
  <rect x="815" y="470" width="10" height="30" class="resistor"/>
  <text x="830" y="490" class="small">10kΩ</text>
  <line x1="820" y1="500" x2="820" y2="515" class="wire-gnd"/>
  <line x1="815" y1="515" x2="825" y2="515" class="wire-gnd"/>
  
  <!-- Моторы -->
  <rect x="1070" y="350" width="100" height="50" class="motor" rx="5"/>
  <text x="1120" y="375" text-anchor="middle" class="small" font-weight="bold">Левый мотор</text>
  <text x="1120" y="390" text-anchor="middle" class="small">DC 3-7V</text>
  <line x1="1035" y1="370" x2="1070" y2="365" class="wire-signal"/>
  <line x1="1035" y1="395" x2="1070" y2="385" class="wire-signal"/>
  
  <rect x="1070" y="420" width="100" height="50" class="motor" rx="5"/>
  <text x="1120" y="445" text-anchor="middle" class="small" font-weight="bold">Правый мотор</text>
  <text x="1120" y="460" text-anchor="middle" class="small">DC 3-7V</text>
  <line x1="1035" y1="420" x2="1070" y2="435" class="wire-signal"/>
  <line x1="1035" y1="445" x2="1070" y2="455" class="wire-signal"/>
  
  <!-- NeoPixel LED цепочка (слева внизу) -->
  <rect x="250" y="420" width="80" height="40" class="led" rx="5"/>
  <text x="290" y="440" text-anchor="middle" class="small" font-weight="bold">LED #1</text>
  <text x="290" y="452" text-anchor="middle" class="small">NeoPixel</text>
  <circle cx="245" cy="440" r="4" fill="#1976d2"/>
  <line x1="445" y1="440" x2="245" y2="440" class="wire-signal"/>
  <text x="350" y="430" class="small">GPIO2 Data</text>
  
  <rect x="180" y="420" width="60" height="40" class="led" rx="5"/>
  <text x="210" y="440" text-anchor="middle" class="small" font-weight="bold">LED #2</text>
  <line x1="250" y1="440" x2="240" y2="440" class="wire-signal"/>
  
  <rect x="110" y="420" width="60" height="40" class="led" rx="5"/>
  <text x="140" y="440" text-anchor="middle" class="small" font-weight="bold">LED #3</text>
  <line x1="180" y1="440" x2="170" y2="440" class="wire-signal"/>
  
  <!-- Бузер (слева снизу) -->
  <rect x="300" y="520" width="80" height="50" class="component" rx="5"/>
  <text x="340" y="545" text-anchor="middle" class="small" font-weight="bold">Buzzer</text>
  <text x="340" y="560" text-anchor="middle" class="small">Активный 3.3V</text>
  <circle cx="340" cy="515" r="4" fill="#1976d2"/>
  <line x1="445" y1="465" x2="440" y2="465" class="wire-signal"/>
  <line x1="440" y1="465" x2="440" y2="545" class="wire-signal"/>
  <line x1="440" y1="545" x2="380" y2="545" class="wire-signal"/>
  <text x="420" y="535" class="small">GPIO16</text>
  
  <!-- Легенда -->
  <rect x="50" y="630" width="1100" height="140" fill="#f5f5f5" stroke="#666" stroke-width="1" rx="5"/>
  <text x="70" y="655" class="label" font-weight="bold">⚠️ ВАЖНО:</text>
  
  <line x1="70" y1="675" x2="100" y2="675" stroke="#f57f17" stroke-width="3"/>
  <text x="110" y="680" class="small">Pull-down резисторы 10kΩ ОБЯЗАТЕЛЬНЫ для GPIO12-15!</text>
  <text x="110" y="695" class="small">Без них моторы будут крутиться при Reset/включении питания</text>
  
  <line x1="70" y1="710" x2="100" y2="710" stroke="#d32f2f" stroke-width="3"/>
  <text x="110" y="715" class="small">Конденсаторы развязки: 100µF для ESP32-CAM, 220µF для питания моторов MX1508</text>
  
  <line x1="70" y1="730" x2="100" y2="730" stroke="#1976d2" stroke-width="3"/>
  <text x="110" y="735" class="small">NeoPixel подключаются цепочкой: GPIO2 → LED1 → LED2 → LED3</text>
  
  <line x1="70" y1="750" x2="100" y2="750" stroke="#000" stroke-width="3"/>
  <text x="110" y="755" class="small">Общая земля (GND) должна быть соединена для всех компонентов!</text>
  
  <text x="1100" y="765" class="small" text-anchor="end" font-style="italic">МикРоББокс v1.0 | 2025-11-01</text>
</svg>'''
    
    with open('/home/runner/work/microbbox/microbbox/hardware/diagrams/connection_schematic.svg', 'w', encoding='utf-8') as f:
        f.write(svg)
    
    print("✓ Схема подключения создана: connection_schematic.svg")


def create_svg_power():
    """Создает SVG схему распределения питания"""
    
    svg = '''<?xml version="1.0" encoding="UTF-8"?>
<svg width="1000" height="700" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <style>
      .title { font: bold 20px sans-serif; }
      .label { font: 12px sans-serif; }
      .small { font: 10px sans-serif; }
      .wire-power { stroke: #d32f2f; stroke-width: 3; fill: none; }
      .wire-gnd { stroke: #000; stroke-width: 3; fill: none; }
      .component { fill: #e3f2fd; stroke: #1976d2; stroke-width: 2; }
      .battery { fill: #ffe6e6; stroke: #c62828; stroke-width: 2; }
    </style>
  </defs>
  
  <text x="500" y="40" text-anchor="middle" class="title">Схема распределения питания МикРоББокс</text>
  
  <!-- Батарея -->
  <rect x="50" y="100" width="100" height="120" class="battery" rx="5"/>
  <text x="100" y="140" text-anchor="middle" class="label" font-weight="bold">LiPo 1S</text>
  <text x="100" y="160" text-anchor="middle" class="small">3.7V nominal</text>
  <text x="100" y="175" text-anchor="middle" class="small">500-1000mAh</text>
  <text x="100" y="195" text-anchor="middle" class="small" fill="#d32f2f">+ (4.2V max)</text>
  <text x="100" y="210" text-anchor="middle" class="small" fill="#000">- (GND)</text>
  
  <!-- Выключатель -->
  <rect x="180" y="130" width="60" height="50" class="component" rx="5"/>
  <text x="210" y="155" text-anchor="middle" class="small" font-weight="bold">Switch</text>
  <text x="210" y="170" text-anchor="middle" class="small">ON/OFF</text>
  <line x1="150" y1="150" x2="180" y2="150" class="wire-power"/>
  
  <!-- Защитный диод -->
  <polygon points="270,140 290,140 280,160" fill="#333"/>
  <line x1="280" y1="160" x2="280" y2="165" stroke="#333" stroke-width="2"/>
  <line x1="275" y1="165" x2="285" y2="165" stroke="#333" stroke-width="2"/>
  <text x="280" y="185" text-anchor="middle" class="small">Diode</text>
  <text x="280" y="197" text-anchor="middle" class="small">1N5819</text>
  <line x1="240" y1="150" x2="270" y2="150" class="wire-power"/>
  <line x1="290" y1="150" x2="330" y2="150" class="wire-power"/>
  
  <!-- Разветвление -->
  <circle cx="330" cy="150" r="5" fill="#d32f2f"/>
  
  <!-- Ветка 1: Buck Converter для ESP32 -->
  <line x1="330" y1="150" x2="400" y2="150" class="wire-power"/>
  <rect x="400" y="120" width="100" height="60" class="component" rx="5"/>
  <text x="450" y="145" text-anchor="middle" class="label" font-weight="bold">Buck</text>
  <text x="450" y="160" text-anchor="middle" class="small">3.7V → 5V</text>
  <text x="450" y="173" text-anchor="middle" class="small">до 3A</text>
  
  <!-- Конденсатор на входе Buck -->
  <rect x="385" y="190" width="10" height="40" fill="#fff9c4" stroke="#f57f17" stroke-width="1"/>
  <text x="395" y="240" text-anchor="middle" class="small">220µF</text>
  <line x1="390" y1="180" x2="390" y2="190" class="wire-power"/>
  <line x1="390" y1="230" x2="390" y2="250" class="wire-gnd"/>
  
  <!-- 5V выход Buck -->
  <line x1="500" y1="150" x2="580" y2="150" class="wire-power"/>
  <text x="540" y="140" class="small" fill="#d32f2f" font-weight="bold">5V</text>
  
  <!-- Конденсатор развязки ESP32 -->
  <rect x="575" y="160" width="10" height="40" fill="#fff9c4" stroke="#f57f17" stroke-width="1"/>
  <text x="580" y="215" text-anchor="middle" class="small">100µF</text>
  <line x1="580" y1="150" x2="580" y2="160" class="wire-power"/>
  <line x1="580" y1="200" x2="580" y2="220" class="wire-gnd"/>
  
  <!-- ESP32-CAM -->
  <line x1="580" y1="150" x2="650" y2="150" class="wire-power"/>
  <rect x="650" y="120" width="120" height="70" fill="#fff3e0" stroke="#f57c00" stroke-width="2" rx="5"/>
  <text x="710" y="145" text-anchor="middle" class="label" font-weight="bold">ESP32-CAM</text>
  <text x="710" y="162" text-anchor="middle" class="small">5V input</text>
  <text x="710" y="177" text-anchor="middle" class="small">~300mA</text>
  <text x="710" y="190" text-anchor="middle" class="small" fill="#757575">(пик 500mA)</text>
  
  <!-- NeoPixel LEDs -->
  <line x1="580" y1="150" x2="580" y2="280" class="wire-power"/>
  <line x1="580" y1="280" x2="650" y2="280" class="wire-power"/>
  <rect x="650" y="250" width="120" height="60" fill="#fce4ec" stroke="#c2185b" stroke-width="2" rx="5"/>
  <text x="710" y="275" text-anchor="middle" class="label" font-weight="bold">NeoPixel x3</text>
  <text x="710" y="292" text-anchor="middle" class="small">5V</text>
  <text x="710" y="305" text-anchor="middle" class="small">до 180mA</text>
  
  <!-- Ветка 2: Прямое питание моторов 3.7V -->
  <line x1="330" y1="150" x2="330" y2="350" class="wire-power"/>
  <line x1="330" y1="350" x2="400" y2="350" class="wire-power"/>
  <text x="360" y="340" class="small" fill="#d32f2f" font-weight="bold">3.7V</text>
  
  <!-- Конденсатор для моторов -->
  <rect x="385" y="360" width="10" height="50" fill="#fff9c4" stroke="#f57f17" stroke-width="1"/>
  <text x="390" y="425" text-anchor="middle" class="small">220µF</text>
  <line x1="390" y1="350" x2="390" y2="360" class="wire-power"/>
  <line x1="390" y1="410" x2="390" y2="430" class="wire-gnd"/>
  
  <!-- MX1508 -->
  <rect x="450" y="320" width="100" height="80" class="component" rx="5"/>
  <text x="500" y="350" text-anchor="middle" class="label" font-weight="bold">MX1508</text>
  <text x="500" y="367" text-anchor="middle" class="small">Motor Driver</text>
  <text x="500" y="382" text-anchor="middle" class="small">VM = 3.7V</text>
  <text x="500" y="395" text-anchor="middle" class="small">до 2A пик</text>
  
  <!-- Моторы -->
  <line x1="550" y1="350" x2="620" y2="330" class="wire-power"/>
  <rect x="620" y="310" width="90" height="40" fill="#e8f5e9" stroke="#388e3c" stroke-width="2" rx="5"/>
  <text x="665" y="333" text-anchor="middle" class="small" font-weight="bold">Левый мотор</text>
  
  <line x1="550" y1="370" x2="620" y2="390" class="wire-power"/>
  <rect x="620" y="370" width="90" height="40" fill="#e8f5e9" stroke="#388e3c" stroke-width="2" rx="5"/>
  <text x="665" y="393" text-anchor="middle" class="small" font-weight="bold">Правый мотор</text>
  
  <!-- Общая земля (GND rail) -->
  <line x1="100" y1="220" x2="100" y2="500" class="wire-gnd"/>
  <line x1="100" y1="500" x2="850" y2="500" class="wire-gnd"/>
  <text x="475" y="520" text-anchor="middle" class="label" font-weight="bold">Общая земля (GND)</text>
  
  <!-- GND подключения -->
  <line x1="390" y1="250" x2="390" y2="500" class="wire-gnd"/>
  <line x1="450" y1="160" x2="450" y2="500" class="wire-gnd"/>
  <line x1="580" y1="220" x2="580" y2="500" class="wire-gnd"/>
  <line x1="710" y1="190" x2="710" y2="500" class="wire-gnd"/>
  <line x1="710" y1="310" x2="710" y2="500" class="wire-gnd"/>
  <line x1="390" y1="430" x2="390" y2="500" class="wire-gnd"/>
  <line x1="500" y1="400" x2="500" y2="500" class="wire-gnd"/>
  
  <!-- Таблица потребления -->
  <rect x="50" y="550" width="900" height="120" fill="#f5f5f5" stroke="#666" stroke-width="1" rx="5"/>
  <text x="70" y="575" class="label" font-weight="bold">Потребление тока:</text>
  
  <text x="80" y="600" class="small">• ESP32-CAM (WiFi активен): ~300mA номинально, пик до 500mA при передаче</text>
  <text x="80" y="618" class="small">• NeoPixel LED x3: до 180mA (60mA каждый на максимальной яркости белым)</text>
  <text x="80" y="636" class="small">• Моторы x2: до 2A пиковый (по 1A каждый при максимальной нагрузке)</text>
  <text x="80" y="654" class="small" font-weight="bold">• ИТОГО: до 2.5-3A при полной нагрузке (рекомендуется источник минимум 3A)</text>
  
  <text x="900" y="665" class="small" text-anchor="end" font-style="italic">МикРоББокс v1.0</text>
</svg>'''
    
    with open('/home/runner/work/microbbox/microbbox/hardware/diagrams/power_distribution.svg', 'w', encoding='utf-8') as f:
        f.write(svg)
    
    print("✓ Схема питания создана: power_distribution.svg")


if __name__ == '__main__':
    print("Генерация SVG схем МикРоББокс...")
    print("-" * 50)
    
    create_svg_wiring()
    create_svg_power()
    
    print("-" * 50)
    print("✅ SVG схемы успешно созданы!")
    print("\nФайлы:")
    print("  hardware/diagrams/connection_schematic.svg")
    print("  hardware/diagrams/power_distribution.svg")
    print("\nДля просмотра откройте SVG файлы в браузере или GitHub.")
