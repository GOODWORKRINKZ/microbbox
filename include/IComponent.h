#ifndef ICOMPONENT_H
#define ICOMPONENT_H

// ═══════════════════════════════════════════════════════════════
// БАЗОВЫЙ ИНТЕРФЕЙС ДЛЯ ВСЕХ КОМПОНЕНТОВ РОБОТА
// ═══════════════════════════════════════════════════════════════
// Следует принципу Interface Segregation (SOLID)
// Каждый компонент имеет единую ответственность (Single Responsibility)

class IComponent {
public:
    virtual ~IComponent() = default;
    
    // Инициализация компонента
    virtual bool init() = 0;
    
    // Обновление состояния (вызывается в loop)
    virtual void update() = 0;
    
    // Завершение работы компонента
    virtual void shutdown() = 0;
    
    // Проверка инициализации
    virtual bool isInitialized() const = 0;
};

#endif // ICOMPONENT_H
