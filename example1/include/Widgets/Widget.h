#ifndef WIDGET_H
#define WIDGET_H

#include <TFT_eSPI.h>
#include "Context/DisplayContext.h"

class Widget {
public:
    // Конструктор виджета может принимать дополнительные параметры, если это необходимо
    Widget(TFT_eSPI *display) : tft(display) {}

    // Виртуальный деструктор гарантирует корректное уничтожение производных объектов
    virtual ~Widget() {}

    // Метод для обновления виджета данными из контекста
    virtual void update(DisplayContext& context) = 0;

    // Метод для отрисовки виджета на дисплее
    virtual void draw() = 0;

protected:
    TFT_eSPI* tft; // Указатель на объект TFT_eSPI, который используется для управления дисплеем
};

#endif // WIDGET_H