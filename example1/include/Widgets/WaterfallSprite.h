#include <TFT_eSPI.h>
#include <utils.h>

class WaterfallSprite : public TFT_eSprite
{
public:
    using TFT_eSprite::TFT_eSprite; // Используем конструктор базового класса

    // Функция для отрисовки диаграммы "водопад"
    void drawWaterfall(uint8_t *data)
    {
        // Получаем размеры спрайта и общее количество пикселей
        int16_t height = this->height();
        int16_t width = this->width();

        // Перемещаем данные на один пиксель вниз
        memmove(_img + width, _img, (height - 1) * width * 2); // Размер пикселя - 2 байта (RGB565)

        // Отрисовываем новую верхнюю строку на основе данных RSSI
        for (size_t x = 0; x < width; ++x)
        {
            uint16_t color = rssiToColor565(data[x]);
            // Пишем пиксель напрямую в буфер _img
            drawPixel(x, 0, color);
        }
    }
};