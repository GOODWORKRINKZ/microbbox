# Инструкции для GitHub Copilot - МикРоББокс

## 🗣️ Главное правило
**ВСЕГДА пиши на РУССКОМ ЯЗЫКЕ** - комментарии, документацию, сообщения.
Названия переменных/функций/классов - на английском.

---

## 🏗️ Backend (C++ / ESP32)

### Принципы SOLID, KISS, DRY

**SOLID:**
- **S** - один класс = одна ответственность (`ClassicRobot`, `LinerRobot`, `BrainRobot`)
- **O** - новые типы через наследование от `BaseRobot`
- **L** - любой робот заменяет `BaseRobot`
- **I** - маленькие интерфейсы (`IComponent`, `IMotorController`)
- **D** - зависимость от абстракций, не реализаций

**DRY:** Общий код в `BaseRobot`, не дублируй.  
**KISS:** Простое решение лучше сложного.

### Стиль кода

```cpp
// ✅ ПРАВИЛЬНО
class MyRobot : public BaseRobot {
public:
    MyRobot();
    virtual ~MyRobot();
    RobotType getRobotType() const override;
    
protected:
    bool initSpecificComponents() override;
    
private:
    int motorSpeed_;  // Приватные с _
    void helperMethod();
};
```

**Именование:**
- Классы: `PascalCase` - `ClassicRobot`
- Методы: `camelCase` - `initMotors()`
- Приватные: `camelCase_` - `motorSpeed_`
- Константы: `UPPER_SNAKE_CASE`

---

## 🌐 Frontend (JavaScript / HTML / CSS)

### Архитектура (SOLID)

- `Logger` - логирование
- `CommandController` - отправка команд
- `BaseRobotUI` - базовый класс UI
- `ClassicRobotUI`, `LinerRobotUI` - специфичные UI
- `RobotUIFactory` - фабрика

### Стиль кода

```javascript
// ✅ Async/await, не callbacks
async function sendCommand(cmd) {
    try {
        const response = await fetch('/cmd');
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        return await response.text();
    } catch (error) {
        Logger.error('Ошибка команды', error);
        throw error;
    }
}

// ✅ Классы с наследованием
class ClassicRobotUI extends BaseRobotUI {
    async initSpecificElements() {
        this.initJoysticks();
        this.initKeyboard();
    }
}

// ✅ Фабрика
class RobotUIFactory {
    static async create() {
        const data = await fetch('/api/robot-type').then(r => r.json());
        switch(data.type) {
            case 'classic': return new ClassicRobotUI();
            case 'liner': return new LinerRobotUI();
        }
    }
}
```

**Именование:**
- Классы: `PascalCase`
- Методы: `camelCase`
- Константы: `UPPER_SNAKE_CASE`

---

## 🚫 Что НЕ делать

- ❌ НЕ добавляй `#ifdef` в реализации классов
- ❌ НЕ дублируй код между типами роботов
- ❌ НЕ создавай "божественные" классы
- ❌ НЕ используй callbacks - только async/await
- ❌ НЕ изменяй `target_config.h` вручную

---

## ✅ Чек-лист

- [ ] Комментарии на русском
- [ ] SOLID, KISS, DRY
- [ ] Нет дублирования
- [ ] Async/await в JS
- [ ] Код компилируется без warnings

---

## 🎯 Быстрые примеры

### Новый тип робота
1. Создай `MyRobot.h` + `MyRobot.cpp`
2. Наследуй от `BaseRobot`
3. Добавь в `platformio.ini` и `main.cpp`

### Новый UI
1. Создай `MyRobotUI extends BaseRobotUI`
2. Переопредели `initSpecificElements()`
3. Добавь в `RobotUIFactory`

---

**МикРоББокс 0.1** | MIT License
